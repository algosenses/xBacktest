#include "Stoch.h"
#include "Errors.h"

namespace xBacktest
{

static void get_low_high_values(const SequenceDataSeries<Bar>& bars, double& lowestLow, double& highestHigh)
{
    Bar currBar = bars[0];
    lowestLow   = currBar.getLow();
    highestHigh = currBar.getHigh();
    for (int i = 0; i < bars.length(); i++) {
        currBar = bars[i];
        if (lowestLow > currBar.getLow()) {
            lowestLow = currBar.getLow();
        }
        if (highestHigh < currBar.getHigh()) {
            highestHigh = currBar.getHigh();
        }
    }
}

void SOEventWindow::init(int period)
{
    REQUIRE(period > 1, "Period must greater than 1");
    EventWindow<Bar, double>::init(period);
}

void SOEventWindow::onNewValue(const DateTime& datetime, const Bar& bar)
{
    m_barSeries.appendWithDateTime(datetime, &bar);
}

double SOEventWindow::getValue() const
{
    double ret = std::numeric_limits<double>::quiet_NaN();
    if (windowFull()) {
        double lowestLow, highestHigh;
        get_low_high_values(m_barSeries, lowestLow, highestHigh);
        double currentClose = m_barSeries[0].getClose();
        ret = (currentClose - lowestLow) / float(highestHigh - lowestLow) * 100;
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////
void StochasticOscillator::init(BarSeries& barSeries, int period, int dSMAPeriod, bool useAdjustedValues, int maxLen)
{
    REQUIRE(dSMAPeriod > 1, "SMA period must greater than 1");

    EventBasedFilter<Bar, double>::init(&barSeries, &m_evtWindow, maxLen);
    m_d.init(*this, dSMAPeriod, maxLen);
}

DataSeries& StochasticOscillator::getD()
{
    return m_d;
}

} // namespace xBacktest
