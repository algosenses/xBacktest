#ifndef SHARPE_H
#define SHARPE_H

#include "StgyAnalyzer.h"
#include "Returns.h"

namespace xBacktest
{

// A `StgyAnalyzer` that calculates Sharpe ratio for the whole portfolio.
class SharpeRatio : public StgyAnalyzer, public IEventHandler
{
public:
    // @param useDailyReturns: True if daily returns should be used instead of the returns for each bar.
    SharpeRatio(bool useDailyReturns = true);
    const vector<double>& getReturns() const;
    void beforeAttach(BacktestingBroker& broker);
    // Returns the Sharpe ratio for the strategy execution. If the volatility is 0, 0 is returned.
    // @param riskFreeRate: The risk free rate per annum.
    // @param annualized: True if the sharpe ratio should be annualized.
    double getSharpeRatio(double riskFreeRate, bool annualized = true);

    void onEvent(int type, const DateTime& datetime, const void *context);

private:
    void onReturns(const DateTime& dateTime, ReturnsAnalyzerBase& returnsAnalyzerBase);

private:
    bool m_useDailyReturns;
    vector<double> m_returns;
    // Only use when useDailyReturns == false
    DateTime m_firstDateTime;
    DateTime m_lastDateTime;
    // Only use when useDailyReturns == true
    DateTime m_currentDate;
};

} // namespace xBacktest

#endif // SHARPE_H
