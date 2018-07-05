#include <cmath>
#include <cassert>
#include "DateTime.h"
#include "MA.h"

namespace xBacktest
{

////////////////////////////////////////////////////////////////////////////////
void MAEventWindow::init(int period)
{
    EventWindow<double, double>::init(period);
    m_value = std::numeric_limits<double>::quiet_NaN();
}

void MAEventWindow::onNewValue(const DateTime& datetime, const double& value)
{
    double firstValue = std::numeric_limits<double>::quiet_NaN();
    if (getValues().size() > 0) {
        firstValue = getValues()[0];
        assert(!std::isnan(firstValue));
    }

    EventWindow::pushNewValue(datetime, value);

    if (!std::isnan(value) && windowFull()) {
        if (std::isnan(m_value)) {
            m_value = getValues().mean();
        } else {            
            // m_value = m_value + value / float(getWindowSize()) - firstValue / float(getWindowSize());
            // Above statement simply equal to :
            m_value += (value - firstValue) / double(getWindowSize());
        }
    }
}

double MAEventWindow::getValue() const
{
	return m_value;
}

MA::MA()
{

}

void MA::init(DataSeries& dataSeries, int period, int maxLen)
{
    m_smaEventWnd.init(period);
    EventBasedFilter<double, double>::init(&dataSeries, &m_smaEventWnd, maxLen);
}

////////////////////////////////////////////////////////////////////////////////
void EMAEventWindow::init(int period)
{
    assert(period > 1);
    EventWindow<double, double>::init(period);
    m_multiplier = (2.0 / (period + 1));
    m_value = std::numeric_limits<double>::quiet_NaN();
}

void EMAEventWindow::onNewValue(const DateTime& datetime, const double& value)
{
    EventWindow::pushNewValue(datetime, value);

    // Formula from http://stockcharts.com/school/doku.php?id=chart_school:technical_indicators:moving_averages
    if (!std::isnan(value) && windowFull()) {
        if (std::isnan(m_value)) {
            m_value = getValues().mean();
        } else {
            m_value = (value - m_value) * m_multiplier + m_value;
        }
    }
}

double EMAEventWindow::getValue() const
{
    return m_value;
}

void EMA::init(DataSeries& dataSeries, int period, int maxLen)
{
    m_emaEventWnd.init(period);

    EventBasedFilter<double, double>::init(&dataSeries, &m_emaEventWnd, maxLen);
}

////////////////////////////////////////////////////////////////////////////////
void SMAEventWindow::init(int period, double weight)
{
    assert(period > 1);
    EventWindow<double, double>::init(period);
    m_weight = weight;
    m_multiplier = weight / period;
    m_value = std::numeric_limits<double>::quiet_NaN();
}

void SMAEventWindow::onNewValue(const DateTime& datetime, const double& value)
{
    EventWindow::pushNewValue(datetime, value);

    if (!std::isnan(value) && windowFull()) {
        if (std::isnan(m_value)) {
            m_value = getValues().mean();
        } else {
            m_value = (value - m_value) * m_multiplier + m_value;
        }
    }
}

double SMAEventWindow::getValue() const
{
    return m_value;
}

void SMA::init(DataSeries& dataSeries, int period, double weight, int maxLen)
{
    m_smaEventWnd.init(period, weight);

    EventBasedFilter<double, double>::init(&dataSeries, &m_smaEventWnd, maxLen);
}

////////////////////////////////////////////////////////////////////////////////
void AMAEventWindow::init(int effRatioLen, int fastAvgLen, int slowAvgLen)
{
    assert(effRatioLen > 1);
    assert(fastAvgLen > 1);
    assert(slowAvgLen > 1);
    
    m_effRatioLen = effRatioLen;
    m_fastAvgLen = fastAvgLen;
    m_slowAvgLen = slowAvgLen;

    m_fastSc = 2.0 / (fastAvgLen + 1);
    m_slowSc = 2.0 / (slowAvgLen + 1);

    EventWindow<double, double>::init(effRatioLen + 1);

    m_value = std::numeric_limits<double>::quiet_NaN();
}

void AMAEventWindow::onNewValue(const DateTime& datetime, const double& value)
{
    EventWindow::pushNewValue(datetime, value);

    if (!std::isnan(value) && windowFull()) {
        if (std::isnan(m_value)) {
            m_value = value;
        } else {
            double volativity = 0;
            for (int i = 0; i < m_effRatioLen; i++) {
                volativity += fabs(getValues()[i] - getValues()[i + 1]);
            }

            double direction = fabs(getValues()[0] - getValues()[m_effRatioLen]);
            double effRatio = 0;
            if (volativity > 0) {
                effRatio = direction / volativity;
            }

            double smooth = effRatio * (m_fastSc - m_slowSc) + m_slowSc;
            double constant = smooth * smooth;

            m_value = m_value + constant * (value - m_value);
        }
    }

}

AMA::AMA()
{

}

double AMAEventWindow::getValue() const
{
    return m_value;
}

void AMA::init(DataSeries& dataSeries, int effRatioLen, int fastAvgLen, int slowAvgLen, int maxLen)
{
    m_amaEventWnd.init(effRatioLen, fastAvgLen, slowAvgLen);

    EventBasedFilter<double, double>::init(&dataSeries, &m_amaEventWnd, maxLen);
}

} // namespace xBacktest
