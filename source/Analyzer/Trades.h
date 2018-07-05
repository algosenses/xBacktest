#ifndef TRADES_H
#define TRADES_H

#include "Defines.h"
#include "Event.h"
#include "DataSeries.h"
#include "StgyAnalyzer.h"
#include "PositionTracker.h"
#include "Position.h"

namespace xBacktest
{

// A `StgyAnalyzer` that records the profit/loss and returns of every completed trade.
// note:
//    This analyzer operates on individual completed trades.
//    For example, lets say you start with a $1000 cash, and then you buy 1 share of XYZ
//    for $10 and later sell it for $20:
//
//    * The trade's profit was $10.
//    * The trade's return is 100%, even though your whole portfolio went from $1000 to $1020, a 2% return.
class Trades : public StgyAnalyzer, public IEventHandler
{
public:
    typedef struct {
        DateTime datetime;
        double   value;
    } TradeProfit;

    Trades();
    ~Trades();
    void attached(BacktestingBroker& broker);
    // Returns the total number of trades.
    const int getCount() const;
    // Returns the number of profitable trades.
    int getProfitableCount() const;
    // Returns the number of unprofitable trades.
    int getUnprofitableCount() const;
    // Returns the number of trades whose net profit was 0.
    int getEvenCount() const;
    
    // The overall dollar profit or loss achieved by the trading
    // strategy in the test period. (If you are working with an
    // investment instrument using leverage, Return on Account plays
    // a more important part in strategy evaluation.)
    double getTotalNetProfits() const;

    double getTotalTradeCost() const;

    const vector<DailyMetrics>& getAllDailyMetrics();
    // Returns a array with the profits/losses for each trade.
    const vector<TradeProfit>& getAll() const;
    // Returns a array with the profits for each profitable trade.
    const vector<TradeProfit>& getProfits() const;
    // Returns a array with the losses for each unprofitable trade.
    const vector<double>& getLosses() const;
    // Returns a array with the returns for each trade.
    const vector<double>& getAllReturns() const;
    // Returns a array with the positive returns for each trade.
    const vector<double>& getPositiveReturns() const;
    // Returns a array with the negative returns for each trade.
    const vector<double>& getNegativeReturns() const;
    // Returns a array with the commissions for each trade.
    const vector<double>& getCommissionsForAllTrades() const;
    // Returns a array with the commissions for each profitable trade.
    const vector<double>& getCommissionsForProfitableTrades() const;
    // Returns a array with the commissions for each unprofitable trade.
    const vector<double>& getCommissionsForUnprofitableTrades() const;
    // Returns a array with the commissions for each trade whose net profit was 0.
    const vector<double>& getCommissionsForEvenTrades() const;

    void getAllTradeRecords(vector<TradingRecord>& trades) const;

    void onEvent(int type, const DateTime& datetime, const void *context);

private:
    const Contract& getContract(const string& instrument) const;
    void updateTrades(const DateTime& dt, PositionTracker& posTracker);
    void updateDailyMetrics();
    void updatePosTracker(PositionTracker& posTracker, 
                          const DateTime& datetime,
                          Order::Action action,
                          double price, 
                          int quantity,
                          double commission, 
                          double slippage);
    void onOrderUpdated(const OrderEvent& orderEvent);
    void onNewTradingDay(const DateTime& lastDateTime, const DateTime& currDateTime);

private:
    BacktestingBroker* m_broker;

    vector<TradeProfit> m_all;
    vector<TradeProfit> m_profits;
    vector<double> m_losses;
    vector<double> m_allReturns;
    vector<double> m_positiveReturns;
    vector<double> m_negativeReturns;
    vector<double> m_profitableCommissions;
    vector<double> m_unprofitableCommissions;
    vector<double> m_evenCommissions;

    map<string, PositionTracker*> m_posTrackers;

    vector<ClosePosTrade>  m_allClosedTransactions;

    unsigned int m_lastClosedPosNum;

    double m_totalNetProfits;
    int m_totalTradedVolume;
    int m_totalTradeNum;
    int m_totalClosedVolume;
    double m_totalRealizedProfit;
    double m_totalTradeCost;
    int m_evenTrades;
    DateTime m_lastTradingDay;
    DateTime m_lastTradingDateTime;
    vector<DailyMetrics> m_allDailyMetrics;
    int m_tradingDayNum;
};

} // namespace xBacktest

#endif // TRADES_H
