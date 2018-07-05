#ifndef RSI_H
#define RSI_H

#include "Technical.h"

namespace xBacktest
{

class RSIEventWindow : public EventWindow<double, double>
{
public:
    void init(int period);
    void onNewValue(const DateTime& datetime, const double& value);
    double getValue() const;

private:
    double m_value;
    double m_prevGain;
    double m_prevLoss;
    int    m_period;
};

class DllExport RSI : public EventBasedFilter<double, double>
{
public:
    //Relative Strength Index filter as described in http://stockcharts.com/school/doku.php?id=chart_school:technical_indicators:relative_strength_index_rsi.
    //@param dataSeries: The DataSeries instance being filtered.
    //@param period: The period. Note that if period is n, then n+1 values are used. Must be > 1.
    //@param maxLen: The maximum number of values to hold.
    // Once a bounded length is full, when new items are added, a corresponding number of items are discarded from the opposite end.
    void init(DataSeries& dataSeries, int period, int maxLen = DataSeries::DEFAULT_MAX_LEN);

private:
    RSIEventWindow m_rsiEventWnd;
};

} // namespace xBacktest

#endif // RSI_H
