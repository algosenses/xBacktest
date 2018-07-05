#include <iostream>
#include "Utils.h"
#include "Trades.h"
#include "Backtesting.h"
#include "Runtime.h"
#include "Logger.h"

namespace xBacktest
{

Trades::Trades()
{
    m_totalNetProfits = 0.0;
    m_totalTradedVolume = 0;
    m_totalClosedVolume = 0;
    m_totalRealizedProfit = 0;
    m_totalTradeNum = 0;
    m_evenTrades = 0;
    m_broker = nullptr;
    m_allClosedTransactions.clear();
    m_lastClosedPosNum = 0;
    m_lastTradingDay.markInvalid();
    m_lastTradingDateTime.markInvalid();
    m_tradingDayNum = 0;
}

Trades::~Trades()
{
    for (map<string, PositionTracker*>::iterator it = m_posTrackers.begin();
        it != m_posTrackers.end();
        it++) {
        if (it->second != nullptr) {
            delete it->second;
        }
    }

    m_posTrackers.clear();
}

void Trades::attached(BacktestingBroker& broker)
{
    m_broker = &broker;

    broker.getOrderUpdatedEvent().subscribe(this);
    broker.getNewTradingDayEvent().subscribe(this);
}

const int Trades::getCount() const
{
    return m_all.size();
}

int Trades::getProfitableCount() const 
{
    return m_profits.size();
}

int Trades::getUnprofitableCount() const
{
    return m_losses.size();
}

int Trades::getEvenCount() const
{
    return m_evenTrades;
}

double Trades::getTotalNetProfits() const
{
    return m_totalNetProfits;
}

double Trades::getTotalTradeCost() const
{
    return m_totalTradeCost;
}

const vector<DailyMetrics>& Trades::getAllDailyMetrics()
{
    // Drain all closed position records.
    if (m_lastClosedPosNum < m_allClosedTransactions.size()) {
        updateDailyMetrics();
        m_lastClosedPosNum = m_allClosedTransactions.size();
    }

    return m_allDailyMetrics;
}

const vector<Trades::TradeProfit>& Trades::getAll() const
{
    return m_all;
}

const vector<Trades::TradeProfit>& Trades::getProfits() const
{
    return m_profits;
}

const vector<double>& Trades::getLosses() const
{
    return m_losses;
}

void Trades::getAllTradeRecords(vector<TradingRecord>& trades) const
{
    map<string, PositionTracker*>::const_iterator itor = m_posTrackers.begin();
    while (itor != m_posTrackers.end()) {
        const vector<TradingRecord>& tradeRecords = itor->second->getAllTrades();
        for (size_t i = 0; i < tradeRecords.size(); i++) {
            trades.push_back(tradeRecords[i]);
        }
        itor++;
    }
}

void Trades::updateTrades(const DateTime& dt, PositionTracker& posTracker)
{
    double netProfit = posTracker.getNetProfit();
    double netReturn = posTracker.getReturn();

    TradeProfit profit;
    profit.datetime = dt;

    double commissons = posTracker.getCommissions();
    double slippage = posTracker.getSlippages();

    if (netProfit > 0) {
        profit.value = netProfit;
        m_profits.push_back(profit);
        m_positiveReturns.push_back(netReturn);
        m_profitableCommissions.push_back(commissons);
    } else if (netProfit < 0) {
        m_losses.push_back(netProfit);
        m_negativeReturns.push_back(netReturn);
        m_unprofitableCommissions.push_back(commissons);
    } else {
        m_evenTrades += 1;
        m_evenCommissions.push_back(commissons);
    }

    profit.value = netProfit;
    m_all.push_back(profit);
    m_allReturns.push_back(netReturn);

    m_totalNetProfits += netProfit;
    m_totalRealizedProfit += netProfit;
}

void Trades::updatePosTracker(PositionTracker& posTracker, 
                              const DateTime& datetime,
                              Order::Action action,
                              double price, 
                              int    quantity,
                              double commission,
                              double slippage)
{
    switch (action) {
    case Order::Action::BUY:
        posTracker.buy(datetime, quantity, price, commission, slippage);
        break;
        
    case Order::Action::SELL:
        posTracker.sell(datetime, quantity, price, commission, slippage);
        break;

    case Order::Action::SELL_SHORT:
        posTracker.sellShort(datetime, quantity, price, commission, slippage);
        break;

    case Order::Action::BUY_TO_COVER:
        posTracker.cover(datetime, quantity, price, commission, slippage);
        break;

    default:
        break;
    }

    if (action == Order::Action::SELL || action == Order::Action::BUY_TO_COVER) {
        updateTrades(datetime, posTracker);
    } 

    m_totalTradeCost += (commission + slippage);
    m_totalRealizedProfit -= (commission + slippage);
}

void Trades::onOrderUpdated(const OrderEvent& orderEvent)
{
    int type = orderEvent.getEventType();
    // Only interested in filled or partially filled orders.
    if (type != OrderEvent::FILLED && type != OrderEvent::PARTIALLY_FILLED) {
        return;
    }

    const Order& order = orderEvent.getOrder();
    const string& instrument = order.getInstrument();
    Order::Action action = order.getAction();

    REQUIRE(action == Order::Action::BUY ||
            action == Order::Action::BUY_TO_COVER ||
            action == Order::Action::SELL || 
            action == Order::Action::SELL_SHORT, 
            "Unknown action.");

    const OrderExecutionInfo& execInfo = orderEvent.getExecInfo();
    double price = execInfo.getPrice();
    int quantity = execInfo.getQuantity();
    double commission = execInfo.getCommission();
    double slippage = execInfo.getSlippage();

    REQUIRE(quantity != 0, "Incorrect quantity.");

    m_totalTradedVolume += quantity;
    m_totalTradeNum++;
    m_lastTradingDateTime = order.getAcceptedDateTime();
    if (order.isClose()) {
        m_totalClosedVolume += quantity;
    }

    // Get or create the tracker for this instrument.
    PositionTracker *posTracker = nullptr;
    auto it = m_posTrackers.find(order.getInstrument());
    if (it == m_posTrackers.end()) {
        PositionTracker* tracker = new PositionTracker(instrument);
        const Contract& contract = m_broker->getContract(order.getInstrument());
        tracker->setMultiplier(contract.multiplier);
        it = m_posTrackers.insert(make_pair(instrument, tracker)).first;
    }

    posTracker = it->second;

    // Update the tracker for this order.
    updatePosTracker(*posTracker, execInfo.getDateTime(), action, price, quantity, commission, slippage);

    // Copy close transaction into historical records.
    if (action == Order::Action::SELL || action == Order::Action::BUY_TO_COVER) {
        ClosePosTrade item;
        posTracker->getLastClosePosTrade(item);
        m_allClosedTransactions.push_back(item);
    }
}

// Calculate daily metrics.
void Trades::updateDailyMetrics()
{
    DailyMetrics daily;
    memset(&daily, 0, sizeof(DailyMetrics));
    daily.tradingDay = m_lastTradingDay.date();

    for (const auto& item : m_posTrackers) {
        PositionTracker* tracker = item.second;
        if (tracker != nullptr) {
            if (tracker->getShares() != 0) {
                daily.todayPosition += abs(tracker->getShares());
            }
        }
    }

    if (daily.tradingDay.isValid()) {
        daily.equity = m_broker->getEquity();
        daily.cash = m_broker->getAvailableCash();
        daily.posProfit = m_broker->getPosProfit();
        int size = m_allClosedTransactions.size();
        daily.tradesNum = size - m_lastClosedPosNum;
        for (int i = 0; i < daily.tradesNum && i < size; i++) {
            daily.closedProfit += m_allClosedTransactions[size - i - 1].realizedProfit;
            daily.tradedVolume += m_allClosedTransactions[size - i - 1].tradedVolume;
        }


        if (m_tradingDayNum == 0) {
            daily.tradeCost = m_totalTradeCost;
            daily.tradedVolume = m_totalTradedVolume;
            daily.cumTradesNum = m_totalTradeNum;
            daily.realizedProfit = daily.closedProfit - daily.tradeCost;
        } else {
            daily.tradeCost = m_totalTradeCost - m_allDailyMetrics.back().cumTradeCost;
            daily.tradedVolume = m_totalTradedVolume - m_allDailyMetrics.back().cumTradedVolume;
            daily.tradesNum = m_totalTradeNum - m_allDailyMetrics.back().cumTradesNum;
            daily.realizedProfit = daily.closedProfit - daily.tradeCost;
        }

        daily.cumTradeCost = m_totalTradeCost;
        daily.cumRealizedProfit = m_totalRealizedProfit;
        daily.cumTradedVolume = m_totalTradedVolume;
        daily.cumCloseVolume = m_totalClosedVolume;
        daily.cumTradesNum = m_totalTradeNum;

        daily.margin = m_broker->getMargin();
        daily.posProfit = m_broker->getPosProfit();
        daily.profit = (daily.posProfit + daily.closedProfit);

        m_allDailyMetrics.push_back(daily);

        m_tradingDayNum++;
    }
}

void Trades::onNewTradingDay(const DateTime& lastDateTime, const DateTime& currDateTime)
{
    if (!m_lastTradingDay.isValid()) {
        m_lastTradingDay = lastDateTime;
    } else if (lastDateTime.dateNum() <= m_lastTradingDay.dateNum()) {
        return;
    }

    m_lastTradingDay = lastDateTime;

    updateDailyMetrics();

    m_lastClosedPosNum = m_allClosedTransactions.size();
}

void Trades::onEvent(int type, const DateTime& datetime, const void *context)
{
    if (type == Event::EvtOrderUpdate) {
        onOrderUpdated(*(OrderEvent*)context);
    } else if (type == Event::EvtNewTradingDay) {
        onNewTradingDay(*(DateTime*)context, datetime);
    }
}

} // namespace xBacktest