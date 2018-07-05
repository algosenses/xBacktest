#include "Strategy.h"
#include "Runtime.h"

namespace xBacktest
{

Strategy::Strategy()
{
    m_runtime = nullptr;
}

Strategy::~Strategy()
{
}

void Strategy::attach(Runtime* runtime) 
{ 
    m_runtime = runtime; 
}

unsigned long Strategy::getId() const
{
    return m_runtime->getId();
}

const char* Strategy::getMainInstrument() const
{
    return m_runtime->getMainInstrument();
}

const Contract& Strategy::getDefaultContract() const
{
    return m_runtime->getContract();
}

BarSeries& Strategy::getBarSeries(const char* instrument)
{
    return m_runtime->getBarSeries(instrument);
}

const Bar& Strategy::getLastBar(const char* instrument) const
{
    return m_runtime->getLastBar(instrument);
}

double Strategy::getLastPrice(const char* instrument) const
{
    return m_runtime->getLastPrice(instrument);
}

double Strategy::getTickSize(const char* instrument) const
{
    return m_runtime->getTickSize(instrument);
}

long Strategy::getPositionSize(const char* instrument)
{
    return m_runtime->getPositionSize(instrument);
}

Position* Strategy::getCurrentPosition(const char* instrument, int side)
{
    return m_runtime->getCurrentPosition(instrument, side);
}

void Strategy::getAllPositions(Vector<Position*>& positions)
{
    return m_runtime->getAllPositions(positions);
}

void Strategy::getAllSubPositions(const char* instrument, SubPositionList& positions)
{
    if (instrument == nullptr || instrument[0] == '\0') {
        return;
    }

    return m_runtime->getAllSubPositions(instrument, positions);
}

void Strategy::closeSubPosition(int subPosId)
{
    return m_runtime->closeSubPosition(subPosId);
}

void Strategy::closePosition(int posId)
{
    return m_runtime->closePosition(posId);
}

void Strategy::closeAllPositions()
{
    return m_runtime->closeAllPositions();
}

void Strategy::closeAllPositionsImmediately(double stopPrice)
{
    return m_runtime->closeAllPositionsImmediately(stopPrice);
}

unsigned long Strategy::buy(const char* instrument, int quantity, double price, bool immediately, const char* signal)
{
    return m_runtime->buy(instrument, quantity, price, immediately, signal);
}

unsigned long Strategy::sell(const char* instrument, int quantity, double price, bool immediately, const char* signal)
{
    return m_runtime->sell(instrument, quantity, price, immediately, signal);
}

unsigned long Strategy::sellShort(const char* instrument, int quantity, double price, bool immediately, const char* signal)
{
    return m_runtime->sellShort(instrument, quantity, price, immediately, signal);
}

unsigned long Strategy::buyToCover(const char* instrument, int quantity, double price, bool immediately, const char* signal)
{
    return m_runtime->buyToCover(instrument, quantity, price, immediately, signal);
}

Order Strategy::createMarketOrder(
    Order::Action action,
    const char* instrument,
    int quantity,
    bool onClose,
    bool goodTillCanceled,
    bool allOrNone)
{
    return m_runtime->createMarketOrder(action, instrument, quantity, onClose, goodTillCanceled, allOrNone);
}

Order Strategy::createLimitOrder(
    Order::Action action,
    const char* instrument,
    double limitPrice,
    int quantity,
    bool goodTillCanceled,
    bool allOrNone)
{
    return m_runtime->createLimitOrder(action, instrument, limitPrice, quantity, goodTillCanceled, allOrNone);
}

Order Strategy::createStopOrder(
    Order::Action action,
    const char* instrument,
    double stopPrice,
    int quantity,
    bool goodTillCanceled,
    bool allOrNone)
{
    return m_runtime->createStopOrder(action, instrument, stopPrice, quantity, goodTillCanceled, allOrNone);
}

Order Strategy::createStopLimitOrder(
    Order::Action action,
    const char* instrument,
    double stopPrice,
    double limitPrice,
    int quantity,
    bool goodTillCanceled,
    bool allOrNone)
{
    return m_runtime->createStopLimitOrder(action, instrument, stopPrice, limitPrice, quantity, goodTillCanceled, allOrNone);
}

unsigned long Strategy::submitOrder(const Order& order)
{
    return m_runtime->submitOrder(order);
}

void Strategy::cancelOrder(unsigned long orderId)
{
    m_runtime->cancelOrder(orderId);
}

long Strategy::openLong(int quantity, const char* instrument, const char* signal)
{
    if (instrument == nullptr || instrument[0] == '0') {
        instrument = m_runtime->getMainInstrument();
    }

    int pos = getPositionSize(instrument);
    if (pos > 0) {
        return 0;
    }

    if (pos < 0) {
        closeShort(instrument, signal);
    }

    return m_runtime->buy(instrument, quantity, 0, false, signal);
}

long Strategy::closeLong(const char* instrument, const char* signal)
{
    if (instrument == nullptr || instrument[0] == '0') {
        instrument = m_runtime->getMainInstrument();
    }

    int pos = getPositionSize(instrument);
    if (pos <= 0) {
        return 0;
    }

    return m_runtime->sell(instrument, pos, 0.0, false, signal);
}

long Strategy::openShort(int quantity, const char* instrument, const char* signal)
{
    if (instrument == nullptr || instrument[0] == '0') {
        instrument = m_runtime->getMainInstrument();
    }

    int pos = getPositionSize(instrument);
    if (pos < 0) {
        return 0;
    }

    if (pos > 0) {
        closeLong(instrument, signal);
    }

    return m_runtime->sellShort(instrument, quantity, 0.0, false, signal);
}

long Strategy::closeShort(const char* instrument, const char* signal)
{
    if (instrument == nullptr || instrument[0] == '0') {
        instrument = m_runtime->getMainInstrument();
    }

    int pos = getPositionSize(instrument);
    if (pos >= 0) {
        return 0;
    }

    pos *= -1;

    return m_runtime->buyToCover(instrument, pos, 0.0, false, signal);
}

long Strategy::openLongImmediately(int quantity, double price, const char* instrument, const char* signal)
{
    if (instrument == nullptr || instrument[0] == '0') {
        instrument = m_runtime->getMainInstrument();
    }

    int pos = getPositionSize(instrument);
    if (pos > 0) {
        return 0;
    }

    if (pos < 0) {
        closeShortImmediately(price, instrument, signal);
    }

    return m_runtime->buy(instrument, quantity, price, true, signal);
}

long Strategy::closeLongImmediately(double price, const char* instrument, const char* signal)
{
    if (instrument == nullptr || instrument[0] == '0') {
        instrument = m_runtime->getMainInstrument();
    }

    int pos = getPositionSize(instrument);
    if (pos <= 0) {
        return 0;
    }

    return m_runtime->sell(instrument, pos, price, true, signal);
}

long Strategy::openShortImmediately(int quantity, double price, const char* instrument, const char* signal)
{
    if (instrument == nullptr || instrument[0] == '0') {
        instrument = m_runtime->getMainInstrument();
    }

    int pos = getPositionSize(instrument);
    if (pos < 0) {
        return 0;
    }

    if (pos > 0) {
        closeLongImmediately(price, instrument, signal);
    }

    return m_runtime->sellShort(instrument, quantity, price, true, signal);
}

long Strategy::closeShortImmediately(double price, const char* instrument, const char* signal)
{
    if (instrument == nullptr || instrument[0] == '0') {
        instrument = m_runtime->getMainInstrument();
    }

    int pos = getPositionSize(instrument);
    if (pos >= 0) {
        return 0;
    }

    pos *= -1;

    return m_runtime->buyToCover(instrument, pos, price, true, signal);
}

double Strategy::getAvailableCash() const
{
    return m_runtime->getAvailableCash();
}

double Strategy::getEquity() const
{
    return m_runtime->getEquity();
}

bool Strategy::registerParameter(const char* name, int type)
{
    return m_runtime->registerParameter(name, type);
}

int Strategy::loadHistoricalData(const DataRequest& request)
{
    return m_runtime->loadData(request);
}

bool Strategy::aggregateBarSeries(const TradingSession& session,
                                 int inRes, int inInterval, BarSeries& input,
                                 int outRes, int outInterval, BarSeries& output)
{
    return m_runtime->aggregateBarSeries(session, inRes, inInterval, input, outRes, outInterval, output);
}

void Strategy::printDebugMsg(const char* msgFmt, ...) const
{
#if 0 //_DEBUG
    char sMsg[1024];
    int bufLen = sizeof(sMsg);
    memset(sMsg, 0, bufLen);

    va_list args;
    va_start(args, msgFmt);
    int len = vsnprintf(NULL, 0, msgFmt, args);
    if (len > 0) {
        len = (len >= bufLen - 1 ? bufLen - 1 : len);
        vsnprintf(sMsg, len, msgFmt, args);
    }
    va_end(args);

    if (len > 0) {
        return m_runtime->writeDebugMsg(sMsg);
    }
#endif
}

void Strategy::onPositionOpened(Position& position)
{
    /* empty */
}

void Strategy::onPositionChanged(Position& position, const Position::Variation& variation)
{
    /* empty */
}

void Strategy::onPositionClosed(Position& position)
{
    /* empty */
}

void Strategy::onCreate()
{
    /* empty */
}

void Strategy::onSetParameter(const char* name, int type, const char* value, bool isLast)
{
    /* empty */
}

void Strategy::onEnvVariable(const char* name, double value)
{
    /* empty */
}

void Strategy::onStart()
{
    /* empty */
}

void Strategy::onStop()
{
    /* empty */
}

void Strategy::onDestroy()
{
    /* empty */
}

void Strategy::onBar(const Bar& bar)
{
    /* empty */
}

void Strategy::onHistoricalData(const Bar& bar, bool isCompleted)
{
    /* empty */
}

// Remove from backtesting mode.
#if 0
void Strategy::onOrderUpdated(const Order& order)
{
    /* empty */
}
#endif

void Strategy::onOrderFilled(const Order& order)
{
    /* empty */
}

void Strategy::onOrderPartiallyFilled(const Order& order)
{
    /* empty */
}

void Strategy::onOrderCanceled(const Order& order)
{
    /* empty */
}

void Strategy::onOrderFailed(const Order& order)
{
    /* empty */
}

void Strategy::onTimeElapsed(const DateTime& prevDateTime, const DateTime& nextDateTime)
{
    /* empty */
}

} // namespace xBacktest