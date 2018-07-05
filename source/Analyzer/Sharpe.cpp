#include <numeric>
#include <algorithm>
#include "Strategy.h"
#include "DataSeries.h"
#include "Sharpe.h"

namespace xBacktest
{

// @param returns: The returns.
// @param riskFreeRate: The risk free rate per annum.
// @param tradingPeriods: The number of trading periods per annum.
// @param annualized: True if the sharpe ratio should be annualized.
//  * If using daily bars, tradingPeriods should be set to 252.
//  * If using hourly bars (with 6.5 trading hours a day) then tradingPeriods should be set to 252 * 6.5 = 1638.
static double sharpe_ratio(vector<double>& returns, 
                double riskFreeRate, 
                int tradingPeriods, 
                bool annualized = true)
{
    double ret = 0.0;

    double sum = std::accumulate(returns.begin(), returns.end(), 0.0);
    double mean = sum / returns.size();

    double sq_sum = std::inner_product(returns.begin(), returns.end(), returns.begin(), 0.0);
    double stdev = std::sqrt(sq_sum / returns.size() - mean * mean);

    // From http://en.wikipedia.org/wiki/Sharpe_ratio: if Rf is a constant risk-free return throughout the period,
    // then stddev(R - Rf) = stddev(R).
    double volatility = stdev;

    if (volatility != 0) {
        double rfPerReturn = riskFreeRate / float(tradingPeriods);
        double avgExcessReturns = mean - rfPerReturn;
        ret = avgExcessReturns / volatility;

        if (annualized) {
            ret = ret * sqrt((double)tradingPeriods);
        }
    }

    return ret;
}

static double sharpe_ratio2(vector<double>& returns, 
            double riskFreeRate, 
            DateTime firstDateTime, 
            DateTime lastDateTime, 
            bool annualized = true)
{
    double ret = 0.0;

    // From http://en.wikipedia.org/wiki/Sharpe_ratio:
    // if Rf is a constant risk-free return throughout the period, then stddev(R - Rf) = stddev(R).

    double sum = std::accumulate(returns.begin(), returns.end(), 0.0);
    double mean = sum / returns.size();

    double sq_sum = std::inner_product(returns.begin(), returns.end(), returns.begin(), 0.0);
    double stdev = std::sqrt(sq_sum / returns.size() - mean * mean);

    double volatility = stdev;

    if (volatility != 0) {
        // We use 365 instead of 252 because we wan't the diff from 1/1/xxxx to 12/31/xxxx to be 1 year.
        double yearsTraded = ((lastDateTime - firstDateTime).days() + 1) / 365.0;
        double riskFreeRateForPeriod = riskFreeRate * yearsTraded;
        double rfPerReturn = riskFreeRateForPeriod / double(returns.size());

        double avgExcessReturns = mean - rfPerReturn;
        ret = avgExcessReturns / volatility;
        if (annualized) {
            ret = ret * sqrt(returns.size() / yearsTraded);
        }
    }

    return ret;
}

SharpeRatio::SharpeRatio(bool useDailyReturns /* = true */)
{
    m_useDailyReturns = useDailyReturns;

    m_currentDate.markInvalid();
    m_firstDateTime.markInvalid();
}

const vector<double>& SharpeRatio::getReturns() const
{
    return m_returns;
}

void SharpeRatio::beforeAttach(BacktestingBroker& broker)
{
    // Get or create a shared ReturnsAnalyzerBase
    ReturnsAnalyzerBase* analyzer = ReturnsAnalyzerBase::getOrCreateShared(broker);
    analyzer->getEvent().subscribe(this);
}

void SharpeRatio::onEvent(int type, const DateTime& datetime, const void *context)
{
    if (type == Event::EvtNewReturns) {
        Event::Context* ctx = (Event::Context *)context;
        ReturnsAnalyzerBase* returnsAnalyzerBase = (ReturnsAnalyzerBase*)(ctx->data);
        onReturns(ctx->datetime, *returnsAnalyzerBase);
    }
}

void SharpeRatio::onReturns(const DateTime& dateTime, ReturnsAnalyzerBase& returnsAnalyzerBase)
{
    double netReturn = returnsAnalyzerBase.getNetReturn();
    if (m_useDailyReturns) {
        // Calculate daily returns.
        if (dateTime.date() == m_currentDate) {
            m_returns.front() = (1 + m_returns[0]) * (1 + netReturn) - 1;
        } else {
            m_currentDate = dateTime.date();
            m_returns.push_back(netReturn);
        }
    } else {
        m_returns.push_back(netReturn);
        if (!m_firstDateTime.isValid()) {
            m_firstDateTime = dateTime;
        }
        m_lastDateTime = dateTime;
    }
}

double SharpeRatio::getSharpeRatio(double riskFreeRate, bool annualized)
{
    if (m_useDailyReturns) {
        return sharpe_ratio(m_returns, riskFreeRate, 252, annualized);
    } else {
        return sharpe_ratio2(m_returns, riskFreeRate, m_firstDateTime, m_lastDateTime, annualized);
    }
}

} // namespace xBacktest
