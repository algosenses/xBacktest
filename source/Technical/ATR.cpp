#include "ATR.h"

namespace xBacktest
{

void ATREventWindow::init(int period)
{
    assert(period > 1);

    EventWindow<Bar, double>::init(period);
    m_prevClose = std::numeric_limits<double>::quiet_NaN();
    m_value     = std::numeric_limits<double>::quiet_NaN();
}

double ATREventWindow::calculateTrueRange(const Bar& bar)
{
    double ret = std::numeric_limits<double>::quiet_NaN();
    if (isnan(m_prevClose)) {
        m_prevClose = bar.getHigh() - bar.getLow();
    } else {
        double tr1 = bar.getHigh() - bar.getLow();
        double tr2 = fabs(bar.getHigh() - m_prevClose);
        double tr3 = fabs(bar.getLow() - m_prevClose);
        ret = max(max(tr1, tr2), tr3);
    }

    return ret;
}

void ATREventWindow::onNewValue(const DateTime& datetime, const Bar& bar)
{
    double tr = calculateTrueRange(bar);
    EventWindow<Bar, double>::pushNewValue(datetime, tr);
    m_prevClose = bar.getClose();

    if (windowFull()) {
        if (isnan(m_value)) {
            m_value = getValues().mean();
        } else {
            m_value = (m_value * (getWindowSize() - 1) + tr) / float(getWindowSize());
        }
    }
}

double ATREventWindow::getValue() const
{
    return m_value;
}

void ATR::init(BarSeries& barSeries, int period, int maxLen /* = DataSeries::DEFAULT_MAX_LEN */)
{
    m_atrEventWnd.init(period);

    EventBasedFilter<Bar, double>::init(&barSeries, &m_atrEventWnd, maxLen);
}

} // namespace xBacktest
