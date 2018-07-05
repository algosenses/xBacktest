#ifndef ATR_H
#define ATR_H

#include "BarSeries.h"
#include "Technical.h"

namespace xBacktest
{

// This event window will calculate and hold true - range values.
// Formula from http ://stockcharts.com/help/doku.php?id=chart_school:technical_indicators:average_true_range_a.
class ATREventWindow : public EventWindow<Bar, double>
{
public:
    void init(int period);
    void onNewValue(const DateTime& datetime, const Bar& bar);
    double getValue() const;

private:
    double calculateTrueRange(const Bar& bar);

private:
    double m_value;
    double m_prevClose;
};

// Average True Range filter as described in http://stockcharts.com/help/doku.php?id=chart_school:technical_indicators:average_true_range_a.
//
// @param barSeries : The BarSeries instance being filtered.
// @param period : The average period.Must be > 1.
// @param useAdjustedValues : True to use adjusted Low / High / Close values.
// @param maxLen : The maximum number of values to hold.
// Once a bounded length is full, when new items are added, a corresponding number of items are discarded from the opposite end.
class DllExport ATR : public EventBasedFilter<Bar, double>
{
public:
    void init(BarSeries& barSeries, int period, int maxLen = DataSeries::DEFAULT_MAX_LEN);

private:
    ATREventWindow m_atrEventWnd;
};

} // namespace xBacktest

#endif // ATR_H
