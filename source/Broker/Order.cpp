#include "Utils.h"
#include "Order.h"
#include "Backtesting.h"

namespace xBacktest
{

OrderExecutionInfo::OrderExecutionInfo()
{
    m_price      = 0;
    m_quantity   = 0;
    m_commission = 0;
    m_slippage   = 0;
}

OrderExecutionInfo::OrderExecutionInfo(
    const DateTime& dt, 
    double price, 
    int quantity, 
    double commission, 
    double slippage)
{
    m_dateTime   = dt;
    m_price      = price;
    m_quantity   = quantity;
    m_commission = commission;
    m_slippage   = slippage;
}

OrderExecutionInfo::OrderExecutionInfo(const OrderExecutionInfo& execInfo)
{
    m_dateTime   = execInfo.m_dateTime;
    m_price      = execInfo.m_price;
    m_quantity   = execInfo.m_quantity;
    m_commission = execInfo.m_commission;
    m_slippage   = execInfo.m_slippage;
}

double OrderExecutionInfo::getPrice() const
{
    return m_price;
}

int OrderExecutionInfo::getQuantity() const
{
    return m_quantity;
}

double OrderExecutionInfo::getCommission() const
{
    return m_commission;
}

double OrderExecutionInfo::getSlippage() const
{
    return m_slippage;
}

const DateTime& OrderExecutionInfo::getDateTime() const
{
    return m_dateTime;
}

////////////////////////////////////////////////////////////////////////////////
Order::Order()
{
    m_id               = 0;
    m_quantity         = 0;
    m_filled           = 0;
    m_avgFillPrice     = 0;
    m_goodTillCanceled = false;
    m_commissions      = 0;
    m_slippages        = 0;
    m_allOrNone        = false;
    m_state            = State::INITIAL;
    m_limitPrice       = 0;
    m_stopPrice        = 0;
    m_tag              = 0;
    m_onClose          = true;
    m_stopHit          = false;
    m_execTiming       = ExecTiming::NextBar;
    m_triggerPrice     = 0.0;
    m_isValid          = false;
}

Order::Order(unsigned long orderId, Type type, Action action, const string& instrument, int quantity)
{
    REQUIRE(!instrument.empty(), "Instrument is empty.");

    if (quantity <= 0) {
        char errMsg[128];
        sprintf(errMsg, "Invalid quantity. [%d, %s, %s, %d]", orderId, action.toString().c_str(), instrument.c_str(), quantity);
        ASSERT(false, errMsg);
    }

    m_id = orderId;
    m_type = type;
    m_action = action;
    m_instrument = instrument;
    m_quantity = quantity;
    m_filled = 0;
    m_avgFillPrice = 0;
    m_goodTillCanceled = false;
    m_commissions = 0;
    m_slippages = 0;
    m_allOrNone = false;
    m_state = State::INITIAL;
    m_limitPrice = 0;
    m_stopPrice = 0;
    m_tag = 0;
    // Set to true when the limit order is activated (stop price is hit)
    m_stopHit = false;

    // Set to IntraBar when this order must be executed immediately on current bar
    m_execTiming = ExecTiming::NextBar;

    m_triggerPrice = 0.0;

    m_isValid = true;
}

Order::~Order()
{

}

unsigned long Order::getId() const
{
	return m_id;
}

Order::Type Order::getType() const
{
    return m_type;
}

Order::Action Order::getAction() const
{
    return m_action;
}

Order::State Order::getState() const
{
    return m_state;
}

bool Order::isActive() const
{
    return m_state != Order::State::CANCELED &&
        m_state != Order::State::REJECTED && 
        m_state != Order::State::FILLED;
}

bool Order::isInitial() const
{
    return m_state == Order::State::INITIAL;
}

bool Order::isSubmitted() const
{
    return m_state == Order::State::SUBMITTED;
}

bool Order::isAccepted() const
{
    return m_state == Order::State::ACCEPTED;
}

bool Order::isCanceled() const
{
    return m_state == Order::State::CANCELED;
}

bool Order::isPartiallyFilled() const
{
    return m_state == Order::State::PARTIALLY_FILLED;
}

bool Order::isFilled() const
{
    return m_state == Order::State::FILLED;
}

const string& Order::getInstrument() const
{
    return m_instrument;
}

int Order::getQuantity() const
{
    return m_quantity;
}

int Order::getFilled() const
{
    return m_filled;
}

int Order::getRemaining() const
{
    return m_quantity - m_filled;
}

double Order::getAvgFillPrice() const
{
    return m_avgFillPrice;
}

bool Order::getGoodTillCanceled() const
{
    return m_goodTillCanceled;
}

void Order::setGoodTillCanceled(bool goodTillCanceled)
{
    if (m_state != State::INITIAL) {
        throw std::runtime_error("The order has already been submitted.");
    }
    m_goodTillCanceled = goodTillCanceled;
}

void Order::setAllOrNone(bool allOrNone)
{
    if (m_state != State::INITIAL) {
        throw std::runtime_error("The order has already been submitted.");
    }
    m_allOrNone = allOrNone;
}

bool Order::getAllOrNone() const
{
    return m_allOrNone;
}

void Order::addExecutionInfo(const OrderExecutionInfo& execInfo)
{
    if (execInfo.getQuantity() > getRemaining()) {
        throw std::runtime_error("Invalid fill size.");
    }

    if (m_avgFillPrice == 0) {
        m_avgFillPrice = execInfo.getPrice();
    } else {
        m_avgFillPrice = (m_avgFillPrice * m_filled + execInfo.getPrice() * execInfo.getQuantity()) / float(m_filled + execInfo.getQuantity());
    }

    m_executionInfo = execInfo;
    m_filled += execInfo.getQuantity();
    m_commissions += execInfo.getCommission();
    m_slippages += execInfo.getSlippage();

    if (getRemaining() == 0) {
        switchState(Order::State::FILLED);
    } else {
        assert(!m_allOrNone);
        switchState(Order::State::PARTIALLY_FILLED);
    }
}

void Order::switchState(State newState)
{
    bool valid = false;
    switch (m_state) {
    case State::INITIAL:
        if (newState == State::SUBMITTED || 
            newState == State::CANCELED) {
            valid = true;
        }
        break;

    case State::SUBMITTED:
        if (newState == State::ACCEPTED || 
            newState == State::CANCELED ||
            newState == State::REJECTED) {
            valid = true;
        }
        break;
    case State::ACCEPTED:
        if (newState == State::PARTIALLY_FILLED || 
            newState == State::FILLED || 
            newState == State::CANCELED ||
            newState == State::REJECTED) {
            valid = true;
        }
        break;

    case State::PARTIALLY_FILLED:
        if (newState == State::PARTIALLY_FILLED || 
            newState == State::FILLED || 
            newState == State::CANCELED ||
            newState == State::REJECTED) {
            valid = true;
        }
        break;
    }

    if (!valid) {
        char errMsg[64];
        sprintf(errMsg, "Invalid order state transition [%d].", getId());
        ASSERT(false, errMsg);
    } else {
        m_state = newState;
    }
}

void Order::setState(State newState)
{
    m_state = newState;
}

const OrderExecutionInfo& Order::getExecutionInfo() const
{
    return m_executionInfo;
}

bool Order::isBuy() const
{
    return (m_action == Action::BUY || m_action == Action::BUY_TO_COVER);
}

bool Order::isSell() const
{
    return m_action == Action::SELL || m_action == Action::SELL_SHORT;
}

bool Order::isOpen() const
{
    return (m_action == Action::BUY || m_action == Action::SELL_SHORT);
}

bool Order::isClose() const
{
    return (m_action == Action::SELL || m_action == Action::BUY_TO_COVER);
}

bool Order::isValid() const
{
    return m_isValid;
}

void Order::setSubmittedDateTime(const DateTime& dateTime)
{
    m_submitted = dateTime;
}

const DateTime& Order::getSubmittedDateTime() const
{
    return m_submitted;
}

void Order::setAcceptedDateTime(const DateTime& dateTime)
{
    m_accepted = dateTime;
}

const DateTime& Order::getAcceptedDateTime() const
{
    return m_accepted;
}

bool Order::getFillOnClose() const
{
    return m_onClose;
}

void Order::setFillOnClose(bool onClose)
{
    m_onClose = onClose;
}

double Order::getLimitPrice() const
{
    return m_limitPrice;
}

void Order::setLimitPrice(double price)
{
    m_limitPrice = price;
}

double Order::getStopPrice() const
{
    return m_stopPrice;
}

void Order::setStopPrice(double price)
{
    m_stopPrice = price;
}

double Order::getTriggerPrice() const
{
    return m_triggerPrice;
}

void Order::setTriggerPrice(double price)
{
    m_triggerPrice = price;
}

const char* Order::getSignalName() const
{
    return m_signalName;
}

void Order::setSignalName(const char* signal)
{
    strncpy(m_signalName, signal, sizeof(m_signalName));
}

void Order::setStopHit(bool stopHit)
{
    m_stopHit = stopHit;
}

bool Order::getStopHit() const
{
    return m_stopHit;
}

void Order::setExecTiming(int timing)
{
    m_execTiming = timing;
}

int Order::getExecTiming() const
{
    return m_execTiming;
}

void Order::setTag(int tag)
{
    m_tag = tag;
}

int Order::getTag() const
{
    return m_tag;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
OrderEvent::OrderEvent(const Order& order, int eventType)
    : m_order(order)
    , m_eventType(eventType)
{
}

const Order& OrderEvent::getOrder() const
{
    return m_order;
}

int OrderEvent::getEventType() const
{
    return m_eventType;
}

const string& OrderEvent::getEventInfo() const
{
    return m_eventInfo;
}

void OrderEvent::setEventInfo(const string& eventInfo)
{
    m_eventInfo = eventInfo;
}

const OrderExecutionInfo& OrderEvent::getExecInfo() const
{
    return m_execInfo;
}

void OrderEvent::setExecInfo(const OrderExecutionInfo& execInfo)
{
    m_execInfo = execInfo;
}

} // namespace xBacktest