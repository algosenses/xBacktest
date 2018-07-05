#include "MACD.h"

namespace xBacktest
{

void MACD::init(DataSeries& dataSeries, int fastEMA, int slowEMA, int signalEMA, int maxLen)
{
    // We need to skip some values when calculating the fast EMA in order for both EMA
    // to calculate their first values at the same time.
    // I'M FORCING THIS BEHAVIOUR ONLY TO MAKE THIS FITLER MATCH TA-Lib MACD VALUES.
    m_fastEMASkip = slowEMA - fastEMA;

    m_fastEMAWindow.init(fastEMA);
    m_slowEMAWindow.init(slowEMA);
    m_signalEMAWindow.init(signalEMA);

    dataSeries.getNewValueEvent().subscribe(this);
}

DataSeries& MACD::getSignal()
{
    return m_signal;
}

DataSeries& MACD::getHistogram()
{
    return m_histogram;
}

void MACD::onNewValue(const DateTime& datetime, double value)
{
    double diff           = std::numeric_limits<double>::quiet_NaN();
    double macdValue      = std::numeric_limits<double>::quiet_NaN();
    double signalValue    = std::numeric_limits<double>::quiet_NaN();
    double histogramValue = std::numeric_limits<double>::quiet_NaN();

    // We need to skip some values when calculating the fast EMA in order for both EMA
    // to calculate their first values at the same time.
    // I'M FORCING THIS BEHAVIOUR ONLY TO MAKE THIS FITLER MATCH TA-Lib MACD VALUES.
    m_slowEMAWindow.onNewValue(datetime, value);
    if (m_fastEMASkip > 0) {
        m_fastEMASkip -= 1;
    } else {
        m_fastEMAWindow.onNewValue(datetime, value);
        if (m_fastEMAWindow.windowFull()) {
            diff = m_fastEMAWindow.getValue() - m_slowEMAWindow.getValue();
        }
    }

    // Make the first MACD value available as soon as the first signal value is available.
    // I'M FORCING THIS BEHAVIOUR ONLY TO MAKE THIS FITLER MATCH TA-Lib MACD VALUES.
    m_signalEMAWindow.onNewValue(datetime, diff);
    if (m_signalEMAWindow.windowFull()) {
        macdValue = diff;
        signalValue = m_signalEMAWindow.getValue();
        histogramValue = macdValue - signalValue;
    }

    appendWithDateTime(datetime, &macdValue);
    m_signal.appendWithDateTime(datetime, &signalValue);
    m_histogram.appendWithDateTime(datetime, &histogramValue);
}

void MACD::onEvent(int type, const void* context)
{
    if (type == Event::EvtDataSeriesNewValue) {
        Event::Context& ctx = *((Event::Context *)context);
        onNewValue(ctx.datetime, *((double *)ctx.data));
    }
}

} // namespace xBacktest
