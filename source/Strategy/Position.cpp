#include <cassert>
#include <algorithm>
#include "Position.h"
#include "PositionImpl.h"
#include "Runtime.h"
#include "Vector.h"

namespace xBacktest
{

////////////////////////////////////////////////////////////////////////////////
Position::Position()
{
    m_implementor = new PositionImpl();
}

Position::~Position()
{
    delete m_implementor;
    m_implementor = nullptr;
}

PositionImpl* Position::getImplementor() const
{
    return m_implementor;
}

unsigned long Position::getId() const
{
    return m_implementor->getId();
}

unsigned long Position::getEntryId() const
{
    return m_implementor->getEntryId();
}

const DateTime& Position::getEntryDateTime() const
{
    return m_implementor->getEntryDateTime();
}

const DateTime& Position::getExitDateTime() const
{
    return m_implementor->getExitDateTime();
}

const DateTime& Position::getLastDateTime() const
{
    return m_implementor->getLastDateTime();
}

int Position::getExitType() const
{
    return m_implementor->getExitType();
}

double Position::getAvgFillPrice() const
{
    return m_implementor->getAvgFillPrice();
}

int Position::getShares() const
{
    return m_implementor->getShares();
}

void Position::getTransactionRecords(TransactionList& outRecords)
{
    return m_implementor->getTransactionRecords(outRecords);
}

void Position::getSubPositions(SubPositionList& outPos)
{
    return m_implementor->getSubPositions(outPos);
}

double Position::getReturn(bool includeCommissions)
{
    return m_implementor->getReturn(includeCommissions);
}

double Position::getPnL(bool includeCommissions)
{
    return m_implementor->getPnL(includeCommissions);
}

double Position::getCommissions() const
{
    return m_implementor->getCommissions();
}

double Position::getSlippages() const
{
    return m_implementor->getSlippages();
}

double Position::getRealizedPnL() const
{
    return m_implementor->getRealizedPnL();
}

bool Position::entryActive()
{
    return m_implementor->entryActive();
}

bool Position::entryFilled()
{
    return m_implementor->entryFilled();
}

bool Position::exitActive(int subPosId)
{
    return m_implementor->exitActive(subPosId);
}

bool Position::exitFilled()
{
    return m_implementor->exitFilled();
}

const string& Position::getInstrument() const
{
    return m_implementor->getInstrument();
}

unsigned long Position::getDuration() const
{
    return m_implementor->getDuration();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Position::close(int subPosId)
{
    if (m_implementor->isLong()) {
        m_implementor->close(SignalType::ExitLong, subPosId, 0, false);
    } else {
        m_implementor->close(SignalType::ExitShort, subPosId, 0, false);
    }
}

void Position::closeImmediately(int subPosId, double price)
{
    if (m_implementor->isLong()) {
        m_implementor->close(SignalType::ExitLong, subPosId, price, true);
    } else {
        m_implementor->close(SignalType::ExitShort, subPosId, price, true);
    }
}

bool Position::isOpen() const
{
    return m_implementor->isOpen();
}

bool Position::isLong() const
{
    return m_implementor->isLong();
}

void Position::setStopLossAmount(double amount, int subPosId)
{
    return m_implementor->setStopLossAmount(amount, subPosId);
}

void Position::setStopLossPercent(double pct, int subPosId)
{
    return m_implementor->setStopLossPercent(pct, subPosId);
}

void Position::setPercentTrailing(double amount, double percentage)
{
    return m_implementor->setPercentTrailing(amount, percentage);
}

void Position::setStopProfitPct(double returns, int subPosId)
{
    return m_implementor->setStopProfitPct(returns, subPosId);
}

void Position::setTrailingStop(double returns, double drawdown, int subPosId)
{
    return m_implementor->setTrailingStop(returns, drawdown, subPosId);
}

} // namespace xBacktest