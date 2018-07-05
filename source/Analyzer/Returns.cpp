#include <cmath>
#include <cassert>
#include "Utils.h"
#include "Event.h"
#include "Returns.h"
#include "Runtime.h"
#include "Bar.h"

namespace xBacktest
{

ReturnsAnalyzerBase::ReturnsAnalyzerBase()
{
    m_netRet = 0;
    m_cumRet = 0;
    m_event.setType(Event::EvtNewReturns);
}

ReturnsAnalyzerBase* ReturnsAnalyzerBase::getOrCreateShared(BacktestingBroker& broker)
{
    // Get or create the shared ReturnsAnalyzerBase.
    StgyAnalyzer *ret = broker.getNamedAnalyzer("ReturnsAnalyzerBase");
    
    assert(ret != nullptr);

    return (ReturnsAnalyzerBase*)ret;
}

void ReturnsAnalyzerBase::attached(BacktestingBroker& broker)
{
    m_lastPortfolioValue = broker.getEquity();
}

Event& ReturnsAnalyzerBase::getEvent()
{
    return m_event;
}

double ReturnsAnalyzerBase::getNetReturn() const
{
    return m_netRet;
}

double ReturnsAnalyzerBase::getCumulativeReturn() const
{
    return m_cumRet;
}

double ReturnsAnalyzerBase::getEquity() const
{
    return m_equity;
}

void ReturnsAnalyzerBase::beforeOnBar(BacktestingBroker& broker, const Bar& bar)
{
    double currentPortfolioValue = broker.getEquity();
    double netReturn = (currentPortfolioValue - m_lastPortfolioValue) / float(m_lastPortfolioValue);
    m_lastPortfolioValue = currentPortfolioValue;
    m_equity = currentPortfolioValue;

    m_netRet = netReturn;

    // Calculate cumulative return.
    // 
    // ASSUMPTION:
    // Initial portfolio value:   initPV
    // Last portfolio value:      lastPV
    // Current portfolio vale:    currPV
    // Current cumulative return: cumRet
    // Last cumulative return:    lastCumRet
    // Current net return:        netRet (equal to (currPV - lastPV) / lastPV)
    // 
    // THEN:
    // currPV = initPV * (1 + cumRet) = lastPV * (1 + netRet)
    //                   (1 + cumRet) = (lastPV / initPV) * (1 + netRet)
    //                lastPV / initPV = (1 + cumRet) / (1 + netRet)
    //            lastPV / initPV - 1 = (1 + cumRet) / (1 + netRet) - 1
    //   (lastPV - initPV) / (initPV) = (1 + cumRet) / (1 + netRet) - 1
    //                     lastCumRet = (1 + cumRet) / (1 + netRet) - 1
    //                 lastCumRet + 1 = (1 + cumRet) / (1 + netRet)
    //                     1 + cumRet = (1 + lastCumRet) * ( 1 + netRet)
    //                         cumRet = (1 + lastCumRet) * (1 + netRet) - 1
    m_cumRet = (1 + m_cumRet) * (1 + netReturn) - 1;

    // Notify that new returns are available.
    Event::Context ctx;
    ctx.datetime = bar.getDateTime();
    ctx.data     = this;
    m_event.emit(bar.getDateTime(), &ctx);
}

////////////////////////////////////////////////////////////////////////////////
Returns::Returns()
{

}

void Returns::beforeAttach(BacktestingBroker& broker)
{
    // Get or create a shared ReturnsAnalyzerBase
    ReturnsAnalyzerBase* analyzer = ReturnsAnalyzerBase::getOrCreateShared(broker);
    if (analyzer != nullptr) {
        analyzer->getEvent().subscribe(this);
    }
}

void Returns::attached(BacktestingBroker& broker)
{

}

void Returns::onReturns(const DateTime& datetime, ReturnsAnalyzerBase& returnsAnalyzerBase)
{
    Ret ret;
    ret.datetime = datetime;
    ret.ret = returnsAnalyzerBase.getNetReturn();
    if (m_netReturns.size() > 0) {
        if (datetime < m_netReturns.back().datetime) {
            ASSERT(false, string("Datetime wrap back:") + datetime.toString());
        }
        // Update last one.
        if (m_netReturns.back().datetime == datetime) {
            m_netReturns.back() = ret;
        } else {
            m_netReturns.push_back(ret);
        }
    } else {
        m_netReturns.push_back(ret);
    }
    
    ret.ret = returnsAnalyzerBase.getCumulativeReturn();
    if (m_cumReturns.size() > 0) {
        if (datetime < m_cumReturns.back().datetime) {
            ASSERT(false, string("Datetime wrap back:") + datetime.toString());
        }
        if (m_cumReturns.back().datetime == datetime) {
            m_cumReturns.back() = ret;
        } else {
            m_cumReturns.push_back(ret);
        }
    } else {
        m_cumReturns.push_back(ret);
    }

    Equity equity;
    equity.datetime = datetime;
    equity.equity   = returnsAnalyzerBase.getEquity();
    if (m_equities.size() > 0) {
        if (datetime < m_equities.back().datetime) {
            ASSERT(false, string("Datetime wrap back:") + datetime.toString());
        }
        if (m_equities.back().datetime == datetime) {
            m_equities.back() = equity;
        } else {
            m_equities.push_back(equity);
        }
    } else {
        m_equities.push_back(equity);
    }
}

void Returns::onEvent(int type, const DateTime& datetime, const void *context)
{
    if (type == Event::EvtNewReturns) {
        Event::Context* ctx = (Event::Context *)context;
        ReturnsAnalyzerBase* returnsAnalyzerBase = (ReturnsAnalyzerBase*)(ctx->data);
        onReturns(ctx->datetime, *returnsAnalyzerBase);
    }
}

const vector<Returns::Ret>& Returns::getReturns() const
{
    return m_netReturns;
}

const vector<Returns::Ret>& Returns::getCumulativeReturns() const
{
    return m_cumReturns;
}

const vector<Returns::Equity>& Returns::getEquities() const
{
    return m_equities;
}

} // namespace xBacktest