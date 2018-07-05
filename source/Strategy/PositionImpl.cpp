#include <cassert>
#include <algorithm>
#include "Bar.h"
#include "Utils.h"
#include "Errors.h"
#include "Position.h"
#include "PositionImpl.h"
#include "Runtime.h"
#include "Logger.h"

namespace xBacktest
{

////////////////////////////////////////////////////////////////////////////////
volatile unsigned long PositionImpl::m_nextPositionId = 1;

PositionImpl::PositionImpl()
{
    m_abstraction  = nullptr;
    m_runtime = nullptr;
    m_id = m_nextPositionId++;
}

PositionImpl::~PositionImpl()
{
}

void PositionImpl::init(Position* abstraction, Runtime* runtime, const Order& entryOrder)
{
    assert(runtime != nullptr);
    
    m_abstraction = abstraction;
    m_runtime = runtime;
    m_instrument = entryOrder.getInstrument();
    assert(!m_instrument.empty());
    m_entryShares = 0;
    m_exitShares = 0;
    m_allOrNone  = false;
    m_multiplier = runtime->getContract().multiplier;

    m_entryOrdId = entryOrder.getId();

    m_duration = 0;
    m_entryTriggeredPrice = entryOrder.getTriggerPrice();

    double avgPrice = entryOrder.getAvgFillPrice();

    assert(entryOrder.getAction() == Order::Action::BUY || 
           entryOrder.getAction() == Order::Action::SELL_SHORT);

    if (entryOrder.getAction() == Order::Action::BUY) {
        m_direction = Position::LongPos;
        m_entryType = SignalType::EntryLong;
    } else {
        m_direction = Position::ShortPos;
        m_entryType = SignalType::EntryShort;
    }

    m_entryOrders.push_back(entryOrder);
    m_allOrders.push_back(entryOrder);

    m_entryDateTime = entryOrder.getAcceptedDateTime();

    m_shares = 0;
    m_exitType = SignalType::Unknown;
    m_cost = 0;
    m_cash = 0;
    m_avgFillPrice = avgPrice;
    m_netPnL = 0;
    m_realizedPnL = 0;
    m_commissions = 0;
    m_slippages = 0;
    m_exitDateTime.markInvalid();

    m_histHighestPrice = avgPrice;
    m_histLowestPrice = avgPrice;
}

unsigned long PositionImpl::getId() const
{
    return m_id;
}

unsigned long PositionImpl::getEntryId() const
{
    ASSERT(m_subPosList.size() > 0, "Position is empty.");

    return m_subPosList.front().entryId;
}

Position* PositionImpl::getAbstraction() const
{
    return m_abstraction;
}

void PositionImpl::placeAndRegisterOrder(const Order& order)
{
    // Check if an order can be placed in the current state.
    if (!canPlaceOrder(order)) {
        ASSERT(false, "Order can not be placed in current state.");
    }

    m_runtime->submitOrder(order);
}

void PositionImpl::setEntryDateTime(const DateTime& dateTime)
{
    m_entryDateTime = dateTime;
}

const DateTime& PositionImpl::getEntryDateTime() const
{
    return m_entryDateTime;
}

void PositionImpl::setExitDateTime(const DateTime& dateTime)
{
    m_exitDateTime = dateTime;
}

const DateTime& PositionImpl::getExitDateTime() const
{
    return m_exitDateTime;
}

const DateTime& PositionImpl::getLastDateTime() const
{
    return m_lastBar.getDateTime();
}

int PositionImpl::getExitType() const
{
    return m_exitType;
}

double PositionImpl::getAvgFillPrice() const
{
    return m_avgFillPrice;
}

void PositionImpl::getTransactionRecords(TransactionList& outRecords)
{
    for (size_t i = 0; i < m_transactions.size(); i++) {
        outRecords.push_back(m_transactions[i]);
    }
}

void PositionImpl::getSubPositions(SubPositionList& outPos)
{
    for (const auto& subpos : m_subPosList) {
        if (subpos.currShares != 0) {
            SubPosition pos = { 0 };
            pos.id       = subpos.entryId;
            pos.datetime = subpos.entryDateTime;
            pos.price    = subpos.entryPrice;
            pos.shares   = subpos.currShares;
            pos.histHighestPrice = subpos.histHighestPrice;
            pos.histLowestPrice = subpos.histLowestPrice;
            outPos.push_back(pos);
        }
    }
}

int PositionImpl::getShares() const
{
    return m_shares;
}

double PositionImpl::getReturn(bool includeCommissions) const
{
    double ret = 0;
    double price = m_runtime->getLastPrice(m_instrument.c_str());
    if (price > 0) {
        double netProfit = getPnL();
        if (m_cost != 0) {
            ret = netProfit / m_cost;
        }
    }

    return ret;
}

double PositionImpl::getPnL(bool includeCommissions) const
{
    double ret = 0;
    double price = m_runtime->getLastPrice(m_instrument.c_str());
    if (price > 0) {
        ret = m_cash + m_shares * price * m_multiplier;
        ret -= (m_commissions + m_slippages);
        return ret;
    }

    return ret;
}

double PositionImpl::getCommissions() const
{
    return m_commissions;
}

double PositionImpl::getSlippages() const
{
    return m_slippages;
}

double PositionImpl::getRealizedPnL() const
{
    return m_realizedPnL;
}

bool PositionImpl::entryActive() const
{
    for (size_t i = 0; i < m_entryOrders.size(); i++) {
        if (m_entryOrders[i].isValid() && m_entryOrders[i].isActive()) {
            return true;
        }
    }

    return false;
}

bool PositionImpl::entryFilled() const
{
    for (size_t i = 0; i < m_entryOrders.size(); i++) {
        if (!m_entryOrders[i].isValid() || !m_entryOrders[i].isFilled()) {
            return false;
        }
    }

    return true;
}

bool PositionImpl::exitActive(int subPosId) const
{
    for (size_t i = 0; i < m_exitOrders.size(); i++) {
        if (m_exitOrders[i].isValid() && m_exitOrders[i].isActive()) {
            if (subPosId > 0) {
                if (getSubPosIdByOrd(m_exitOrders[i].getId()) == subPosId) {
                    return true;
                } else {
                    continue;
                }
            }
            return true;
        }
    }

    return false;
}

bool PositionImpl::exitFilled() const
{
    for (size_t i = 0; i < m_exitOrders.size(); i++) {
        if (!m_exitOrders[i].isValid() || !m_exitOrders[i].isFilled()) {
            return false;
        }
    }

    return true;
}

unsigned long PositionImpl::getDuration() const
{
#if 0
    if (!m_closed) {
        return (getLastDateTime() - m_entryDateTime).ticks();
    } else {
        return (m_exitDateTime - m_entryDateTime).ticks();
    }
#else
    return m_duration;
#endif
}

double PositionImpl::getHighestPrice() const
{
    return m_histHighestPrice;
}

double PositionImpl::getLowestPrice() const
{
    return m_histLowestPrice;
}

double PositionImpl::getEntryTriggerPrice() const
{
    return m_entryTriggeredPrice;
}

double PositionImpl::getExitTriggerPrice() const
{
    return m_exitTriggeredPrice;
}

const vector<Order>& PositionImpl::getEntryOrders() const
{
    return m_entryOrders;
}

const vector<Order>& PositionImpl::getExitOrders() const
{
    return m_exitOrders;
}

const string& PositionImpl::getInstrument() const
{
    return m_instrument;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void PositionImpl::close(int signalType, int subPosId, double price, bool immediately)
{
    int shares = 0;
    if (subPosId > 0) {
        auto it = m_subPosList.begin();
        for (; it != m_subPosList.end(); it++) {
            if (it->entryId == subPosId) {
                shares = it->currShares;
                // Place a market order to close position.
                if (shares != 0) {
                    if (!immediately) {
                        exit(signalType, shares, 0, 0, subPosId);
                    } else {
                        exitImmediately(signalType, shares, price, 0, subPosId);
                    }
                }
                return;
            }
        }
    } else {
        shares = getShares();
        if (shares != 0) {
            if (!immediately) {
                exit(signalType, shares, 0, 0, subPosId);
            } else {
                exitImmediately(signalType, shares, price, 0, subPosId);
            }
        }
    }
}

void PositionImpl::placeStopOrder(int signalType, int quantity, double stopPrice, double limitPrice, int closePosId, bool immediately)
{
    // Position can be increased or reduced many times, but it can be exited only once.
    assert(!exitActive(closePosId));

    Order exitOrder = buildExitOrder(quantity, stopPrice, limitPrice);
    if (stopPrice > 0) {
        exitOrder.setTriggerPrice(stopPrice);
    } else {
        exitOrder.setTriggerPrice(limitPrice);
    }

    exitOrder.setGoodTillCanceled(true);
    exitOrder.setAllOrNone(m_allOrNone);
    // Execute immediately
    if (immediately) {
        exitOrder.setExecTiming(Order::ExecTiming::IntraBar);
    }

    if (closePosId > 0) {
        mapOrdIdToSubPos(closePosId, exitOrder.getId());
    }

    m_exitOrders.push_back(exitOrder);

    placeAndRegisterOrder(exitOrder);

    return;
}

void PositionImpl::onBarEvent(const Bar& bar)
{
    m_duration++;

    double high = bar.getHigh();
    if (high > m_histHighestPrice) {
        m_histHighestPrice = high;
    }

    double low = bar.getLow();
    if (low < m_histLowestPrice) {
        m_histLowestPrice = low;
    }

    for (auto& subpos : m_subPosList) {
        // Sub-position is still alive.
        if (subpos.currShares != 0) {
            if (high > subpos.histHighestPrice) {
                subpos.histHighestPrice = high;
            }

            if (low < subpos.histLowestPrice) {
                subpos.histLowestPrice = low;
            }

            subpos.duration++;
        }
    }

    for (size_t i = 0; i < m_stopConditions.size(); i++) {
        checkStopCondition(bar, m_stopConditions[i]);
    }

    m_lastBar = bar;
}

double PositionImpl::roundUp(double price)
{
    // price precision is (1/1000000)
    int magnification = 1000000;
    const Contract& contract = m_runtime->getContract();
    long long unit = (long long)(contract.tickSize * magnification);
    if (unit == 0) {
        return price;
    }

    long long result = (long long)(price * magnification);

    if (result % unit) {
        result = (result / unit) * unit + unit;
    }

    return (result / (double)magnification);
}

double PositionImpl::roundDown(double price)
{
    // price precision is (1/1000000)
    int magnification = 1000000;
    const Contract& contract = m_runtime->getContract();
    long long unit = (long long)(contract.tickSize * magnification);
    if (unit == 0) {
        return price;
    }

    long long result = (long long)(price * magnification);

    result = (result / unit) * unit;

    return (result / (double)magnification);
}

bool PositionImpl::checkStopLoss(const Bar& bar, StopCondition& condition)
{
    if (!condition.active) {
        return false;
    }

    if (condition.shares == 0) {
        return false;
    }

    double avgFillPrice = condition.avgFillPrice;

    double stopLossPrice;
    if (condition.drawdownCalcMethod == CalcMethod::Percentage) {
        if (condition.shares > 0) {
            stopLossPrice = avgFillPrice * (1 - condition.lossPct);
        } else {
            stopLossPrice = avgFillPrice * (1 + condition.lossPct);
        }
    } else if (condition.drawdownCalcMethod == CalcMethod::Fixed) {
        if (condition.shares > 0) {
            stopLossPrice = avgFillPrice - condition.lossAmount / m_multiplier;
        } else {
            stopLossPrice = avgFillPrice + condition.lossAmount / m_multiplier;
        }
    }

    double low  = (bar.getResolution() == Bar::TICK ? bar.getClose() : bar.getLow());
    double high = (bar.getResolution() == Bar::TICK ? bar.getClose() : bar.getHigh());

    if (condition.shares > 0) {
        if (low < stopLossPrice || fabs(low - stopLossPrice) < 0.0000001) {
            if (condition.stopImmediately) {
                double stopPrice = Utils::roundFloorMultiple<double>(
                    stopLossPrice + 0.0000001, 
                    m_runtime->getContract().tickSize);
                // Guarantee the stop price can be triggered.
                if (stopPrice < low) {
                    stopPrice = low;
                }

                exitImmediately(SignalType::StopLoss, condition.shares, stopPrice, 0, condition.posId);
            } else {
                close(SignalType::StopLoss, condition.posId, 0, false);
            }
            return true;
        }
    } else if (condition.shares < 0) {
        if (high > stopLossPrice || fabs(high - stopLossPrice) < 0.0000001) {
            if (condition.stopImmediately) {
                double stopPrice = Utils::roundCeilMultiple<double>(
                    stopLossPrice - 0.0000001, 
                    m_runtime->getContract().tickSize);
                // Guarantee the stop price can be triggered.
                if (stopPrice > high) {
                    stopPrice = high;
                }

                exitImmediately(SignalType::StopLoss, condition.shares, stopPrice, 0, condition.posId);
            } else {
                close(SignalType::StopLoss, condition.posId, 0, false);
            }
            return true;
        }
    }

    return false;
}

bool PositionImpl::checkProfitTarget(const Bar& bar, StopCondition& condition)
{
    if (condition.shares == 0) {
        return false;
    }

    double returns = 0;
    double avgFillPrice = condition.avgFillPrice;

    double currLow  = (bar.getResolution() == Bar::TICK ? bar.getClose() : bar.getLow());
    double currHigh = (bar.getResolution() == Bar::TICK ? bar.getClose() : bar.getHigh());

    if (condition.shares > 0) {
        returns = (currHigh - avgFillPrice) / avgFillPrice;
    } else if (condition.shares < 0) {
        returns = (avgFillPrice - currLow) / avgFillPrice;
    }

    // Check first stop profit condition.
    if (returns > condition.profitLevels[0].returns ||
        fabs(returns - condition.profitLevels[0].returns) < 0.0000001) {
        condition.profitLevels[0].triggered = true;
        if (condition.shares > 0) {
            if (condition.stopImmediately) {
                double limitPrice = condition.avgFillPrice * (1 + condition.profitLevels[0].returns);

                limitPrice = Utils::roundFloorMultiple<double>(
                    limitPrice + 0.0000001,
                    m_runtime->getContract().tickSize);

                exitImmediately(SignalType::TakeProfit, condition.shares, 0, limitPrice, condition.posId);

                return true;
            } else {
                close(SignalType::TakeProfit, condition.posId, 0, false);
                return true;
            }
        } else if (condition.shares < 0) {
            if (condition.stopImmediately) {
                double limitPrice = condition.avgFillPrice * (1 - condition.profitLevels[0].returns);
                limitPrice = Utils::roundCeilMultiple<double>(
                    limitPrice - 0.0000001,
                    m_runtime->getContract().tickSize);

                exitImmediately(SignalType::TakeProfit, condition.shares, 0, limitPrice, condition.posId);

                return true;
            } else {
                close(SignalType::TakeProfit, condition.posId, 0, false);
                return true;
            }
        }
    }

    return false;
}

bool PositionImpl::checkProfitDynamicDrawDown(const Bar& bar, StopCondition& condition)
{
    if (!m_lastBar.isValid()) {
        return false;
    }

    if (condition.shares == 0) {
        return false;
    }

    double drawdown = 0;
    double returns = 0;
    double avgFillPrice = condition.avgFillPrice;

    double lastLow  = (bar.getResolution() == Bar::TICK ? m_lastBar.getClose() : m_lastBar.getLow());
    double lastHigh = (bar.getResolution() == Bar::TICK ? m_lastBar.getClose() : m_lastBar.getHigh());
    double currLow  = (bar.getResolution() == Bar::TICK ? bar.getClose() : bar.getLow());
    double currHigh = (bar.getResolution() == Bar::TICK ? bar.getClose() : bar.getHigh());

    condition.lowestPrice  = min(lastLow, condition.lowestPrice);
    condition.highestPrice = max(lastHigh, condition.highestPrice);

    // Use last bar's low/high price to calculate returns.
    // Use current bar's low/high price to calculate drawdown.
    if (condition.shares > 0) {
        if (condition.profitCalcMethod == CalcMethod::Percentage) {
            returns = (lastHigh - avgFillPrice) / avgFillPrice;
        } else {
            returns = (lastHigh - avgFillPrice) * m_multiplier;
        }

        if (condition.drawdownCalcMethod == CalcMethod::Percentage) {
            if (currLow < condition.highestPrice && condition.highestPrice != avgFillPrice) {
                drawdown = (condition.highestPrice - currLow) / (condition.highestPrice - avgFillPrice);
            } else {
                drawdown = 0;
            }
        } else {
            drawdown = condition.highestPrice - currLow;
        }
    } else if (condition.shares < 0) {
        if (condition.profitCalcMethod == CalcMethod::Percentage) {
            returns = (avgFillPrice - lastLow) / avgFillPrice;
        } else {
            returns = (avgFillPrice - lastLow) * m_multiplier;
        }

        if (condition.drawdownCalcMethod == CalcMethod::Percentage) {
            if (currHigh > condition.lowestPrice && condition.lowestPrice != avgFillPrice) {
                drawdown = (currHigh - condition.lowestPrice) / (avgFillPrice - condition.lowestPrice);
            } else {
                drawdown = 0;
            }
        } else {
            drawdown = currHigh - condition.lowestPrice;
        }
    }

    // Stop profit levels sorted in descending order.
    for (int i = condition.profitLevels.size() - 1; i >= 0; i--) {
        if (returns > condition.profitLevels[i].returns ||
            fabs(returns - condition.profitLevels[i].returns) < 0.0000001) {
            condition.profitLevels[i].triggered = true;
        }

        if (condition.profitLevels[i].triggered) {
            bool stopTriggered = false;
            if (condition.drawdownCalcMethod == CalcMethod::Percentage) {
                if (drawdown > (1 - condition.profitLevels[i].drawdown) ||
                    fabs(drawdown + condition.profitLevels[i].drawdown - 1) < 0.0000001) {
                    stopTriggered = true;
                }
            } else if (condition.drawdownCalcMethod == CalcMethod::Fixed) {
                if (drawdown > condition.profitLevels[i].drawdown) {
                    stopTriggered = true;
                }
            }

            if (!stopTriggered) {
                continue;
            }

            if (condition.shares > 0) {
                if (condition.stopImmediately) {
                    double stopPrice = 0;
                    if (condition.drawdownCalcMethod == CalcMethod::Percentage) {
                        stopPrice = condition.highestPrice - (condition.highestPrice - avgFillPrice) * (1 - condition.profitLevels[i].drawdown);
                    } else {
                        stopPrice = condition.highestPrice - condition.profitLevels[i].drawdown;
                    }

                    stopPrice = Utils::roundFloorMultiple<double>(
                        stopPrice + 0.0000001, 
                        m_runtime->getContract().tickSize);
                    // Guarantee the stop price can be triggered.
                    if (stopPrice < currLow) {
                        stopPrice = currLow;
                    }
                    exitImmediately(SignalType::TakeProfit, condition.shares, stopPrice, 0, condition.posId);
                    return true;
                } else {
                    close(SignalType::TakeProfit, condition.posId, 0, false);
                    return true;
                }
            } else if (condition.shares < 0) {
                if (condition.stopImmediately) {
                    double stopPrice = 0;
                    if (condition.drawdownCalcMethod == CalcMethod::Percentage) {
                        stopPrice = condition.lowestPrice + (avgFillPrice - condition.lowestPrice) * (1 - condition.profitLevels[i].drawdown);
                    } else {
                        stopPrice = condition.lowestPrice + condition.profitLevels[i].drawdown;
                    }

                    stopPrice = Utils::roundCeilMultiple<double>(
                        stopPrice - 0.0000001, 
                        m_runtime->getContract().tickSize);
                    // Guarantee the stop price can be triggered.
                    if (stopPrice > currHigh) {
                        stopPrice = currHigh;
                    }
                    exitImmediately(SignalType::TakeProfit, condition.shares, stopPrice, 0, condition.posId);
                    return true;
                } else {
                    close(SignalType::TakeProfit, condition.posId, 0, false);
                    return true;
                }
            }
        }
    }

    return false;
}

void PositionImpl::checkStopCondition(const Bar& bar, StopCondition& condition)
{
    if (!condition.active) {
        return;
    }

    if (condition.type != StopType::StopLoss &&
        condition.type != StopType::StopProfit) {
        return;
    }

    if (condition.shares == 0) {
        return;
    }

    if (!bar.isValid() || 
        m_instrument != bar.getInstrument()) {
        return;
    }

    // Check stop loss first.
    if (condition.type == StopType::StopLoss) {
        checkStopLoss(bar, condition);
        return;
    } else if (condition.type == StopType::StopProfit) {
        if (condition.profitLevels.size() == 0) {
            checkProfitTarget(bar, condition);
        } else {
            checkProfitDynamicDrawDown(bar, condition);
        }
    }
}

void PositionImpl::updateOrderList(vector<Order>& list, const Order& order)
{
    unsigned long id = order.getId();
    size_t i = 0;
    for (i = 0; i < list.size(); i++) {
        if (id == list[i].getId()) {
            list[i] = order;
            break;
        }
    }

    if (i == list.size()) {
        list.push_back(order);
    }
}

void PositionImpl::onOrderEvent(const OrderEvent& orderEvent, vector<Position::Variation>& variations)
{
    int type = orderEvent.getEventType();

    ASSERT(type == OrderEvent::FILLED || type == OrderEvent::PARTIALLY_FILLED, 
          "Only accept trade event.");

    const Order& order   = orderEvent.getOrder();
    Order::Action action = order.getAction();

    // Update state of orders which belongs to this position
    updateOrderList(m_allOrders, order);

    if (order.isOpen()) {
        updateOrderList(m_entryOrders, order);
    } else {
        updateOrderList(m_exitOrders, order);
    }

    // Create/Update sub-position and gather position variations.
    createOrUpdateSubPos(orderEvent, variations);

    // Update finally position state.
    update(orderEvent);
}

double PositionImpl::getAvgEntryPrice() const
{
    return m_avgFillPrice;
}

double PositionImpl::getAvgExitPrice() const
{
    size_t i = 0;
    double sum = 0;
    int quantity = 0;
    for (i = 0; i < m_exitOrders.size(); i++) {
        sum += (m_exitOrders[i].getAvgFillPrice() * m_exitOrders[i].getFilled());
        quantity += m_exitOrders[i].getQuantity();
    }

    if (quantity == 0) {
        return 0;
    } else {
        return sum / quantity;
    }
}

void PositionImpl::printPositionDetail()
{
    if (m_shares == 0) {
        return;
    }

    std::cout << "\n\nPositionImpl: " << m_instrument << "\n";
    for (const auto& subpos : m_subPosList) {
        if (subpos.currShares != 0) {
            std::cout << "Quantity: " << subpos.currShares << ", Price: " << subpos.entryPrice << std::endl;
        }
    }
    std::cout << "\n\n";
}

void PositionImpl::buy(const Order& order, const OrderExecutionInfo& execInfo, vector<Position::Variation>& variations)
{
    int id = order.getId();
    bool createNewSubPos = true;
    // Update existed sub-position entry.
    for (auto& subpos : m_subPosList) {
        if (subpos.entryId == id) {
            ASSERT(false, "Not Implemented.")
            createNewSubPos = false;
            if (!subpos.entryDateTime.isValid()) {
                subpos.entryDateTime = order.getAcceptedDateTime();
            }
            subpos.currShares += execInfo.getQuantity();
            subpos.entryShares += execInfo.getQuantity();
            // TODO: calculate average filled price
            subpos.entryPrice = execInfo.getPrice();
            break;
        }
    }

    // Create a new sub-position entry.
    if (createNewSubPos) {
        SubPosItem pos;
        memset(&pos, 0, sizeof(pos));
        pos.entryId = order.getId();
        pos.entryDateTime = order.getAcceptedDateTime();
        pos.entryTrigger = order.getTriggerPrice();
        pos.entryPrice = execInfo.getPrice();
        pos.entryShares = execInfo.getQuantity();
        pos.commissions = execInfo.getCommission();
        pos.slippages = execInfo.getSlippage();
        pos.currShares = execInfo.getQuantity();
        pos.exitDateTime.markInvalid();
        pos.exitId = 0;
        pos.exitPrice = 0;
        pos.histHighestPrice = pos.entryPrice;
        pos.histLowestPrice = pos.entryPrice;
        pos.duration = 0;
        pos.entryType = createNewSubPos ? SignalType::EntryLong : SignalType::IncreaseLong;

        m_subPosList.push_back(pos);

        Position::Variation variation = { 0 };
        variation.action = (pos.currShares > 0) ? (createNewSubPos ? Position::Action::EntryLong : Position::Action::IncreaseLong)
            : (createNewSubPos ? Position::Action::EntryShort : Position::Action::IncreaseShort);

        variation.id = pos.entryId;
        variation.datetime = order.getAcceptedDateTime();
        variation.currShares = pos.currShares;
        variation.price = pos.entryPrice;
        variation.commissions = pos.commissions;
        variation.slippages = pos.slippages;
        variations.push_back(variation);
    }
}

void PositionImpl::sell(const Order& order, const OrderExecutionInfo& execInfo, vector<Position::Variation>& variations)
{
    int id = order.getId();
    // Cut down sub-position entries.
    int quantity = execInfo.getQuantity();
    double price = execInfo.getPrice();
    double commissions = execInfo.getCommission();
    double slippages = execInfo.getSlippage();

    int size = m_subPosList.size();
    int total = quantity;
    int posId = getSubPosIdByOrd(id);
    bool found = false;
    for (auto& subpos : m_subPosList) {
        if (posId > 0 && subpos.entryId != posId) {
            continue;
        }

        found = true;

        Position::Variation variation = { 0 };
        variation.origShares = subpos.currShares;

        int shares = abs(subpos.currShares);
        if (shares > 0) {
            if (total - shares >= 0) {
                total -= shares;
                subpos.exitId = id;
                subpos.exitDateTime = order.getAcceptedDateTime();
                subpos.commissions += (commissions / quantity) * shares;
                subpos.slippages += (slippages / quantity) * shares;
                subpos.exitTrigger = order.getTriggerPrice();
                subpos.exitPrice = price;
                subpos.exitType = SignalType::ExitLong;

                double pnl = 0;
                if (m_direction == Position::LongPos) {
                    pnl = (subpos.exitPrice - subpos.entryPrice);
                } else {
                    pnl = (subpos.entryPrice - subpos.exitPrice);
                }
                pnl *= (m_multiplier * shares);
                subpos.realizedPnL += pnl;
                subpos.realizedPnL -= (subpos.commissions + subpos.slippages);

                // Save transactions.
                Transaction transaction;
                transaction.instrument = m_instrument;
                transaction.entryDateTime = subpos.entryDateTime;
                transaction.entryTrigger = subpos.entryTrigger;
                transaction.entryPrice = subpos.entryPrice;
                transaction.entryType = subpos.entryType;
                transaction.exitDateTime = subpos.exitDateTime;
                transaction.exitTrigger = subpos.exitTrigger;
                transaction.exitPrice = subpos.exitPrice;
                transaction.exitType = subpos.exitType;
                transaction.shares = subpos.currShares;
                transaction.commissions = subpos.commissions;
                transaction.slippages = subpos.slippages;
                transaction.realizedPnL = subpos.realizedPnL;
                transaction.highestPrice = subpos.histHighestPrice;
                transaction.lowestPrice = subpos.histLowestPrice;
                transaction.duration = subpos.duration;
                m_transactions.push_back(transaction);

                subpos.currShares = 0;

                // disable associated stop condition.
                for (size_t i = 0; i < m_stopConditions.size(); i++) {
                    if (m_stopConditions[i].posId == posId) {
                        m_stopConditions[i].active = false;
                    }
                }

            } else {
                // Split position into multiple transactions with same entry datetime but 
                // with different exit datetime.
                ASSERT(false, "Not support asymmetric position size reducing yet.");
            }

            // Record position variation.
            variation.action = order.isBuy() ? Position::Action::ReduceShort : Position::Action::ReduceLong;
            variation.id = subpos.entryId;
            variation.datetime = order.getAcceptedDateTime();
            variation.currShares = subpos.currShares;
            variation.price = price;
            variation.commissions = subpos.commissions;
            variation.slippages = subpos.slippages;
            variation.realizedPnL = subpos.realizedPnL;
            variations.push_back(variation);
        }

        if (total <= 0) {
            break;
        }
    }

    if (posId > 0 && !found) {
        ASSERT(false, "Close unknown sub position.");
    }

    if (posId > 0) {
        unsetSubPosOrdIdMap(posId);
    }
}

void PositionImpl::sellShort(const Order& order, const OrderExecutionInfo& execInfo, vector<Position::Variation>& variations)
{
    int id = order.getId();
    bool createNewSubPos = true;
    // Update existed sub-position entry.
    for (auto& subpos : m_subPosList) {
        if (subpos.entryId == id) {
            ASSERT(false, "Not Implemented.")
                createNewSubPos = false;
            if (!subpos.entryDateTime.isValid()) {
                subpos.entryDateTime = order.getAcceptedDateTime();
            }

            subpos.currShares -= execInfo.getQuantity();
            subpos.entryShares += execInfo.getQuantity();
            // TODO: calculate average filled price
            subpos.entryPrice = execInfo.getPrice();
            break;
        }
    }

    // Create a new sub-position entry.
    if (createNewSubPos) {
        SubPosItem pos;
        memset(&pos, 0, sizeof(pos));
        pos.entryId = order.getId();
        pos.entryDateTime = order.getAcceptedDateTime();
        pos.entryTrigger = order.getTriggerPrice();
        pos.entryPrice = execInfo.getPrice();
        pos.entryShares = execInfo.getQuantity();
        pos.commissions = execInfo.getCommission();
        pos.slippages = execInfo.getSlippage();
        pos.currShares = execInfo.getQuantity();
        pos.exitDateTime.markInvalid();
        pos.exitId = 0;
        pos.exitPrice = 0;
        pos.histHighestPrice = pos.entryPrice;
        pos.histLowestPrice = pos.entryPrice;
        pos.duration = 0;

        pos.entryShares *= -1;
        pos.currShares *= -1;

        pos.entryType = createNewSubPos ? SignalType::EntryShort : SignalType::IncreaseShort;

        m_subPosList.push_back(pos);

        Position::Variation variation = { 0 };
        variation.action = (pos.currShares > 0) ? (createNewSubPos ? Position::Action::EntryLong : Position::Action::IncreaseLong)
            : (createNewSubPos ? Position::Action::EntryShort : Position::Action::IncreaseShort);

        variation.id = pos.entryId;
        variation.datetime = order.getAcceptedDateTime();
        variation.currShares = pos.currShares;
        variation.price = pos.entryPrice;
        variation.commissions = pos.commissions;
        variation.slippages = pos.slippages;
        variations.push_back(variation);
    }
}

void PositionImpl::cover(const Order& order, const OrderExecutionInfo& execInfo, vector<Position::Variation>& variations)
{
    int id = order.getId();
    // Cut down sub-position entries.
    int quantity = execInfo.getQuantity();
    double price = execInfo.getPrice();
    double commissions = execInfo.getCommission();
    double slippages = execInfo.getSlippage();

    int size = m_subPosList.size();
    int total = quantity;
    int posId = getSubPosIdByOrd(id);
    bool found = false;
    for (auto& subpos : m_subPosList) {
        if (posId > 0 && subpos.entryId != posId) {
            continue;
        }

        found = true;

        Position::Variation variation = { 0 };
        variation.origShares = subpos.currShares;

        int shares = abs(subpos.currShares);
        if (shares > 0) {
            if (total - shares >= 0) {
                total -= shares;
                subpos.exitId = id;
                subpos.exitDateTime = order.getAcceptedDateTime();
                subpos.commissions += (commissions / quantity) * shares;
                subpos.slippages += (slippages / quantity) * shares;
                subpos.exitTrigger = order.getTriggerPrice();
                subpos.exitPrice = price;
                subpos.exitType = SignalType::ExitShort;

                double pnl = 0;
                if (m_direction == Position::LongPos) {
                    pnl = (subpos.exitPrice - subpos.entryPrice);
                } else {
                    pnl = (subpos.entryPrice - subpos.exitPrice);
                }
                pnl *= (m_multiplier * shares);
                subpos.realizedPnL += pnl;
                subpos.realizedPnL -= (subpos.commissions + subpos.slippages);

                // Save transactions.
                Transaction transaction;
                transaction.instrument = m_instrument;
                transaction.entryDateTime = subpos.entryDateTime;
                transaction.entryTrigger = subpos.entryTrigger;
                transaction.entryPrice = subpos.entryPrice;
                transaction.entryType = subpos.entryType;
                transaction.exitDateTime = subpos.exitDateTime;
                transaction.exitTrigger = subpos.exitTrigger;
                transaction.exitPrice = subpos.exitPrice;
                transaction.exitType = subpos.exitType;
                transaction.shares = subpos.currShares;
                transaction.commissions = subpos.commissions;
                transaction.slippages = subpos.slippages;
                transaction.realizedPnL = subpos.realizedPnL;
                transaction.highestPrice = subpos.histHighestPrice;
                transaction.lowestPrice = subpos.histLowestPrice;
                transaction.duration = subpos.duration;
                m_transactions.push_back(transaction);

                subpos.currShares = 0;
                for(size_t i = 0; i < m_stopConditions.size(); i++) {
                    if (m_stopConditions[i].posId == posId) {
                        m_stopConditions[i].active = false;
                    }
                }

            } else {
                // Split position into multiple transactions with same entry datetime but 
                // with different exit datetime.
                ASSERT(false, "Not support asymmetric position size reducing yet.");
            }

            // Record position variation.
            variation.action = order.isBuy() ? Position::Action::ReduceShort : Position::Action::ReduceLong;
            variation.id = subpos.entryId;
            variation.datetime = order.getAcceptedDateTime();
            variation.currShares = subpos.currShares;
            variation.price = price;
            variation.commissions = subpos.commissions;
            variation.slippages = subpos.slippages;
            variation.realizedPnL = subpos.realizedPnL;
            variations.push_back(variation);
        }

        if (total <= 0) {
            break;
        }
    }

    if (posId > 0 && !found) {
        ASSERT(false, "Close unknown sub position.");
    }

    if (posId > 0) {
        unsetSubPosOrdIdMap(posId);
    }
}

void PositionImpl::createOrUpdateSubPos(const OrderEvent& orderEvent, vector<Position::Variation>& variations)
{
    const Order& order = orderEvent.getOrder();
    unsigned long id = order.getId();
    Order::Action action = order.getAction();
    const OrderExecutionInfo& execInfo = orderEvent.getExecInfo();
    switch (action) {
    case Order::Action::BUY:
        buy(order, execInfo, variations);
        break;
    case Order::Action::SELL:
        sell(order, execInfo, variations);
        break;
    case Order::Action::SELL_SHORT:
        sellShort(order, execInfo, variations);
        break;
    case Order::Action::BUY_TO_COVER:
        cover(order, execInfo, variations);
        break;
    default:
        break;
    }
}

void PositionImpl::update(const OrderEvent& orderEvent)
{
    int type = orderEvent.getEventType();
    const Order& order = orderEvent.getOrder();
    unsigned long id = order.getId();
    Order::Action action = order.getAction();
    int quantity = orderEvent.getExecInfo().getQuantity();
    double price = orderEvent.getExecInfo().getPrice();

    double cost = quantity * price * m_multiplier;

    if (action == Order::Action::BUY) {
        m_cost += cost;
        m_cash -= cost;
        m_shares += quantity;
        m_avgFillPrice = (m_cost / (m_shares * m_multiplier));
    } else if (action == Order::Action::SELL) {
        m_cash += cost;
        double profit = (price - m_avgFillPrice) * quantity * m_multiplier;
        m_netPnL += profit;
        m_shares -= quantity;

        if (m_shares == 0) {
            m_avgFillPrice = 0;
            m_cost = 0;
            m_exitType = SignalType::ExitLong;
        } else {
            m_cost = m_shares * m_avgFillPrice;
        }
    } else if (action == Order::Action::SELL_SHORT) {
        m_cost += cost;
        m_cash -= cost;
        m_shares += quantity;
        m_avgFillPrice = (m_cost / (m_shares * m_multiplier));
    } else if (action == Order::Action::BUY_TO_COVER) {
        m_cash += cost;
        double profit = (m_avgFillPrice - price) * quantity * m_multiplier;
        m_netPnL += profit;
        m_shares -= quantity;
        if (m_shares == 0) {
            m_avgFillPrice = 0;
            m_cost = 0;
            m_exitType = SignalType::ExitShort;
        } else {
            m_cost = m_avgFillPrice * m_shares;
        }
    }

    m_commissions += orderEvent.getExecInfo().getCommission();
    m_slippages += orderEvent.getExecInfo().getSlippage();
    m_cash -= (m_commissions + m_slippages);

    if (order.isOpen()) {
        m_entryShares += quantity;
    } else {
        m_exitShares += quantity;
    }

    for (size_t i = 0; i < m_stopConditions.size(); i++) {
        if (m_entryOrdId == m_stopConditions[i].posId) {
            StopCondition& cond = m_stopConditions[i];
            cond.shares = m_shares;
            if (m_direction == Position::ShortPos) {
                cond.shares *= -1;
            }

            cond.avgFillPrice = m_avgFillPrice;
        }
    }

    m_commissions += orderEvent.getExecInfo().getCommission();
    m_slippages += orderEvent.getExecInfo().getSlippage();

    if (m_shares == 0) {
        assert(type == OrderEvent::FILLED);

        m_exitTriggeredPrice = order.getTriggerPrice();

    } else {
        // Nothing to do since the entry/exit order may be completely filled or canceled after a partial fill.
    }
}

void PositionImpl::mapOrdIdToSubPos(int posId, int orderId)
{
    size_t i = 0;
    for (; i < m_subPosOrderMap.size(); i++) {
        if (!m_subPosOrderMap[i].valid) {
            m_subPosOrderMap[i].orderId = orderId;
            m_subPosOrderMap[i].posId = posId;
            m_subPosOrderMap[i].valid = true;
            break;
        }
    }

    if (i == m_subPosOrderMap.size()) {
        SubPosOrderMappingItem info;
        info.valid = true;
        info.posId = posId;
        info.orderId = orderId;
        m_subPosOrderMap.push_back(info);
    }
}

int PositionImpl::getSubPosIdByOrd(int orderId) const
{
    for (size_t i = 0; i < m_subPosOrderMap.size(); i++) {
        if (m_subPosOrderMap[i].valid && m_subPosOrderMap[i].orderId == orderId) {
            return m_subPosOrderMap[i].posId;
        }
    }

    return 0;
}

void PositionImpl::unsetSubPosOrdIdMap(int posId)
{
    for (size_t i = 0; i < m_subPosOrderMap.size(); i++) {
        if (m_subPosOrderMap[i].posId == posId) {
            m_subPosOrderMap[i].valid = false;
            return;
        }
    }
}

void PositionImpl::exit(int signalType, int quantity, double stopPrice, double limitPrice, int subPosId)
{
    if (m_shares == 0) {
        ASSERT(false, "Shares is 0.");
    }
    // Fail if a previous exit order is active.
    if (exitActive(subPosId)) {
        ASSERT(false, "Exit order is active and it should be canceled first");
    }

    // If the entry order is active, request cancellation.
    for (size_t i = 0; i < m_entryOrders.size(); i++) {
        if (m_entryOrders[i].isActive()) {
            m_runtime->cancelOrder(m_entryOrders[i].getId());
        }
    }

    placeStopOrder(signalType, quantity, stopPrice, limitPrice, subPosId, false);
}

void PositionImpl::exitImmediately(int signalType, int quantity, double stopPrice, double limitPrice, int subPosId)
{
    if (m_shares == 0) {
        ASSERT(false, "Shares is 0.");
    }

    if (exitActive(subPosId)) {
        Logger_Warn() << "[" << m_instrument << "]: Exit order is active and it should be canceled first.";
//        ASSERT(false, "Exit order is active and it should be canceled first");
        return;
    }

    for (size_t i = 0; i < m_entryOrders.size(); i++) {
        if (m_entryOrders[i].isActive()) {
            m_runtime->cancelOrder(m_entryOrders[i].getId());
        }
    }

    placeStopOrder(signalType, quantity, stopPrice, limitPrice, subPosId, true);
}

bool PositionImpl::canPlaceOrder(const Order& order)
{
    return true;
}

Order PositionImpl::buildExitOrder(int quantity, double stopPrice, double limitPrice)
{
    if (m_direction == Position::LongPos) {
        return buildExitLongOrder(abs(quantity), stopPrice, limitPrice);
    } else if (m_direction == Position::ShortPos) {
        return buildExitShortOrder(abs(quantity), stopPrice, limitPrice);
    } else {
        ASSERT(false, "Unknown position side.");
        return Order();
    }
}

Order PositionImpl::buildExitLongOrder(int quantity, double stopPrice, double limitPrice)
{
    if (limitPrice == 0 && stopPrice == 0) {
        return m_runtime->createMarketOrder(Order::Action::SELL, getInstrument().c_str(), quantity, false);
    }
    else if (limitPrice > 0 && stopPrice == 0) {
        return m_runtime->createLimitOrder(Order::Action::SELL, getInstrument().c_str(), limitPrice, quantity);
    }
    else if (limitPrice == 0 && stopPrice > 0) {
        return m_runtime->createStopOrder(Order::Action::SELL, getInstrument().c_str(), stopPrice, quantity);
    }
    else if (limitPrice > 0 && stopPrice > 0) {
        return m_runtime->createStopLimitOrder(Order::Action::SELL, getInstrument().c_str(), stopPrice, limitPrice, quantity);
    }
    else {
        assert(false);
    }

    return Order();
}

Order PositionImpl::buildExitShortOrder(int quantity, double stopPrice, double limitPrice)
{
    if (limitPrice == 0 && stopPrice == 0) {
        return m_runtime->createMarketOrder(Order::Action::BUY_TO_COVER, getInstrument().c_str(), quantity, false);
    }
    else if (limitPrice > 0 && stopPrice == 0) {
        return m_runtime->createLimitOrder(Order::Action::BUY_TO_COVER, getInstrument().c_str(), limitPrice, quantity);
    }
    else if (limitPrice == 0 && stopPrice > 0) {
        return m_runtime->createStopOrder(Order::Action::BUY_TO_COVER, getInstrument().c_str(), stopPrice, quantity);
    }
    else if (limitPrice > 0 && stopPrice > 0) {
        return m_runtime->createStopLimitOrder(Order::Action::BUY_TO_COVER, getInstrument().c_str(), stopPrice, limitPrice, quantity);
    }
    else {
        assert(false);
    }

    return Order();
}

bool PositionImpl::isOpen() const
{
    return (m_shares != 0);
}

bool PositionImpl::isLong() const
{
    return m_direction == Position::LongPos;
}

void PositionImpl::setStopLossAmount(double amount, int subPosId)
{
    StopCondition cond;
    cond.active = true;
    cond.type = StopType::StopLoss;
    cond.drawdownCalcMethod = CalcMethod::Fixed;
   
    if (subPosId == 0) {
        cond.avgFillPrice = m_avgFillPrice;
        cond.lossAmount = amount;
        cond.shares = m_shares;
        cond.posId = 0;
        if (m_direction == Position::ShortPos) {
            cond.shares *= -1;
        }
    } else {
        cond.lossAmount = amount;
        for (auto& subpos : m_subPosList) {
            if (subpos.entryId == subPosId) {
                cond.posId = subPosId;
                cond.shares = subpos.currShares;
                cond.avgFillPrice = subpos.entryPrice;
                cond.highestPrice = subpos.entryPrice;
                cond.lowestPrice = subpos.entryPrice;
            }
        }
    }

    // install stop condition.
    size_t i = 0;
    for (; i < m_stopConditions.size(); i++) {
        StopCondition& c = m_stopConditions[i];
        if (!c.active) {
            c = cond;
            return;
        }
    }

    m_stopConditions.push_back(cond);

    return;
}

void PositionImpl::setStopLossPercent(double pct, int subPosId)
{
    assert(pct > 0 && pct <= 1);

    StopCondition cond;
    cond.active = true;
    cond.type = StopType::StopLoss;
    cond.drawdownCalcMethod = CalcMethod::Percentage;

    if (subPosId == 0) {
        cond.avgFillPrice = m_avgFillPrice;
        cond.lossPct = pct;
        cond.shares = m_shares;
        cond.posId = 0;
        if (m_direction == Position::ShortPos) {
            cond.shares *= -1;
        }
    } else {
        cond.active = true;
        cond.lossPct = pct;
        for (auto& subpos : m_subPosList) {
            if (subpos.entryId == subPosId) {
                cond.posId = subPosId;
                cond.shares = subpos.currShares;
                cond.avgFillPrice = subpos.entryPrice;
                cond.highestPrice = subpos.entryPrice;
                cond.lowestPrice = subpos.entryPrice;
                cond.stopImmediately = true;
            }
        }
    }

    size_t i = 0;
    for (; i < m_stopConditions.size(); i++) {
        StopCondition& c = m_stopConditions[i];
        if (!c.active) {
            c = cond;
            return;
        }
    }

    m_stopConditions.push_back(cond);

    return;
}

void PositionImpl::setStopProfitPct(double returns, int subPosId)
{
}

void PositionImpl::setStopProfit(double price, int subPosId)
{
}

void PositionImpl::setTrailingStop(double returns, double drawdown, int subPosId)
{
    assert(drawdown > 0 && drawdown <= 1);

    drawdown = 1 - drawdown;

    StopCondition cond;
    cond.active = true;
    cond.type = StopType::StopProfit;
    cond.posId = 0;
    cond.profitCalcMethod = CalcMethod::Percentage;
    cond.drawdownCalcMethod = CalcMethod::Percentage;
    cond.avgFillPrice = m_avgFillPrice;
    cond.lowestPrice = m_avgFillPrice;
    cond.highestPrice = m_avgFillPrice;
    cond.shares = m_shares;
    if (m_direction == Position::ShortPos) {
        cond.shares *= -1;
    }

    struct ProfitLevel level;
    level.returns = returns;
    level.drawdown = drawdown;
    level.triggered = false;

    cond.profitLevels.push_back(level);

    size_t i = 0;
    for (; i < m_stopConditions.size(); i++) {
        StopCondition& c = m_stopConditions[i];
        if (!c.active) {
            c = cond;
            return;
        }
    }

    m_stopConditions.push_back(cond);
}

void PositionImpl::setPercentTrailing(double amount, double percentage)
{
    assert(percentage > 0 && percentage <= 1);

    percentage = 1 - percentage;

    StopCondition cond;
    cond.active = true;
    cond.type = StopType::StopProfit;
    cond.posId = 0;
    cond.profitCalcMethod = CalcMethod::Fixed;
    cond.drawdownCalcMethod = CalcMethod::Percentage;
    cond.avgFillPrice = m_avgFillPrice;
    cond.lowestPrice = m_avgFillPrice;
    cond.highestPrice = m_avgFillPrice;
    cond.shares = m_shares;
    if (m_direction == Position::ShortPos) {
        cond.shares *= -1;
    }

    struct ProfitLevel level;
    level.returns = amount;
    level.drawdown = percentage;
    level.triggered = false;

    cond.profitLevels.push_back(level);

    size_t i = 0;
    for (; i < m_stopConditions.size(); i++) {
        StopCondition& c = m_stopConditions[i];
        if (!c.active) {
            c = cond;
            return;
        }
    }

    m_stopConditions.push_back(cond);
}

} // namespace xBacktest