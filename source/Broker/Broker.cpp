#include "Broker.h"

namespace xBacktest
{

BaseBroker::BaseBroker()
{
    m_orderEvent.setType(Event::EvtOrderUpdate);
    m_newTradingDayEvent.setType(Event::EvtNewTradingDay);
}

BaseBroker::~BaseBroker()
{

}

void BaseBroker::notifyOrderEvent(OrderEvent& orderEvent)
{
    m_orderEvent.emit(DateTime(), &orderEvent);
}

void BaseBroker::notifyNewTradingDayEvent(const DateTime& lastDateTime, const DateTime& currDateTime)
{
    m_newTradingDayEvent.emit(currDateTime, &lastDateTime);
}

Event& BaseBroker::getOrderUpdatedEvent()
{
	return m_orderEvent;
}

Event& BaseBroker::getNewTradingDayEvent()
{
    return m_newTradingDayEvent;
}

} // namespace xBacktest
