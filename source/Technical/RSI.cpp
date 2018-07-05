#include "Utils.h"
#include "RSI.h"

namespace xBacktest
{

// RSI = 100 - 100 / (1 + RS)
// RS = Average gain / Average loss
// First Average Gain = Sum of Gains over the past 14 periods / 14
// First Average Loss = Sum of Losses over the past 14 periods / 14
// Average Gain = [(previous Average Gain) x 13 + current Gain] / 14
// Average Loss = [(previous Average Loss) x 13 + current Loss] / 14
//
// RSI is 0 when the Average Gain equals zero. Assuming a 14-period RSI, a zero RSI value means prices moved lower all
// 14 periods. There were no gains to measure.
// RSI is 100 when the Average Loss equals zero. This means prices moved higher all 14 periods.
// There were no losses to measure.
//
// If Average Loss equals zero, a "divide by zero" situation occurs for RS and RSI is set to 100 by definition.
// Similarly, RSI equals 0 when Average Gain equals zero.
//
// RSI is considered overbought when above 70 and oversold when below 30.
// These traditional levels can also be adjusted to better fit the security or analytical requirements.
// Raising overbought to 80 or lowering oversold to 20 will reduce the number of overbought/oversold readings.
// Short-term traders sometimes use 2-period RSI to look for overbought readings above 80 and oversold readings below 20.

static void gain_loss_one(double& gain, double& loss, double prevValue, double nextValue)
{
    double change = nextValue - prevValue;
    if (change < 0) {
        gain = 0;
        loss = abs(change);
    } else {
        gain = change;
        loss = 0;
    }
}

// [begin, end)
static void avg_gain_loss(double& gain, double& loss, const EventWindow<double>::Values& values, int begin, int end)
{
    int rangeLen = end - begin;
    if (rangeLen < 2) {
        return;
    }

    gain = 0;
    loss = 0;
    for (int i = begin + 1; i < end; i++) {
        double currGain;
        double currLoss;
        gain_loss_one(currGain, currLoss, values[i-1], values[i]);
        gain += currGain;
        loss += currLoss;
    }

    gain = gain / float(rangeLen-1);
    loss = loss / float(rangeLen-1);
}

static double rsi(EventWindow<double>::Values& values, int period)
{
    assert(period > 1);
    if (values.size() < period + 1) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    double avgGain, avgLoss;
    avg_gain_loss(avgGain, avgLoss, values, 0, period);

    for (int i = period; i < values.size(); i++) {
        double gain, loss;
        gain_loss_one(gain, loss, values[i-1], values[i]);
        avgGain = (avgGain * (period - 1) + gain) / float(period);
        avgLoss = (avgLoss * (period - 1) + loss) / float(period);
    }

    if (avgLoss == 0) {
        return 100;
    }
    
    double rs = avgGain / avgLoss;
    return 100 - 100 / (1 + rs);
}

void RSIEventWindow::init(int period)
{
    assert(period > 1);
    // We need N + 1 samples to calculate N averages because they are calculated based on the diff with previous values.
    EventWindow<double, double>::init(period + 1);
    m_value    = std::numeric_limits<double>::quiet_NaN();
    m_prevGain = std::numeric_limits<double>::quiet_NaN();
    m_prevLoss = std::numeric_limits<double>::quiet_NaN();
    m_period   = period;
}

void RSIEventWindow::onNewValue(const DateTime& datetime, const double& value)
{
    EventWindow<double, double>::pushNewValue(datetime, value);
    // Formula from http://stockcharts.com/school/doku.php?id=chart_school:technical_indicators:relative_strength_index_rsi
    if (!std::isnan(value) && windowFull()) {
        double avgGain, avgLoss;
        if (std::isnan(m_prevGain)) {
            assert(std::isnan(m_prevLoss));
            avg_gain_loss(avgGain, avgLoss, getValues(), 0, getValues().size());
        } else {
            // Rest of averages are smoothed
            assert(!std::isnan(m_prevLoss));
            double prevValue = getValues()[-2];
            double currValue = getValues()[-1];
            double currGain, currLoss;
            gain_loss_one(currGain, currLoss, prevValue, currValue);
            avgGain = (m_prevGain * (m_period-1) + currGain) / float(m_period);
            avgLoss = (m_prevLoss * (m_period-1) + currLoss) / float(m_period);
        }

        if (avgLoss == 0) {
            m_value = 100;
        } else {
            double rs = avgGain / avgLoss;
            m_value = 100 - 100 / (1 + rs);
        }

        m_prevGain = avgGain;
        m_prevLoss = avgLoss;
    }
}

double RSIEventWindow::getValue() const
{
    return m_value;
}

////////////////////////////////////////////////////////////////////////////////
void RSI::init(DataSeries& dataSeries, int period, int maxLen)
{
    m_rsiEventWnd.init(period);
    EventBasedFilter<double, double>::init(&dataSeries, &m_rsiEventWnd, maxLen);
}

} // namespace xBacktest
