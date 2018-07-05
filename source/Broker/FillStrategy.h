#ifndef FILL_STRATEGY_H
#define FILL_STRATEGY_H

#include "DateTime.h"
#include "Bar.h"
#include "Order.h"

namespace xBacktest
{

class Order;
class BaseBroker;

class FillInfo
{
public:
    FillInfo() 
    { 
        m_quantity = 0;
        m_price    = 0.0; 
    }

    FillInfo(double price, int quantity) 
        : m_price(price), m_quantity(quantity) 
    { }
    
    double getPrice() const
    { 
        return m_price; 
    }
    
    int getQuantity() const
    { 
        return m_quantity; 
    }

private:
    int    m_quantity;
    double m_price;
};

// Base class for order filling strategies.
class FillStrategy
{
public:
    virtual ~FillStrategy() {}
    // Override (optional) to get notified when the broker is about to process new bars.
	virtual void onBar(const DateTime& datetime, const Bar& bar);

    // Override (optional) to get notified when an order was filled, or partially filled.
	virtual void onOrderFilled(const Order &order);

    // Override to return the fill price and quantity for a market order or None if the order can't be filled at the given time.
    virtual FillInfo fillMarketOrder(Order* orderRef, BaseBroker& broker, const Bar& bar) = 0;

    // Override to return the fill price and quantity for a limit order or None if the order can't be filled at the given time.
    virtual FillInfo fillLimitOrder(Order* orderRef, BaseBroker& broker, const Bar& bar) = 0;

    // Override to return the fill price and quantity for a stop order or None if the order can't be filled at the given time.
    virtual FillInfo fillStopOrder(Order* orderRef, BaseBroker& broker, const Bar& bar) = 0;

    // Override to return the fill price and quantity for a stop limit order or None if the order can't be filled at the given time.
    virtual FillInfo fillStopLimitOrder(Order* orderRef, BaseBroker& broker, const Bar& bar) = 0;
};

/*
    Default fill strategy.

    @param volumeLimit: The proportion of the volume that orders can take up in a bar. Must be > 0 and <= 1.
    @type  volumeLimit: float.

    This strategy works as follows:

    * A : `MarketOrder` is always filled using the open/close price.
    * A : `LimitOrder` will be filled like this:
        * If the limit price was penetrated with the open price, then the open price is used.
        * If the bar includes the limit price, then the limit price is used.
        * Note that when buying the price is penetrated if it gets <= the limit price, and when selling the price is penetrated
          if it gets >= the limit price
    * A : `StopOrder` will be filled like this:
        * If the stop price was penetrated with the open price, then the open price is used.
        * If the bar includes the stop price, then the stop price is used.
        * Note that when buying the price is penetrated if it gets >= the stop price, and when selling the price is penetrated
          if it gets <= the stop price
    * A : `StopLimitOrder` will be filled like this:
        * If the stop price was penetrated with the open price, or if the bar includes the stop price, then the limit order becomes active.
        * If the limit order is active:
        * If the limit order was activated in this same bar and the limit price is penetrated as well, then the best between
          the stop price and the limit fill price (as described earlier) is used.
        * If the limit order was activated at a previous bar then the limit fill price (as described earlier) is used.

      NOTE:
        * This is the default strategy used by the Broker.
        * If volumeLimit is 0.25, and a certain bar's volume is 100, then no more than 25 shares can be used by all
          orders that get processed at that bar.
        * If using trade bars, then all the volume from that bar can be used.
*/

/*
 * The difference between LIMIT and STOP order:
 * STOP:
 *    1. Stop can be read as "this price or worse" meaning higher for a Long Entry and Short Exit, 
 *       lower for a Short Entry and Long Exit.
 *    2. Stop orders require a reference price.
 *
 * LIMIT:
 *    1. Limit can be read as "this price or better" meaning lower for a Long Entry and Short Exit, 
 *       higher for a Short Entry and Long Exit.
 *    2. Limit orders require a reference price.
 */

#define DEFAULT_VOLUME_LIMIT      (0.25)

class DefaultStrategy : public FillStrategy
{
public:
    DefaultStrategy(double volumeLimit = 0.0);

    void onBar(const DateTime& datetime, const Bar& bar);
    void onOrderFilled(const Order& order);
    void setVolumeLimit(double volumeLimit);

    FillInfo fillMarketOrder(Order* orderRef, BaseBroker& broker, const Bar& bar);
    FillInfo fillLimitOrder(Order* orderRef, BaseBroker& broker, const Bar& bar);
    FillInfo fillStopOrder(Order* orderRef, BaseBroker& broker, const Bar& bar);
    FillInfo fillStopLimitOrder(Order* orderRef, BaseBroker& broker, const Bar& bar);

private:
    int calculateFillSize(const Order& order, BaseBroker& broker, const Bar& bar);

private:
    double m_volumeLimit;
    map<string, long long> m_volumeLeft;
};

// Tick data based fill strategy
class TickStrategy : public FillStrategy
{
public:
    TickStrategy();

    void onBar(const DateTime& datetime, const Bar& bar);

    FillInfo fillMarketOrder(Order* orderRef, BaseBroker& broker, const Bar& bar);
    FillInfo fillLimitOrder(Order* orderRef, BaseBroker& broker, const Bar& bar);
    FillInfo fillStopOrder(Order* orderRef, BaseBroker& broker, const Bar& bar);
    FillInfo fillStopLimitOrder(Order* orderRef, BaseBroker& broker, const Bar& bar);
};

} // namespace xBacktest

#endif // FILL_STRATEGY.H
