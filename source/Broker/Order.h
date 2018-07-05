#ifndef XBACKTEST_ORDER_H
#define XBACKTEST_ORDER_H

#include <string>
#include <stdexcept>
#include "DateTime.h"

using std::string;
using std::invalid_argument;

namespace xBacktest
{

// Execution information for an order.
class DllExport OrderExecutionInfo
{
public:
    OrderExecutionInfo();
    OrderExecutionInfo(const DateTime& dt, double price, int quantity, double commission, double slippage = 0);
    OrderExecutionInfo(const OrderExecutionInfo& execInfo);
    // Returns the fill price.
    double getPrice() const;
    // Returns the quantity.
    int    getQuantity() const;
    // Returns the commission applied.
    double getCommission() const;
    // Returns the slippage.
    double getSlippage() const;
    // Returns the `datetime` when the order was executed.
    const DateTime& getDateTime() const;

private:
    double   m_price;
    int      m_quantity;
    double   m_commission;
    double   m_slippage;
    DateTime m_dateTime;
};


//######################################################################
// Orders
// http://stocks.about.com/od/tradingbasics/a/markords.htm
// http://www.interactivebrokers.com/en/software/tws/usersguidebook/ordertypes/basic_order_types.htm
//
// State chart:
// INITIAL           -> SUBMITTED
// INITIAL           -> CANCELED
// SUBMITTED         -> ACCEPTED
// SUBMITTED         -> CANCELED
// ACCEPTED          -> FILLED
// ACCEPTED          -> PARTIALLY_FILLED
// ACCEPTED          -> CANCELED
// PARTIALLY_FILLED  -> PARTIALLY_FILLED
// PARTIALLY_FILLED  -> FILLED
// PARTIALLY_FILLED  -> CANCELED

// Class for orders.
//
//	Valid type parameter values are:
//
//	Order.Type.MARKET
// A market order is an order to buy or sell an asset at the bid or offer price currently available in the marketplace. 
// When you submit a market order, you have no guarantee that the order will execute at any specific price.
//
//	Order.Type.LIMIT
// A limit order is an order to buy or sell a contract ONLY at the specified price or better.
//
//	Order.Type.STOP
// A Stop order becomes a market order to buy or sell securities or commodities once the specified stop price is attained or penetrated. 
// A Stop order is not guaranteed a specific execution price.
//
//	Order.Type.STOP_LIMIT
// A Stop Limit order is similar to a stop order in that a stop price will activate the order. However, unlike the stop order, 
// which is submitted as a market order when elected, the stop limit order is submitted as a limit order. 
//
// When you place a stop or limit order, you are telling your broker that you don't want the market price
// (the current price at which a stock is trading), but that you want the stock price to move in a certain 
// direction before your order is executed.
// 
// With a stop order, your trade will be executed only when the security you want to buy or sell reaches a 
// particular price (the stop price). Once the stock has reached this price, a stop order essentially becomes 
// a market order and is filled. For instance, if you own stock ABC, which currently trades at $20, and you 
// place a stop order to sell it at $15, your order will only be filled once stock ABC drops below $15. Also 
// known as a "stop-loss order", this allows you to limit your losses. However, this type of order can also be
// used to guarantee profits. For example, assume that you bought stock XYZ at $10 per share and now the stock
// is trading at $20 per share. Placing a stop order at $15 will guarantee profits of approximately $5 per share,
// depending on how quickly the market order can be filled.
//
// A STOP_LIMIT order is an order placed with a broker that combines the features of stop order with those of a 
// limit order. A stop-limit order will be executed at a specified price (or better) after a given stop price 
// has been reached. Once the stop price is reached, the stop-limit order becomes a limit order to buy (or sell)
// at the limit price or better.
//
//	Valid action parameter values are:
//
//	Order.Action.BUY
//	Order.Action.BUY_TO_COVER
//	Order.Action.SELL
//	Order.Action.SELL_SHORT
//
class DllExport Order 
{
public:
	class Action
	{
	public:
		enum Type_ {
			BUY          = 1,
			BUY_TO_COVER = 2,
			SELL         = 3,
			SELL_SHORT   = 4,
		};

        Action() { t_ = BUY; }
        Action(Type_ t) : t_(t) { }
        operator Type_ () const { return t_; }

