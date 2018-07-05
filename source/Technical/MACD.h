#ifndef MACD_H
#define MACD_H

#include "DataSeries.h"
#include "Technical.h"
#include "MA.h"

namespace xBacktest
{

// Moving Average Convergence-Divergence indicator as described in 
// http://stockcharts.com/school/doku.php?id=chart_school:technical_indicators:moving_average_conve.
class DllExport MACD : public SequenceDataSeries<double>, public IEventHandler
{
public:
    void init(DataSeries& dataSeries, int fastEMA = 12, int slowEMA = 26, int signalEMA = 9, int maxLen = DataSeries::DEFAULT_MAX_LEN);
    // Returns a `DataSeries` with the EMA over the MACD.
    DataSeries& getSignal();
    // Returns a `DataSeries` with the histogram (the difference between the MACD and the Signal).
    DataSeries& getHistogram();

private:
    void onEvent(int type, const void* context);
    void onNewValue(const DateTime& datetime, double value);

private:
    int m_fastEMASkip;
    EMAEventWindow m_fastEMAWindow;
    EMAEventWindow m_slowEMAWindow;
    EMAEventWindow m_signalEMAWindow;
    SequenceDataSeries<double> m_signal;
    SequenceDataSeries<double> m_histogram;
};

} // namespace xBacktest

#endif // MACD_H
