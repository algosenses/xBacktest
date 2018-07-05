#ifndef POSITION_TRACKER_H
#define POSITION_TRACKER_H

#include <vector>
#include "DateTime.h"

namespace xBacktest
{

typedef struct {
    DateTime datetime;
    int      type;
    double   price;
    int      quantity;
} TradingRecord;

typedef struct {
    string instrument;
    int    tradeNum;
    int    tradedVolume;
    int    closeVolume;
    double realizedProfit;
    double commissions;
    double slippages;
    vector<TradingRecord> trades;
} ClosePosTrade;

// Helper class to calculate returns and net profit.
class PositionTracker
{
public:
    PositionTracker(const string& instrument);

    const string& getInstrument() const;
    void   setMultiplier(double multiplier);
    int    getShares() const;
    double getCommissions() const;
    double getSlippages() const;
    double getNetProfit(bool includeTradeCost = true) const;
    double getReturn(bool includeTradeCost = true) const;
    int    getCumTradedShares() const;
    void   buy(const DateTime& datetime, int quantity, double price, double commission, double slippage);
    void   sell(const DateTime& datetime, int quantity, double price, double commission, double slippage);
    void   sellShort(const DateTime& datetime, int quantity, double price, double commission, double slippage);
    void   cover(const DateTime& datetime, int quantity, double price, double commission, double slippage);
    void   getLastClosePosTrade(ClosePosTrade& item);
    const vector<TradingRecord>& getAllTrades() const;
    const vector<TradingRecord>& getCurrPosTrades() const;

private:
    string m_instrument;
    int    m_longPos;
    int    m_shortPos;
    double m_longPosAvgPrice;
    double m_shortPosAvgPrice;
    double m_multiplier;
    double m_commissions;
    double m_slippages;
    double m_costPerShare;
    double m_costBasis;

    double m_lastNetProfit;
    double m_lastReturn;

    // Cumulative traded shares.
    int    m_cumTradedShares;

    // All trades(entry, exit, all) for this instrument during
    // the trading period.
    vector<TradingRecord> m_entryTrades;
    vector<TradingRecord> m_exitTrades;
    vector<TradingRecord> m_allTrades;

    // Trades from every time position opened to closed.
    // It will be cleared when position's share is equal to 0.
    vector<TradingRecord> m_currActivePosTrades;
    int m_currPosTradedVolume;
};

} // namespace xBacktest

#endif // POSITION_TRACKER_H