        std::string toString() {
            switch (t_) {
            case BUY:
                return "BUY";
                break;
            case BUY_TO_COVER:
                return "BUY_TO_COVER";
                break;
            case SELL:
                return "SELL";
                break;
            case SELL_SHORT:
                return "SELL_SHORT";
                break;
            default:
                throw std::invalid_argument("Invalid action.");
            }
        }

    private:
        //prevent automatic conversion for any other built-in types such as bool, int, etc
//        template<typename T>
//        operator T () const;
        Type_ t_;
	};

	class State
	{
	public:
		enum Type_ {
			INITIAL          = 1,   // Initial state.
			SUBMITTED        = 2,   // Order has been submitted.
			ACCEPTED         = 3,   // Order has been acknowledged by the broker.
			CANCELED         = 4,   // Order has been canceled.
			PARTIALLY_FILLED = 5,	// Order has been partially filled.
			FILLED           = 6,   // Order has been completely filled.
            REJECTED         = 7,   // Order has been rejected.
		};

        State() { t_ = INITIAL; }
        State(Type_ t) : t_(t) { }
        operator Type_ () const { return t_; }

		std::string toString() {
			switch (t_) {
			case INITIAL:
				return "INITIAL";
				break;
			case SUBMITTED:
				return "SUBMITTED";
				break;
			case ACCEPTED:
				return "ACCEPTED";
				break;
			case CANCELED:
				return "CANCELED";
				break;
			case PARTIALLY_FILLED:
				return "PARTIALLY_FILLED";
				break;
			case FILLED:
				return "FILLED";
				break;
            case REJECTED:
                return "REJECTED";
                break;
			default:
				throw std::invalid_argument("Invalid state.");
			}
		}

    private:
        Type_ t_;
//        template<typename T>
//        operator T () const;
	};

	class Type 
	{
    public:
		enum Type_ {
			MARKET = 1,
			LIMIT = 2,
			STOP = 3,
			STOP_LIMIT = 4,
		};

        Type() { t_ = MARKET; }
        Type(Type_ t) : t_(t) { }
        operator Type_ () const { return t_; }

    private:
        Type_ t_;
//        template<typename T>
//        operator T () const;
	};

    typedef struct {
        enum {
            NextBar,
            IntraBar,
        };
    } ExecTiming;

    Order();

	Order(unsigned long orderId, Type type, Action action, const string& instrument, int quantity);

    ~Order();

    // Returns the order id.
	unsigned long getId() const;
    // Returns the order type. Valid order types are:
    // * Order.Type.MARKET
    // * Order.Type.LIMIT
    // * Order.Type.STOP
    // * Order.Type.STOP_LIMIT
	Type getType() const;
    // Returns the order action. Valid order actions are:
    // * Order.Action.BUY
    // * Order.Action.BUY_TO_COVER
    // * Order.Action.SELL
    // * Order.Action.SELL_SHORT
	Action getAction() const;
    // Returns the order state. Valid order states are:
    // * Order.State.INITIAL (the initial state).
    // * Order.State.SUBMITTED
    // * Order.State.ACCEPTED
    // * Order.State.CANCELED
    // * Order.State.PARTIALLY_FILLED
    // * Order.State.FILLED
	Order::State getState() const;
    // Returns True if the order is active.
	bool isActive() const;
    // Returns True if the order state is Order.State.INITIAL.
	bool isInitial() const;
    // Returns True if the order state is Order.State.SUBMITTED.
	bool isSubmitted() const;
    // Returns True if the order state is Order.State.ACCEPTED.
	bool isAccepted() const;
    // Returns True if the order state is Order.State.CANCELED.
	bool isCanceled() const;
    // Returns True if the order state is Order.State.PARTIALLY_FILLED.
	bool isPartiallyFilled() const;
    // Returns True if the order state is Order.State.FILLED.
	bool isFilled() const;
    // Returns the instrument identifier.
	const string& getInstrument() const;
    // Returns the quantity.
	int getQuantity() const;
    // Returns the number of shares that have been executed.
    int getFilled() const;
    // Returns the number of shares still outstanding.
    int getRemaining() const;
    // Returns the average price of the shares that have been executed, or None if nothing has been filled.
    double getAvgFillPrice() const;
    double getCommissions() const;
    // Returns True if the order is good till canceled.
    bool getGoodTillCanceled() const;
    // Sets if the order should be good till canceled.
    // Orders that are not filled by the time the session closes will be will be automatically canceled
    // if they were not set as good till canceled
    // @param goodTillCanceled: True if the order should be good till canceled.
    // 
    // NOTE: This can't be changed once the order is submitted.
    void setGoodTillCanceled(bool goodTillCanceled = true);
    // Returns True if the order should be completely filled or else canceled.
    bool getAllOrNone() const;
    // Sets the All-Or-None property for this order.
    //
    // @param allOrNone: True if the order should be completely filled.
    // NOTE: This can't be changed once the order is submitted.
    void setAllOrNone(bool allOrNone);

