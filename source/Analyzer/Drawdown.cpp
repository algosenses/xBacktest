#include "Utils.h"
#include "Drawdown.h"
#include "Backtesting.h"
#include "Strategy.h"
#include "Runtime.h"

namespace xBacktest
{

DrawDownHelper::DrawDownHelper()
{
    m_highWatermark = std::numeric_limits<double>::quiet_NaN();
    m_lowWatermark  = std::numeric_limits<double>::quiet_NaN();

    m_highDateTime.markInvalid();
    m_lastDateTime.markInvalid();
}

int DrawDownHelper::getDuration()
{
    return (m_lastDateTime - m_highDateTime).days();
}

double DrawDownHelper::getMaxDrawDown(bool usePercentage)
{
    if (usePercentage) {
        return (m_lowWatermark - m_highWatermark) / float(m_highWatermark);
    } else {
        return (m_lowWatermark - m_highWatermark);
    }
}

double DrawDownHelper::getCurrentDrawDown(bool usePercentage)
{
    if (usePercentage) {
        return (m_lastLow - m_highWatermark) / float(m_highWatermark);
    } else {
        return (m_lastLow - m_highWatermark);
    }
}

const DateTime& DrawDownHelper::getHighDateTime() const
{
    return m_highDateTime;
}

const DateTime& DrawDownHelper::getLastDateTime() const
{
    return m_lastDateTime;
}

void DrawDownHelper::update(const DateTime& dateTime, double equity)
{
    m_lastLow      = equity;
    m_lastDateTime = dateTime;

    if (std::isnan(m_highWatermark) || equity >= m_highWatermark) {
        m_highWatermark = equity;
        m_lowWatermark  = equity;
        m_highDateTime  = dateTime;
    } else {
        m_lowWatermark = (std::min)(m_lowWatermark, equity);
    }
}

////////////////////////////////////////////////////////////////////////////////
DrawDownCalculator::DrawDownCalculator()
{
    reset();
}

void DrawDownCalculator::reset()
{
    m_initialized = false;
    m_maxDD = 0;
    m_maxDDPercentage = 0;
    m_longestDDDuration = 0;

    m_highWatermark = std::numeric_limits<double>::quiet_NaN();
    m_lowWatermark  = std::numeric_limits<double>::quiet_NaN();

    m_highDateTime.markInvalid();
    m_lastDateTime.markInvalid();
}

bool DrawDownCalculator::calculate(const vector<Returns::Ret>& returns)
{
    if (returns.size() == 0) {
        return false;
    }

    for (size_t i = 0; i < returns.size(); i++) {
        update(returns[i].datetime, returns[i].ret);
    }

    return true;
}

bool DrawDownCalculator::calculate(const vector<Returns::Equity>& equities)
{
    if (equities.size() == 0) {
        return false;
    }

    for (size_t i = 0; i < equities.size(); i++) {
        update(equities[i].datetime, equities[i].equity);
    }

    return true;
}

bool DrawDownCalculator::calculate(const vector<Trades::TradeProfit>& data, double initial)
{
    if (data.size() == 0) {
        return false;
    }

    for (size_t i = 0; i < data.size(); i++) {
        update(data[i].datetime, data[i].value + initial);
    }

    return true;
}

void DrawDownCalculator::update(const DateTime& dt, double equity)
{
    if (!m_initialized) {
        m_initialized    = true;
        m_highDateTime   = dt;
        m_lastDateTime   = dt;
        m_highWatermark  = equity;
        m_lowWatermark   = equity;
        m_maxDDBegin     = dt;
        m_maxDDEnd       = dt;
        m_longestDDBegin = dt;
        m_longestDDEnd   = dt;
    } else {
        if (equity >= m_highWatermark) {
            int duration = (dt - m_highDateTime).days();
            // Must use 'greater than or equal to' here.
            if (duration >= m_longestDDDuration) {
                m_longestDDDuration = duration;
                m_longestDDBegin = m_highDateTime;
                m_longestDDEnd = dt;
            }

            m_lowWatermark  = equity;
            m_highWatermark = equity;
            m_highDateTime  = dt;
                        
        } else if (equity < m_lowWatermark) {
            m_lowWatermark = equity;
            double drawdown = (m_highWatermark - m_lowWatermark);
            // Use 'greater than' here.
            if (drawdown > m_maxDD) {
                m_maxDD = drawdown;
                m_maxDDPercentage = (m_highWatermark - m_lowWatermark) / m_highWatermark;
                m_maxDDBegin = m_highDateTime;
                m_maxDDEnd = dt;
            }

            int duration = (dt - m_highDateTime).days();
            if (duration >= m_longestDDDuration) {
                m_longestDDDuration = duration;
                m_longestDDBegin = m_highDateTime;
                m_longestDDEnd = dt;
            }
        }

        m_lastDateTime = dt;
    }
}

double DrawDownCalculator::getMaxDrawDown(bool usePercentage)
{
    if (usePercentage) {
        return abs(m_maxDDPercentage);
    } else {
        return abs(m_maxDD);
    }
}

const DateTime& DrawDownCalculator::getMaxDrawDownBegin() const
{
    return m_maxDDBegin;
}

const DateTime& DrawDownCalculator::getMaxDrawDownEnd() const
{
    return m_maxDDEnd;
}

double DrawDownCalculator::getLongestDrawDownDuration()
{
    return m_longestDDDuration;
}

const DateTime& DrawDownCalculator::getLongestDDBegin() const
{
    return m_longestDDBegin;
}

const DateTime& DrawDownCalculator::getLongestDDEnd() const
{
    return m_longestDDEnd;
}

////////////////////////////////////////////////////////////////////////////////
DrawDown::DrawDown()
{
    m_calculator.reset();
}

double DrawDown::calculateEquity(BacktestingBroker& broker)
{
    return broker.getEquity();
}

void DrawDown::beforeOnBar(BacktestingBroker& broker, const Bar& bar)
{
    double equity = calculateEquity(broker);
    m_calculator.update(bar.getDateTime(), equity);
}

double DrawDown::getMaxDrawDown(bool usePercentage)
{
    return m_calculator.getMaxDrawDown(usePercentage);
}

const DateTime& DrawDown::getMaxDrawDownBegin() const
{
    return m_calculator.getMaxDrawDownBegin();
}

const DateTime& DrawDown::getMaxDrawDownEnd() const
{
    return m_calculator.getMaxDrawDownEnd();
}

double DrawDown::getLongestDrawDownDuration()
{
    return m_calculator.getLongestDrawDownDuration();
}

const DateTime& DrawDown::getLongestDDBegin() const
{
    return m_calculator.getLongestDDBegin();
}

const DateTime& DrawDown::getLongestDDEnd() const
{
    return m_calculator.getLongestDDEnd();
}

} // namespace xBacktest
