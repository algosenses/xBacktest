#include "PositionTracker.h"
#include "Position.h"
#include "Errors.h"

namespace xBacktest
{

PositionTracker::PositionTracker(const string& instrument)
{
    m_instrument = instrument;
    m_longPos = 0;
    m_shortPos = 0;
    m_longPosAvgPrice = 0;
    m_shortPosAvgPrice = 0;
    m_multiplier = 1;
    m_commissions = 0;
    m_slippages = 0;
    m_costPerShare = 0;
    m_costBasis = 0;
    m_lastNetProfit = 0;
    m_lastReturn = 0;
    m_cumTradedShares = 0;
    m_currPosTradedVolume = 0;
}

const string& PositionTracker::getInstrument() const
{
    return m_instrument;
}

void PositionTracker::setMultiplier(double multiplier)
{
    if (multiplier > 0) {
        m_multiplier = multiplier;
    } else {
        ASSERT(false, "Multiplier must be greater than 0.");
    }
}

int PositionTracker::getShares() const
{
    return (m_longPos - m_shortPos);
}

int PositionTracker::getCumTradedShares() const
{
    return m_cumTradedShares;
}

double PositionTracker::getCommissions() const
{
    return m_commissions;
}

double PositionTracker::getSlippages() const
{
    return m_slippages;
}

// Transaction cost = commissions + slippages
double PositionTracker::getNetProfit(bool includeTradeCost) const
{
    return m_lastNetProfit;
}

double PositionTracker::getReturn(bool includeTradeCost) const
{
    return m_lastReturn;
}

void PositionTracker::buy(const DateTime& datetime, int quantity, double price, double commission, double slippage)
{
    REQUIRE(quantity > 0, "Quantity must be greater than 0");

    TradingRecord trade;
    trade.datetime = datetime;
    trade.price    = price;
    trade.quantity = quantity;

    if (m_longPos == 0) {
        trade.type = Position::Action::EntryLong;         // Entry long position.
    } else if (m_longPos > 0) {
        trade.type = Position::Action::IncreaseLong;      // Increase long position.
    }

    double cost = m_longPosAvgPrice * m_longPos;
    cost += price * quantity;
    m_longPos += quantity;
    m_longPosAvgPrice = cost / m_longPos;

    m_cumTradedShares += quantity;
    m_commissions += commission;
    m_slippages += slippage;

    m_entryTrades.push_back(trade);
    m_allTrades.push_back(trade);
    m_currActivePosTrades.push_back(trade);
    m_currPosTradedVolume += quantity;
}

void PositionTracker::sell(const DateTime& datetime, int quantity, double price, double commission, double slippage)
{
    REQUIRE(quantity > 0, "Quantity must be greater than 0");
    if (quantity > m_longPos) {
        ASSERT(false, "Quantity must be less than long position size");
    }

    TradingRecord trade;
    trade.datetime = datetime;
    trade.price    = price;
    trade.quantity = quantity;

    if (quantity == m_longPos) {
        trade.type = Position::Action::ExitLong;      // Exit long position.
    } else {
        trade.type = Position::Action::ReduceLong;    // Reduce long position.
    }

    m_lastNetProfit = (price - m_longPosAvgPrice) * quantity * m_multiplier;
    m_lastReturn = m_lastNetProfit / (m_longPosAvgPrice * quantity * m_multiplier);

    m_longPos -= quantity;

    if (m_longPos == 0) {
        m_longPosAvgPrice = 0;
    }

    m_cumTradedShares += quantity;
    m_commissions += commission;
    m_slippages += slippage;

    m_exitTrades.push_back(trade);
    m_allTrades.push_back(trade);
    m_currActivePosTrades.push_back(trade);
    m_currPosTradedVolume += quantity;
}

void PositionTracker::sellShort(const DateTime& datetime, int quantity, double price, double commission, double slippage)
{
    REQUIRE(quantity > 0, "Quantity must be greater than 0");

    TradingRecord trade;
    trade.datetime = datetime;
    trade.price = price;
    trade.quantity = quantity;

    if (m_shortPos == 0) {
        trade.type = Position::Action::EntryShort;        // Entry short position.
    } else if (m_shortPos > 0) {
        trade.type = Position::Action::IncreaseShort;     // Increase short position.
    }

    double cost = m_shortPosAvgPrice * m_shortPos;
    cost += price * quantity;
    m_shortPos += quantity;
    m_shortPosAvgPrice = cost / m_shortPos;

    m_cumTradedShares += quantity;
    m_commissions += commission;
    m_slippages += slippage;

    m_entryTrades.push_back(trade);
    m_allTrades.push_back(trade);
    m_currActivePosTrades.push_back(trade);
    m_currPosTradedVolume += quantity;
}

void PositionTracker::cover(const DateTime& datetime, int quantity, double price, double commission, double slippage)
{
    REQUIRE(quantity > 0, "Quantity must be greater than 0");
    REQUIRE(quantity <= m_shortPos, "Quantity must be less than short position size");

    TradingRecord trade;
    trade.datetime = datetime;
    trade.price = price;
    trade.quantity = quantity;

    if (quantity == abs(m_shortPos)) {
        trade.type = Position::Action::ExitShort;     // Exit short position.
    } else {
        trade.type = Position::Action::ReduceShort;   // Reduce short position.
    }

    m_lastNetProfit = (m_shortPosAvgPrice - price) * quantity * m_multiplier;
    m_lastReturn = m_lastNetProfit / (m_shortPosAvgPrice * quantity * m_multiplier);

    m_shortPos -= quantity;

    if (m_shortPos == 0) {
        m_shortPosAvgPrice = 0;
    }

    m_cumTradedShares += quantity;
    m_commissions += commission;
    m_slippages += slippage;

    m_exitTrades.push_back(trade);
    m_allTrades.push_back(trade);
    m_currActivePosTrades.push_back(trade);
    m_currPosTradedVolume += quantity;
}

void PositionTracker::getLastClosePosTrade(ClosePosTrade& item)
{
    item.instrument = m_instrument;
    item.commissions = m_commissions;
    item.slippages = m_slippages;
    item.realizedProfit = getNetProfit(0);
    item.tradedVolume = m_currPosTradedVolume;
    item.closeVolume = item.tradedVolume / 2;
    item.trades = m_currActivePosTrades;
    item.tradeNum = item.trades.size();
}

const vector<TradingRecord>& PositionTracker::getAllTrades() const
{
    return m_allTrades;
}

const vector<TradingRecord>& PositionTracker::getCurrPosTrades() const
{
    return m_currActivePosTrades;
}

} // namespace xBacktest