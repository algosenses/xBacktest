#include <cstddef>
#include <cassert>
#include "Condition.h"
#include "Logger.h"
#include "Errors.h"
#include "BarSeries.h"
#include "Simulator.h"
#include "Strategy.h"
#include "PositionImpl.h"
#include "Executor.h"
#include "Runtime.h"
#include "Utils.h"

namespace xBacktest
{

Runtime::Runtime(Process* process)
    : m_process(process)
{
    m_id           = 0;
    m_strategyObj  = nullptr;
    m_state        = Idle;
    m_activated    = false;
    m_subscribeAll = false;

    m_barsProcessedEvent.setType(Event::EvtBarProcessed);
    m_orders.clear();
    m_longPosList.clear();
    m_shortPosList.clear();
    m_closedPosList.clear();
}

Runtime::~Runtime()
{
    for (size_t i = 0; i < m_composers.size(); i++) {
        if (m_composers[i] != nullptr) {
            delete m_composers[i];
        }
    }
    m_composers.clear();

    if (m_strategyObj) {
        delete m_strategyObj;
        m_strategyObj = nullptr;
    }

    for (auto& pos : m_longPosList) {
        delete pos.second;
        pos.second = nullptr;
    }

    for (auto& pos : m_shortPosList) {
        delete pos.second;
        pos.second = nullptr;
    }

    for (auto& it : m_closedPosList) {
        delete it;
    }

    for (auto& series : m_barSeries) {
        if (series.second) {
            delete series.second;
            series.second = nullptr;
        }
    }

    m_longPosList.clear();
    m_shortPosList.clear();
    m_closedPosList.clear();
    m_orders.clear();
    m_barSeries.clear();
}

void Runtime::setId(unsigned long id)
{
    m_id = id;
}

unsigned long Runtime::getId() const
{
    return m_id;
}

void Runtime::setName(const char* name)
{
    REQUIRE(name && name[0] != '\0', "Runtime name invalid.");
    m_name = name;
}

const char* Runtime::getName() const
{
    return m_name.c_str();
}

bool Runtime::isActivate() const
{
    return m_activated;
}

void Runtime::addActiveDateTime(const DateTime& begin, const DateTime& end)
{
    if (!begin.isValid() || !end.isValid()) {
        return;
    }

    if (begin >= end) {
        return;
    }

    // TODO: merge periods.

    ActivePeriod period;
    period.begin = begin;
    period.end   = end;

    m_activePeriods.push_back(period);
}

void Runtime::setCash(double cash)
{
    return m_process->getExecutor()->setCash(cash);
}

double Runtime::getAvailableCash() const
{
    return m_process->getExecutor()->getAvailableCash();
}

double Runtime::getEquity() const
{
    return m_process->getExecutor()->getEquity();
}

const Bar& Runtime::getLastBar(const char* instrument) const
{
    if (instrument == nullptr || instrument[0] == '\0') {
        instrument = m_mainInstrument.c_str();
    }

    const auto& it = m_lastBars.find(instrument);
    if (it != m_lastBars.end()) {
        return it->second;
    }

    return m_lastBar;
}

double Runtime::getLastPrice(const char* instrument) const
{
    return m_lastBar.getClose();
}

double Runtime::getTickSize(const char* instrument) const
{
    return getContract(instrument).tickSize;
}

const char* Runtime::getMainInstrument() const
{
    return m_mainInstrument.c_str();
}

void Runtime::setMainInstrument(const char* instrument)
{
    REQUIRE(instrument && instrument[0] != '\0', "Main instrument invalid.");

    m_mainInstrument = instrument;
}

void Runtime::subscribeAll()
{
    m_subscribeAll = true;
}

bool Runtime::isSubscribeAll() const
{
    return m_subscribeAll;
}

const Contract& Runtime::getContract(const char* instrument) const
{
    static Contract cache;

    string instr;
    if (instrument == nullptr || instrument[0] == '\0') {
        instr = m_mainInstrument;
    } else {
        instr = instrument;
    }

    if (instr == cache.instrument) {
        return cache;
    }

    const auto& itor = m_contracts.find(m_mainInstrument);
    if (itor != m_contracts.end()) {
        cache = itor->second;

        return itor->second;
    } else {
        ASSERT(false, "Contract could not be found."); 
    }
}

BarSeries& Runtime::getBarSeries(const char* instrument)
{
    if (instrument == nullptr || instrument[0] == '\0') {
        ASSERT(m_barSeries.size() > 0, "Bar series is empty.");
        return *(m_barSeries.begin()->second);
    } else {
        const auto& itor = m_barSeries.find(instrument);
        ASSERT(itor == m_barSeries.end(), "Bar series cannot be found.");
        return *(itor->second);
    }
}

unsigned long Runtime::getNextOrderId()
{
    return m_process->getNextOrderId();
}

void Runtime::getTransactionRecords(vector<Transaction>& outRecords)
{
    TransactionList records;

    for (auto& pos : m_longPosList) {
        pos.second->getTransactionRecords(records);
    }

    for (auto& pos : m_shortPosList) {
        pos.second->getTransactionRecords(records);
    }

    for (size_t i = 0; i < records.size(); i++) {
        outRecords.push_back(records[i]);
    }
}

void Runtime::registerInstrument(const char* instrument)
{
    if (instrument == nullptr || instrument[0] == '\0') {
        return;
    }

    const auto& itor = m_barSeries.find(instrument);
    if (itor == m_barSeries.end()) {
        BarSeries* series = new BarSeries();
        m_barSeries.insert(std::make_pair(instrument, series));
    }
}

void Runtime::registerContracts(const vector<Contract>& contracts)
{
    for (auto& item : contracts) {
        m_contracts.insert(std::make_pair(item.instrument, item));
    }
}

void Runtime::setStrategyObject(Strategy* strategy)
{
    ASSERT(strategy != nullptr, "Strategy object is null.");
    m_strategyObj = strategy;
    m_strategyObj->attach(this);
}

void Runtime::setParameters(const ParamTuple& params)
{
    m_paramTuple = params;
    bool isLast = false;
    int size = params.size();
    for (size_t i = 0; i < size; i++) {
        if (i == size - 1) {
            isLast = true;
        } else {
            isLast = false;
        }

        m_strategyObj->onSetParameter(params[i].name.c_str(), params[i].type, params[i].value.c_str(), isLast);
    }
}

BacktestingBroker* Runtime::getBroker()
{
    return m_process->getExecutor()->getBroker();
}

int Runtime::loadData(const DataRequest& request)
{
    return m_process->loadData(request);
}

bool Runtime::aggregateBarSeries(const TradingSession& session,
                                int inRes, int inInterval, BarSeries& input,
                                int outRes, int outInterval, BarSeries& output)
{
    bool found = false;
    for (size_t i = 0; i < m_composers.size(); i++) {
        if (m_composers[i]->getOutputBarSeris() == &output) {
            found = true;
            break;
        }
    }

    if (!found) {
        BarComposer *composer = new BarComposer();
        composer->setCompositeParams(session, (Bar::Resolution)inRes, (Bar::Resolution)outRes, inInterval, outInterval);
        composer->compisteBarSeries(input, output);
        m_composers.push_back(composer);
        return true;
    }

    return false;
}

void Runtime::writeDebugMsg(const char* msg)
{
    return m_process->getExecutor()->writeDebugMsg(msg);
}

Order Runtime::createMarketOrder(
    Order::Action action,
    const char* instrument,
    int quantity,
    bool onClose,
    bool goodTillCanceled,
    bool allOrNone)
{
    if (quantity <= 0) {
        ASSERT(false, "Order quantity must greater than 0.");
    }

    Order order(getNextOrderId(), Order::Type::MARKET, action, instrument, quantity);
    order.setGoodTillCanceled(goodTillCanceled);
    order.setAllOrNone(allOrNone);
    order.setFillOnClose(onClose);

    return order;
}

Order Runtime::createLimitOrder(
    Order::Action action,
    const char* instrument,
    double limitPrice,
    int quantity,
    bool goodTillCanceled,
    bool allOrNone)
{
    assert(quantity > 0);

    Order order(getNextOrderId(), Order::Type::LIMIT, action, instrument, quantity);
    order.setLimitPrice(limitPrice);
    order.setGoodTillCanceled(goodTillCanceled);
    order.setAllOrNone(allOrNone);

    return order;
}

Order Runtime::createStopOrder(
    Order::Action action,
    const char* instrument,
    double stopPrice,
    int quantity,
    bool goodTillCanceled,
    bool allOrNone)
{
    assert(quantity > 0);

    Order order(getNextOrderId(), Order::Type::STOP, action, instrument, quantity);
    order.setStopPrice(stopPrice);
    order.setGoodTillCanceled(goodTillCanceled);
    order.setAllOrNone(allOrNone);

    return order;
}

Order Runtime::createStopLimitOrder(
    Order::Action action,
    const char* instrument,
    double stopPrice,
    double limitPrice,
    int quantity,
    bool goodTillCanceled,
    bool allOrNone)
{
    assert(quantity != 0);

    Order order(getNextOrderId(), Order::Type::STOP_LIMIT, action, instrument, quantity);
    order.setLimitPrice(limitPrice);
    order.setStopPrice(stopPrice);
    order.setGoodTillCanceled(goodTillCanceled);
    order.setAllOrNone(allOrNone);

    return order;
}

Order Runtime::createOrder(
    Order::Action action, 
    const char* instrument, 
    double stopPrice, 
    double limitPrice, 
    int    quantity)
{
    if (limitPrice == 0 && stopPrice == 0) {
        // onClose is true by default.
        return createMarketOrder(action, instrument, quantity, false);
    } else if (limitPrice != 0 && stopPrice == 0) {
        return createLimitOrder(action, instrument, limitPrice, quantity);
    } else if (limitPrice == 0 && stopPrice != 0) {
        return createStopOrder(action, instrument, stopPrice, quantity);
    } else if (limitPrice != 0 && stopPrice != 0) {
        return createStopLimitOrder(action, instrument, stopPrice, limitPrice, quantity);
    } else {
        assert(false);
    }

    return Order();
}

unsigned long Runtime::submitOrder(const Order& order)
{
    assert(order.isInitial());

    registerOrder(order);

    placeOrder(order);

    return order.getId();
}

void Runtime::placeOrder(const Order& order)
{
    if (m_activated) {
        return getBroker()->placeOrder(order);
    }
}

void Runtime::cancelOrder(unsigned long orderId)
{
    if (m_activated) {
        return getBroker()->cancelOrder(orderId);
    }
}

void Runtime::registerOrder(const Order& order)
{
    unsigned long id = order.getId();
    unordered_map<unsigned long, Order>::const_iterator it = m_orders.find(id);
    ASSERT(it == m_orders.end(), "Order has already existed.");
    m_orders.insert(std::make_pair(id, order));
}

void Runtime::notifyPositions(const Bar& bar)
{
    for (auto& pos : m_longPosList) {
        pos.second->getImplementor()->onBarEvent(bar);
    }

    for (auto& pos : m_shortPosList) {
        pos.second->getImplementor()->onBarEvent(bar);
    }
}

long Runtime::getPositionSize(const char* instrument) const
{
    string instrName = (instrument == nullptr || instrument[0] == '\0') ? m_mainInstrument : instrument;
    int longPos  = 0;
    int shortPos = 0;

    const auto& longIt = m_longPosList.find(instrName);
    if (longIt != m_longPosList.end()) {
        longPos = longIt->second->getShares();
    }

    const auto& shortIt = m_shortPosList.find(instrName);
    if (shortIt != m_shortPosList.end()) {
        shortPos = shortIt->second->getShares();
    }

    return (longPos - shortPos);
}

Position* Runtime::getCurrentPosition(const char* instrument, int side)
{
    if (side == Position::LongPos) {
        const auto& it = m_longPosList.find(instrument);
        if (it != m_longPosList.end()) {
            return it->second;
        }
    } else if (side == Position::ShortPos) {
        const auto& it = m_shortPosList.find(instrument);
        if (it != m_shortPosList.end()) {
            return it->second;
        }
    }

    return nullptr;
}

void Runtime::getAllPositions(Vector<Position*>& positions)
{
    for (auto& pos : m_longPosList) {
        if (pos.second->getShares() != 0) {
            positions.push_back(pos.second);
        }
    }

    for (auto& pos : m_shortPosList) {
        if (pos.second->getShares() != 0) {
            positions.push_back(pos.second);
        }
    }
}

void Runtime::getAllSubPositions(const char* instrument, SubPositionList& posList)
{
    const auto& longIt = m_longPosList.find(instrument);
    if (longIt != m_longPosList.end()) {
        SubPositionList pos;
        longIt->second->getSubPositions(pos);
        if (pos.size() > 0) {
            for (size_t i = 0; i < pos.size(); i++) {
                posList.push_back(pos[i]);
            }
        }
    }

    const auto& shortIt = m_shortPosList.find(instrument);
    if (shortIt != m_shortPosList.end()) {
        SubPositionList pos;
        shortIt->second->getSubPositions(pos);
        if (pos.size() > 0) {
            for (size_t i = 0; i < pos.size(); i++) {
                posList.push_back(pos[i]);
            }
        }
    }
}

void Runtime::closeSubPosition(int subPosId)
{
    for (auto& pos : m_longPosList) {
        SubPositionList subPos;
        pos.second->getSubPositions(subPos);
        if (subPos.size() > 0) {
            for (size_t i = 0; i < subPos.size(); i++) {
                if (subPos[i].id == subPosId) {
                    pos.second->close(subPosId);
                    return;
                }
            }
        }
    }

    for (auto& pos : m_shortPosList) {
        SubPositionList subPos;
        pos.second->getSubPositions(subPos);
        if (subPos.size() > 0) {
            for (size_t i = 0; i < subPos.size(); i++) {
                if (subPos[i].id == subPosId) {
                    pos.second->close(subPosId);
                    return;
                }
            }
        }
    }
}

void Runtime::closePosition(int posId)
{
    for (auto& pos : m_longPosList) {
        if (!pos.second->exitActive(posId) && pos.second->getId() == posId) {
            pos.second->close();
        }
    }

    for (auto& pos : m_shortPosList) {
        if (!pos.second->exitActive(posId) && pos.second->getId() == posId) {
            pos.second->close();
        }
    }
}

void Runtime::closeAllPositions()
{
    for (auto& pos : m_longPosList) {
        if (!pos.second->exitActive()) {
            pos.second->close();
        }
    }

    for (auto& pos : m_shortPosList) {
        if (!pos.second->exitActive()) {
            pos.second->close();
        }
    }
}

void Runtime::closeAllPositionsImmediately(double price)
{
    for (auto& pos : m_longPosList) {
        if (!pos.second->exitActive()) {
            pos.second->closeImmediately(0, price);
        }
    }

    for (auto& pos : m_shortPosList) {
        if (!pos.second->exitActive()) {
            pos.second->closeImmediately(0, price);
        }
    }
}

void Runtime::updateBarSeries(const Bar& bar)
{
    const string& instrument = bar.getInstrument();

    auto itor = m_barSeries.find(instrument);
    if (itor == m_barSeries.end()) {
        BarSeries* series = new BarSeries();
        itor = m_barSeries.insert(make_pair(instrument, series)).first;
    }

    itor->second->appendWithDateTime(bar.getDateTime(), &bar);
}

bool Runtime::registerParameter(const char* name, int type)
{
    if (name == nullptr || name[0] == '\0') {
        return false;
    }

    for (size_t i = 0; i < m_parameters.size(); i++) {
        if (_stricmp(m_parameters[i].name.c_str(), name) == 0) {
            return false;
        }
    }

    PItem item;
    item.name = name;
    item.type = type;
    m_parameters.push_back(item);

    return true;
}

bool Runtime::checkActive(const DateTime& datetime)
{
    for (int i = 0; i < m_activePeriods.size(); i++) {
        if (datetime >= m_activePeriods[i].begin &&
            datetime < m_activePeriods[i].end) {
            return true;
        }
    }

    return false;
}

void Runtime::inactivate()
{
    m_strategyObj->closeAllPositions();
}

void Runtime::onCreate()
{
    m_strategyObj->onCreate();
}

void Runtime::onStart()
{
    m_strategyObj->onStart();
}

void Runtime::onBarEvent(const Bar& bar)
{
    m_lastBar = bar;

    if (checkActive(bar.getDateTime())) {
        m_activated = true;
    } else {
        if (m_activated) {
            inactivate();
        }
        m_activated = false;
    }

    // THE ORDER HERE IS VERY IMPORTANT

    // 1: Save current bars.
    updateBarSeries(bar);

    // 2: Let active positions calculate profit-and-loss and place stop order.
    notifyPositions(bar);

    // 3: Let the strategy process current bar and place orders.
    m_strategyObj->onBar(bar);

    // 4: Notify that the bar was processed.
    m_barsProcessedEvent.emit(bar.getDateTime(), &bar);
}

unsigned long Runtime::buy(const char* instrument, int quantity, double price, bool immediately, const char* signal)
{
    Order order;
    if (immediately) {
        order = createOrder(Order::Action::BUY, instrument, price, 0, quantity);
        order.setExecTiming(Order::ExecTiming::IntraBar);
    } else {
        order = createOrder(Order::Action::BUY, instrument, 0, price, quantity);
    }

    if (signal && signal[0] != '\0') {
        order.setSignalName(signal);
    }

    return submitOrder(order);
}

unsigned long Runtime::sell(const char* instrument, int quantity, double price, bool immediately, const char* signal)
{
    assert(getPositionSize(instrument) >= quantity);

    Order order;
    if (immediately) {
        order = createOrder(Order::Action::SELL, instrument, price, 0, quantity);
        order.setExecTiming(Order::ExecTiming::IntraBar);
    } else {
        order = createOrder(Order::Action::SELL, instrument, 0, price, quantity);
    }

    if (signal && signal[0] != '\0') {
        order.setSignalName(signal);
    }

    return submitOrder(order);
}

unsigned long Runtime::sellShort(const char* instrument, int quantity, double price, bool immediately, const char* signal)
{
    Order order;
    if (immediately) {
        order = createOrder(Order::Action::SELL_SHORT, instrument, price, 0, quantity);
        order.setExecTiming(Order::ExecTiming::IntraBar);
    } else {
        order = createOrder(Order::Action::SELL_SHORT, instrument, 0, price, quantity);
    }

    if (signal && signal[0] != '\0') {
        order.setSignalName(signal);
    }

    return submitOrder(order);
}

unsigned long Runtime::buyToCover(const char* instrument, int quantity, double price, bool immediately, const char* signal)
{
    Order order;
    if (immediately) {
        order = createOrder(Order::Action::BUY_TO_COVER, instrument, price, 0, quantity);
        order.setExecTiming(Order::ExecTiming::IntraBar);
    } else {
        order = createOrder(Order::Action::BUY_TO_COVER, instrument, 0, price, quantity);
    }
    
    if (signal && signal[0] != '\0') {
        order.setSignalName(signal);
    }

    return submitOrder(order);
}

void Runtime::createOrUpdatePosition(const OrderEvent& evt)
{
    const Order& order = (Order&)evt.getOrder();
    const string& instrument = order.getInstrument();

    assert(!instrument.empty());

    Position* position = nullptr;
    if (order.getAction() == Order::Action::BUY || order.getAction() == Order::Action::SELL) {
        const auto& it = m_longPosList.find(order.getInstrument());
        if (it != m_longPosList.end()) {
            position = it->second;
        } else {
            position = new Position();
            position->getImplementor()->init(position, this, order);
            m_longPosList.insert(std::make_pair(order.getInstrument(), position));
        }
    } else if (order.getAction() == Order::Action::SELL_SHORT || order.getAction() == Order::Action::BUY_TO_COVER) {
        const auto& it = m_shortPosList.find(order.getInstrument());
        if (it != m_shortPosList.end()) {
            position = it->second;
        } else {
            position = new Position();
            position->getImplementor()->init(position, this, order);
            m_shortPosList.insert(std::make_pair(order.getInstrument(), position));
        }
    }

    vector<Position::Variation> variations;

    if (position != nullptr) {
        position->getImplementor()->onOrderEvent(evt, variations);
    }

    // Notify position event.
    bool closeFired = false;
    for (size_t i = 0; i < variations.size(); i++) {
        Position::Variation& variation = variations[i];
        switch (variation.action) {
        case Position::Action::EntryLong:
        case Position::Action::EntryShort:
            m_strategyObj->onPositionOpened(*position);
            return;
            break;

        case Position::Action::IncreaseLong:
        case Position::Action::IncreaseShort: {
            m_strategyObj->onPositionChanged(*position, variation);
            break;
        }
        case Position::Action::ReduceLong:
        case Position::Action::ReduceShort:
            m_strategyObj->onPositionChanged(*position, variation);
            if (!closeFired && position->getShares() == 0) {
                closeFired = true;
                m_strategyObj->onPositionClosed(*position);
            }
            break;

        default:
            break;
        }
    }

    return;
}

bool Runtime::onOrderEvent(const OrderEvent* evt)
{
    assert(evt != nullptr);
    const Order& order = evt->getOrder();
    unsigned long orderId = order.getId();
    int type = evt->getEventType();
    unordered_map<unsigned long, Order>::iterator it = m_orders.find(orderId);
    if (it == m_orders.end()) {
        return false;
    }

    // Update order state
    it->second = order;

    if (type == OrderEvent::FILLED ||
        type == OrderEvent::PARTIALLY_FILLED) {
        // Create or update positions, operation result will come out.
        createOrUpdatePosition(*evt);
#if _DEBUG
//        for (auto& it = m_longPosList.begin(); it != m_longPosList.end(); it++) {
//            it->second->getImplementor()->printPositionDetail();
//        }

//        for (auto& it = m_shortPosList.begin(); it != m_shortPosList.end(); it++) {
//            it->second->getImplementor()->printPositionDetail();
//        }
#endif
    }

    // Notify order event. 
    // Only support following types of event in backtesting mode.
    Order::State state = order.getState();
    switch (state) {
    case Order::State::CANCELED:
        // Cancellation and Rejection have the same callback.
        //        m_strategyObj->onOrderCanceled(order);
        m_strategyObj->onOrderFailed(order);
        break;

    case Order::State::REJECTED:
        m_strategyObj->onOrderFailed(order);
        break;

    case Order::State::FILLED:
        m_strategyObj->onOrderFilled(order);
        break;

    case Order::State::PARTIALLY_FILLED:
        m_strategyObj->onOrderPartiallyFilled(order);
        break;

    default:
        break;
    }

    return true;
}

void Runtime::onTimeElapsed(const DateTime& prevDateTime, const DateTime& nextDateTime)
{
    m_strategyObj->onTimeElapsed(prevDateTime, nextDateTime);
}

void Runtime::onHistoricalData(const Bar& bar, bool isCompleted)
{
    m_strategyObj->onHistoricalData(bar, isCompleted);
}

void Runtime::onStop()
{
    m_strategyObj->onStop();
}

void Runtime::onDestroy()
{
    m_strategyObj->onDestroy();
}

} // namespace xBacktest