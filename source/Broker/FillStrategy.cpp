#include <cassert>
#include "FillStrategy.h"
#include "Backtesting.h"
#include "Logger.h"
#include "Errors.h"

namespace xBacktest
{

void FillStrategy::onBar(const DateTime& datetime, const Bar& bar)
{
    /* empty */
}

void FillStrategy::onOrderFilled(const Order& order)
{
    /* empty */
}

////////////////////////////////////////////////////////////////////////////////
// Order filling strategies
//
// Returns the trigger price for a Stop or StopLimit order, or None if the stop price was not yet penetrated.
static double get_stop_price_trigger(Order::Action action, double stopPrice, const Bar& bar)
{
    double ret = 0.0;
    double open = bar.getOpen();
    double high = bar.getHigh();
    double  low = bar.getLow();

    // If the bar is above the stop price, use the open price.
    // If the bar includes the stop price, use the open price or the stop price. Whichever is better.
    if (action == Order::Action::BUY || action == Order::Action::BUY_TO_COVER) {
        if (low > stopPrice) {
            ret = open;
        } else if (stopPrice < high || fabs(stopPrice - high) < 0.0000001) {
            if (open > stopPrice) {   // The stop price was penetrated on open.
                ret = open;
            } else {
                ret = stopPrice;
            }
        } else {
            ret = stopPrice;
        }
        // If the bar is below the stop price, use the open price.
        // If the bar includes the stop price, use the open price or the stop price. Whichever is better.
    } else if (action == Order::Action::SELL || action == Order::Action::SELL_SHORT) {
        if (high < stopPrice) {
            ret = open;
        } else if (stopPrice > low || fabs(stopPrice - low) < 0.0000001) {
            if (open < stopPrice) {   // The stop price was penetrated on open.
                ret = open;
            } else {
                ret = stopPrice;
            }
        } else {
            ret = stopPrice;
        }
    } else { // Unknown action
        ASSERT(false, "Unknown action");
    }

    if (ret == 0.0) {
        ASSERT(false, "Can not trigger STOP price.");
    }

    return ret;
}

// Returns the trigger price for a Limit or StopLimit order, or None if the limit price was not yet penetrated.
static double get_limit_price_trigger(
    Order::Action action, 
    double limitPrice, 
    const Bar& bar)
{
    double ret = 0.0;
    double open = bar.getOpen();
    double high = bar.getHigh();
    double low  = bar.getLow();

    // If the bar is below the limit price, use the open price.
    // If the bar includes the limit price, use the open price or the limit price.
    if (action == Order::Action::BUY || action == Order::Action::BUY_TO_COVER) {
        if (high < limitPrice) {
            ret = open;
        } else if (limitPrice >= low) {
            if (open < limitPrice) {    // The limit price was penetrated on open.
                ret = open;
            } else {
                ret = limitPrice;
            }
        }
    } else if (action == Order::Action::SELL || action == Order::Action::SELL_SHORT) {
        // If the bar is above the limit price, use the open price.
        // If the bar includes the limit price, use the open price or the limit price.
        if (low > limitPrice) {
            ret = open;
        } else if (limitPrice <= high) {
            if (open > limitPrice) {    // The limit price was penetrated on open.
                ret = open;
            } else {
                ret = limitPrice;
            }
        }
    } else {
        assert(false);
    }

    if (ret == 0.0) {
        ASSERT(false, "Can not trigger LIMIT price.");
    }

    return ret;
}

////////////////////////////////////////////////////////////////////////////////
DefaultStrategy::DefaultStrategy(double volumeLimit)
{
    assert(volumeLimit >= 0 && volumeLimit <= 1);

    m_volumeLimit = volumeLimit;
    m_volumeLeft.clear();
}

void DefaultStrategy::onBar(const DateTime& datetime, const Bar& bar)
{
    if (m_volumeLimit == 0) {
        return;
    }

    // Update the volume available for each instrument.
    const string& instrument = bar.getInstrument();

    if (bar.getResolution() == Bar::TICK) {
        m_volumeLeft[instrument] = bar.getVolume();
    } else if (m_volumeLimit > 0) {
        m_volumeLeft[instrument] = (long long)(bar.getVolume() * m_volumeLimit);
    }
}

void DefaultStrategy::onOrderFilled(const Order& order)
{
    // Update the volume left.
    if (m_volumeLimit != 0) {
        m_volumeLeft[order.getInstrument()] -= order.getExecutionInfo().getQuantity();
    }
}

void DefaultStrategy::setVolumeLimit(double volumeLimit)
{
    m_volumeLimit = volumeLimit;
}

int DefaultStrategy::calculateFillSize(const Order& order, BaseBroker& broker, const Bar& bar)
{
    int ret = 0;
    long long volumeLeft = 0;
    // If m_volumeLimit == 0 then allow all the order to get filled.
    if (m_volumeLimit > 0) {
        map<string, long long>::const_iterator it = m_volumeLeft.find(order.getInstrument());
        if (it == m_volumeLeft.end()) {
            return 0;
        }
        volumeLeft = it->second;
    } else {
        volumeLeft = order.getRemaining();
    }

    BacktestingBroker& backtestingBroker = (BacktestingBroker&)broker;
    if (!backtestingBroker.getAllowFractions()) {
        volumeLeft = (int)volumeLeft;
    }

    if (!order.getAllOrNone()) {
        ret = volumeLeft < order.getRemaining() ? volumeLeft : order.getRemaining();
    } else if (order.getRemaining() <= volumeLeft) {
        ret = order.getRemaining();
    }

    return ret;
}

FillInfo DefaultStrategy::fillMarketOrder(Order* orderRef, BaseBroker& broker, const Bar& bar)
{
    int fillSize = calculateFillSize(*orderRef, broker, bar);
    if (fillSize == 0) {
        return FillInfo(0, 0);
    }

    BacktestingBroker& backtestBroker = (BacktestingBroker&) broker;
    double price = 0.0;
    if (orderRef->getFillOnClose()) {
        price = bar.getClose();
    } else {
        price = bar.getOpen();
    }

    REQUIRE(price > 0.0, 
        "Negative price " << price << " [" << bar.getInstrument() << ", " << bar.getDateTime().toString() << "]"
        ". If you are backtesting STOCK strategy, please use Backward-Adjusted-Price.");
    
    return FillInfo(price, fillSize);
}

FillInfo DefaultStrategy::fillLimitOrder(Order* orderRef, BaseBroker& broker, const Bar& bar)
{
    int fillSize = calculateFillSize(*orderRef, broker, bar);
    if (fillSize == 0) {
        return FillInfo(0, 0);
    }

    BacktestingBroker& backtestBroker = (BacktestingBroker&) broker;
    double price = get_limit_price_trigger(orderRef->getAction(), orderRef->getLimitPrice(), bar);
    if (price > 0.0) {
        return FillInfo(price, fillSize);
    }

    return FillInfo(0, 0);
}

FillInfo DefaultStrategy::fillStopOrder(Order* orderRef, BaseBroker& broker, const Bar& bar)
{
    BacktestingBroker& backtestBroker = (BacktestingBroker&) broker;
    // First check if the stop price was hit so the market order becomes active.
    double stopPriceTrigger = 0;
    if (!orderRef->getStopHit()) {
        stopPriceTrigger = get_stop_price_trigger(orderRef->getAction(), orderRef->getStopPrice(), bar);
        orderRef->setStopHit(stopPriceTrigger != 0);
    }

    // If the stop price was hit, check if we can fill the market order.
    if (orderRef->getStopHit()) {
        int fillSize = calculateFillSize(*orderRef, broker, bar);
        if (fillSize == 0) {
            Logger_Warn() << "Not enough volume to fill " << orderRef->getInstrument() << " stop order [" << orderRef->getId() << "] for " << orderRef->getRemaining() << " share/s.";
            return FillInfo();
        }

        // If we just hit the stop price we'll use it as the fill price.
        // For the remaining bars we'll use the open price.
        double price = 0;
        if (stopPriceTrigger != 0) {
            price = stopPriceTrigger;
        } else {
            price = bar.getOpen();
        }

        return FillInfo(price, fillSize);
    }

    return FillInfo();
}

FillInfo DefaultStrategy::fillStopLimitOrder(Order* orderRef, BaseBroker& broker, const Bar& bar)
{    
    BacktestingBroker& backtestBroker = (BacktestingBroker&) broker;
    // First check if the stop price was hit so the limit order becomes active.
    double stopPriceTrigger = 0;
    if (!orderRef->getStopHit()) {
        stopPriceTrigger = get_stop_price_trigger(orderRef->getAction(), orderRef->getStopPrice(), bar);
        orderRef->setStopHit(stopPriceTrigger != 0);
    }

    // If the stop price was hit, check if we can fill the limit order.
    if (orderRef->getStopHit()) {
        int fillSize = calculateFillSize(*orderRef, broker, bar);
        if (fillSize == 0) {
            Logger_Warn() << "Not enough volume to fill " << orderRef->getInstrument() << " stop limit order [" << orderRef->getId() << "] for " << orderRef->getRemaining() << " share/s.";
            return FillInfo();
        }

        double price = get_limit_price_trigger(orderRef->getAction(), orderRef->getLimitPrice(), bar);
        if (price != 0) {
            // If we just hit the stop price, we need to make additional checks.
            if (stopPriceTrigger != 0) {
                if (orderRef->isBuy()) {
                    // If the stop price triggered is lower than the limit price, then use that one. Else use the limit price.
                    price = min(stopPriceTrigger, orderRef->getLimitPrice());
                } else {
                    // If the stop price triggered is greater than the limit price, then use that one. Else use the limit price.
                    price = max(stopPriceTrigger, orderRef->getLimitPrice());
                }
            }

            return FillInfo(price, fillSize);
        }
    }

    return FillInfo();
}

////////////////////////////////////////////////////////////////////////////////
TickStrategy::TickStrategy()
{
}

void TickStrategy::onBar(const DateTime& datetime, const Bar& bar)
{
    return;
}

FillInfo TickStrategy::fillMarketOrder(Order* orderRef, BaseBroker& broker, const Bar& bar)
{
    return FillInfo(bar.getClose(), orderRef->getQuantity());
}

FillInfo TickStrategy::fillLimitOrder(Order* orderRef, BaseBroker& broker, const Bar& bar)
{
    ASSERT(false, "Limit order is not support on tick data.");
    return FillInfo();
}

FillInfo TickStrategy::fillStopOrder(Order* orderRef, BaseBroker& broker, const Bar& bar)
{
    return FillInfo(bar.getClose(), orderRef->getQuantity());
}

FillInfo TickStrategy::fillStopLimitOrder(Order* orderRef, BaseBroker& broker, const Bar& bar)
{
    ASSERT(false, "Stop limit order is not support on tick data.");
    return FillInfo();
}

} // namespace xBacktest