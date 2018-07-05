#ifndef BROKER_H
#define BROKER_H

#include "Observer.h"
#include "Order.h"
#include "Event.h"

namespace xBacktest
{

// Base class for brokers.
// This is a base class and should not be used directly.
class BaseBroker
{
public:
	BaseBroker();

    virtual ~BaseBroker();
	
    void notifyOrderEvent(OrderEvent& orderEvent);

    void notifyNewTradingDayEvent(const DateTime& lastDateTime, const DateTime& currDateTime);

    // Handlers should expect 2 parameters:
    // 1: broker instance
    // 2: OrderEvent instance
	Event& getOrderUpdatedEvent();

    Event& getNewTradingDayEvent();

	// Returns the number of shares for an instrument.
	virtual int getShares(const string& instrument) const = 0;

	// Returns a dictionary that maps instruments to shares.
	virtual void getPositions() = 0;

	// Submits an order.
    // @param order: The order to submit.
    // 
    // NOTE:
    // * After this call the order is in SUBMITTED state and an event is not triggered for this transition.
    // * Calling this twice on the same order will raise an exception.
	virtual void placeOrder(const Order& order) = 0;

	// Requests an order to be canceled. If the order is filled an Exception is raised.
    // @param orderId: The order id to cancel.
	virtual void cancelOrder(unsigned long orderId) = 0;

private:
	Event m_orderEvent;
    // The trading day is the time span that a particular stock/futures exchange is open.
    Event m_newTradingDayEvent;
};

} // namespace xBacktest

#endif // BROKER_H