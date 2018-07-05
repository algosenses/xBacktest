#ifndef RETURNS_H
#define RETURNS_H

#include "Event.h"
#include "DataSeries.h"
#include "StgyAnalyzer.h"
#include "Trades.h"

using std::vector;

namespace xBacktest
{

class ReturnsAnalyzerBase : public StgyAnalyzer
{
public:
    ReturnsAnalyzerBase();
    static ReturnsAnalyzerBase* getOrCreateShared(BacktestingBroker& broker);
    void attached(BacktestingBroker& broker);
    // An event will be notified when return are calculated at each bar. The hander should receive 1 parameter:
    // 1. The current datetime.
    // 2. This analyzer's instance
    Event& getEvent();
    double getNetReturn() const;
    double getCumulativeReturn() const;
    double getEquity() const;
    void   beforeOnBar(BacktestingBroker& broker, const Bar& bar);

private:
    double m_netRet;
    double m_cumRet;
    double m_equity;
    Event  m_event;
    double m_lastPortfolioValue;
};


// A `StgyAnalyzer` that calculates returns and cumulative returns for the whole portfolio.
class Returns : public StgyAnalyzer, public IEventHandler
{
public:
    typedef struct {
        DateTime datetime;
        double ret;
    } Ret;

    typedef struct {
        DateTime datetime;
        double equity;
    } Equity;

    Returns();
    void beforeAttach(BacktestingBroker& broker);
    void attached(BacktestingBroker& broker);
    // Returns a `DataSeries` with the returns for each bar.
    const vector<Ret>& getReturns() const;
    // Returns a `DataSeries` with the cumulative returns for each bar.
    const vector<Ret>& getCumulativeReturns() const;
    const vector<Equity>& getEquities() const;

    void onEvent(int type, const DateTime& datetime, const void *context);
    
private:
    void onReturns(const DateTime& datetime, ReturnsAnalyzerBase& returnsAnalyzerBase);

    vector<Ret> m_netReturns;
    vector<Ret> m_cumReturns;
    vector<Equity> m_equities;
};

} // namespace xBacktest

#endif // RETURNS_H
