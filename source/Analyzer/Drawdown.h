#ifndef DRAWDOWN_H
#define DRAWDOWN_H

#include "DateTime.h"
#include "Returns.h"
#include "StgyAnalyzer.h"

namespace xBacktest
{

class StrategyImpl;

class DrawDownHelper
{
public:
    DrawDownHelper();
    int    getDuration();
    double getMaxDrawDown(bool usePercentage = true);
    double getCurrentDrawDown(bool usePercentage = true);
    const DateTime& getHighDateTime() const;
    const DateTime& getLastDateTime() const;
    void   update(const DateTime& dateTime, double equity);

private:
    double   m_highWatermark;
    double   m_lowWatermark;
    double   m_lastLow;
    DateTime m_highDateTime;
    DateTime m_lastDateTime;
};

// A standalone drawdown calculator.
class DrawDownCalculator
{
public:
    DrawDownCalculator();
    void reset();
    // Calculate the greatest loss drawdown, from the previous highest
    // equity run-up, closed trade to closed trade looking across all 
    // trades, during the specified period.
    bool calculate(const vector<Returns::Ret>& returns);
    bool calculate(const vector<Returns::Equity>& equities);
    bool calculate(const vector<Trades::TradeProfit>& data, double initial);
    void update(const DateTime& dt, double equity);

    double getMaxDrawDown(bool usePercentage = true);
    const DateTime& getMaxDrawDownBegin() const;
    const DateTime& getMaxDrawDownEnd() const;
    double getLongestDrawDownDuration();
    const DateTime& getLongestDDBegin() const;
    const DateTime& getLongestDDEnd() const;

private:
    bool     m_initialized;

    double   m_maxDD;
    double   m_maxDDPercentage;
    DateTime m_maxDDBegin;
    DateTime m_maxDDEnd;

    double   m_longestDDDuration;
    DateTime m_longestDDBegin;
    DateTime m_longestDDEnd;

    double   m_highWatermark;
    double   m_lowWatermark;

    DateTime m_highDateTime;
    DateTime m_lastDateTime;
};

// A `StgyAnalyzer` that calculates max. drawdown and longest drawdown duration for the portfolio.
class DrawDown : public StgyAnalyzer
{
public:
    DrawDown();
    double calculateEquity(BacktestingBroker& broker);
    void beforeOnBar(BacktestingBroker& broker, const Bar& bars);
    // Returns the max. (deepest) drawdown, from the previous highest equity run-up, bar to bar
    // looking across all trades, during the specified period.
    // If a new bar equity run-up high occurs, the low equity value is reset to 0 so that the
    // next maximum drawdown can be calculated from that point.
    double getMaxDrawDown(bool usePercentage = true);
    const DateTime& getMaxDrawDownBegin() const;
    const DateTime& getMaxDrawDownEnd() const;

    // Returns the duration of the longest drawdown.
    //  Note that this is the duration of the longest drawdown, not necessarily the deepest one.
    double getLongestDrawDownDuration();
    const DateTime& getLongestDDBegin() const;
    const DateTime& getLongestDDEnd() const;

private:
    DrawDownCalculator m_calculator;
};

} // namespace xBacktest

#endif // DRAWDOWN_H