    void addExecutionInfo(const OrderExecutionInfo& execInfo);

    void switchState(State newState);

    void setState(State newState);

    // Returns the last execution information for this order, or None if nothing has been filled so far.
    // This will be different every time an order, or part of it, gets filled.
    const OrderExecutionInfo& getExecutionInfo() const;

    // Returns True if this is a BUY or BUY_TO_COVER order.
	bool isBuy() const;
    // Returns True if this is a SELL or SELL_SHORT order.
	bool isSell() const;
    // Returns True if this is a BUY or SELL_SHORT order.
    bool isOpen() const;
    // Returns True if this is a SELL or BUY_TO_COVER order.
    bool isClose() const;

    bool isValid() const;

    void setSubmittedDateTime(const DateTime& dateTime);
    const DateTime& getSubmittedDateTime() const;
    void setAcceptedDateTime(const DateTime& dateTime);
    const DateTime& getAcceptedDateTime() const;
    // Returns True if the order should be filled as close to the closing price as possible (Market-On-Close order).
    bool getFillOnClose() const;
    void setFillOnClose(bool onClose);
    // Returns the limit price.
    double getLimitPrice() const;
    void setLimitPrice(double price);
    // Returns the stop price.
    double getStopPrice() const;
    void setStopPrice(double price);

    double getTriggerPrice() const;
    void   setTriggerPrice(double price);

    const char* getSignalName() const;
    void  setSignalName(const char* signal);

    void setStopHit(bool stopHit);
    bool getStopHit() const;
    void setExecTiming(int timing);
    int  getExecTiming() const;
    void setTag(int tag);
    int  getTag() const;

private:
	unsigned long      m_id;
	Type               m_type;
    Action             m_action;
    string             m_instrument;
    int                m_quantity;
    int                m_filled;
    double             m_avgFillPrice;
    OrderExecutionInfo m_executionInfo;
    bool               m_goodTillCanceled;
    double             m_commissions;
    double             m_slippages;
    bool               m_allOrNone;
	State              m_state;
    DateTime           m_submitted;
    DateTime           m_accepted;
    bool               m_onClose;
    double             m_stopPrice;
    double             m_limitPrice;
    bool               m_stopHit;
    // Indicates that this order should be executed at current bar or next bar. 
    // This field is only available to Limit or Stop order.
    int                m_execTiming;
    int                m_tag;                 // Use for special purpose.
    double             m_triggerPrice;        // Price which trigger this order, set by user.
    char               m_signalName[32];      // Signal which trigger this order, set by user.
    bool               m_isValid;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class OrderEvent
{
public:
    enum Type {
        SUBMITTED = 1,          // Order has been submitted.
        ACCEPTED = 2,           // Order has been acknowledged by the broker.
        CANCELED = 3,           // Order has been canceled.
        PARTIALLY_FILLED = 4,   // Order has been partially filled.
        FILLED = 5,             // Order has been completely filled.
        REJECTED = 6,           // Order has been rejected.
    };

    OrderEvent(const Order& order, int eventType);

    const Order& getOrder() const;
    int getEventType() const;
    const string& getEventInfo() const;
    void setEventInfo(const string& info);

    const OrderExecutionInfo& getExecInfo() const;
    void setExecInfo(const OrderExecutionInfo& execInfo);

private:
    const Order m_order;
    int m_eventType;

    // This depends on the event type:
    // ACCEPTED: None
    // CANCELED: A string with the reason why it was canceled.
    // PARTIALLY_FILLED: An OrderExecutionInfo instance.
    // FILLED: An OrderExecutionInfo instance.
    string m_eventInfo;
    OrderExecutionInfo m_execInfo;
};

} // namespace xBacktest

#endif	// XBACKTEST_ORDER_H