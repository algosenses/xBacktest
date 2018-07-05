#include "KDJ.h"

namespace xBacktest
{

static void get_low_high_values(const SequenceDataSeries<Bar>& bars, double& lowestLow, double& highestHigh)
{
    const Bar& firstBar = bars[0];
    lowestLow   = firstBar.getLow();
    highestHigh = firstBar.getHigh();
    int len = bars.length();
    for (int i = 0; i < len; i++) {
        const Bar& bar = bars[i];
        if (lowestLow > bar.getLow()) {
            lowestLow = bar.getLow();
        }
        if (highestHigh < bar.getHigh()) {
            highestHigh = bar.getHigh();
        }
    }
}


void KDJ::init(BarSeries& barSeries, int period, int slowLen, int smoothLen, int maxLen)
{
    REQUIRE(period > 1, "KDJ period must greater than 1");

    m_period = period;
    m_slowLen = slowLen;
    m_smoothLen = smoothLen;

    m_barSeries.setMaxLen(period);
    barSeries.getNewValueEvent().subscribe(this);
}

DataSeries& KDJ::getK()
{
    return m_k;
}

DataSeries& KDJ::getD()
{
    return m_d;
}

DataSeries& KDJ::getJ()
{
    return m_j;
}

void KDJ::onNewValue(const DateTime& datetime, Bar& value)
{
    m_barSeries.appendWithDateTime(datetime, &value);
    int len = m_barSeries.length();
    if (len >= m_period) {
        double high, low;
        get_low_high_values(m_barSeries, low, high);
        double close = value.getClose();
        double rsv;
        if (high == low) {
            rsv = 0;
        } else {
            rsv = (close - low) / (high - low);
            rsv *= 100;
        }

        double k;
        if (m_k.length() > 0) {
            k = (m_slowLen - 1.0) / m_slowLen * m_k[0] + 1.0 / m_slowLen * rsv;
        } else {
            k = (m_slowLen - 1.0)  / m_slowLen * 50 + 1.0 / m_slowLen * rsv;
        }
        m_k.appendWithDateTime(datetime, &k);

        double d;
        if (m_d.length() > 0) {
            d = (m_smoothLen - 1.0) / m_smoothLen * m_d[0] + 1.0 / m_smoothLen * k;
        } else {
            d = (m_smoothLen - 1.0) / m_smoothLen * 50 + 1.0 / m_smoothLen * k;
        }
        m_d.appendWithDateTime(datetime, &d);

        double j = 3 * k - 2 * d;
        m_j.appendWithDateTime(datetime, &j);
    }
}

void KDJ::onEvent(int type, const DateTime& datetime, const void *context)
{
    if (type == Event::EvtDataSeriesNewValue) {
        Event::Context& ctx = *((Event::Context *)context);
        onNewValue(ctx.datetime, *((Bar *)ctx.data));
    }
}

} // namespace xBacktest