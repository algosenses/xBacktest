#ifndef HIGH_LOW_H
#define HIGH_LOW_H

#include <deque>
#include "Technical.h"

namespace xBacktest
{

class HighLowEventWindow : public EventWindow<double>
{
public:
    void init(int period, bool useMin);
    void onNewValue(const DateTime& datetime, const double& value);
    double getValue() const;

private:
    bool m_useMin;
    int m_winSize;
    double m_minimun;
    double m_maximum;
    int64_t count;

    std::deque< std::pair<double, int64_t> > m_window;
};

class DllExport High : public EventBasedFilter<double, double>
{
public:
    void init(DataSeries& dataSeries, int period, int maxLen = DataSeries::DEFAULT_MAX_LEN);

private:
    HighLowEventWindow m_eventWindow;
};

class DllExport Low : public EventBasedFilter<double, double>
{
public:
    void init(DataSeries& dataSeries, int period, int maxLen = DataSeries::DEFAULT_MAX_LEN);

private:
    HighLowEventWindow m_eventWindow;
};

DllExport int SwingHigh(const DataSeries& dataSeries, int leftStrength, int rightStrength);

DllExport int SwingLow(const DataSeries& dataSeries, int leftStrength, int rightStrength);

} // namespace xBacktest

#endif // HIGH_LOW_H
