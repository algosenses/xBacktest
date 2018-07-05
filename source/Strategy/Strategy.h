#ifndef STRATEGY_H
#define STRATEGY_H

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the STRATEGY_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// STRATEGY_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef STRATEGY_EXPORTS
#define STRATEGY_API __declspec(dllexport)
#else
#define STRATEGY_API __declspec(dllimport)
#endif

#include "Defines.h"
#include "DateTime.h"
#include "Bar.h"
#include "Order.h"
#include "BarSeries.h"
#include "Position.h"

using namespace xBacktest;

namespace xBacktest
{

class Runtime;

// Base class for strategies.
// This is a base class and should not be used directly.
class DllExport Strategy
{
    friend class Runtime;
    friend class PositionImpl;

public:
    Strategy();
    virtual ~Strategy();

    // Override (optional) to get notified when the order submitted to enter a position was filled. The default implementation is empty.
    // @param position: A position returned by any of the enterLongXXX or enterShortXXX methods.
    virtual void onPositionOpened(Position& position);

    // Called when the position has be increased or reduced.
    // @param position: Integral position object.
    // @param variation: Position variation.
    virtual void onPositionChanged(Position& position, const Position::Variation& variation);

    // Called when the exit order for a position was filled.
    // Override (optional) to get notified when the order submitted to exit a position was filled. The default implementation is empty.
    // @param position: A position returned by any of the enterLongXXX or enterShortXXX methods.
    virtual void onPositionClosed(Position& position);

    // Override (optional) to get notified when the strategy was created. The default implementation is empty.
    // The initialization codes should be placed here.
    virtual void onCreate();

    // Override (optional) to get notified when the parameter was set or changed. The default implementation is empty.
    virtual void onSetParameter(const char* name, int type, const char* value, bool isLast);

    // Override (optional) to get notified when the environment variable was set or changed. The default implementation is empty.
    virtual void onEnvVariable(const char* name, double value);

    // Override (optional) to get notified when the strategy starts executing. The default implementation is empty.
    virtual void onStart();

    // Override (optional) to get notified when the strategy finished executing. The default implementation is empty.
    // param bar: The last bar processed.
    virtual void onStop();

    virtual void onDestroy();

    // "Override (**mandatory**) to get notified when new bar is available. The default implementation raises an Exception.
    // This is the method to override to enter your trading logic and enter/exit positions.
    // @param bar: The current bar.
    virtual void onBar(const Bar& bar);

    // Override (optional) to get notified when historical data are available.
    virtual void onHistoricalData(const Bar& bar, bool isCompleted);

    // Override (optional) to get notified when an order gets updated.
    // This is not called for orders placed using any of the enterLong or enterShort methods.
    // @param order: The order updated.
//    virtual void onOrderUpdated(const Order& order);

    // Override (optional) to get notified when order filled event occurs. 
    // The default implementation is empty.
    virtual void onOrderFilled(const Order& order);
    
    // Override (optional) to get notified when order partial filled event occurs. 
    // The default implementation is empty.
    virtual void onOrderPartiallyFilled(const Order& order);

    // Override (optional) to get notified when order has been canceled. 
    // The default implementation is empty.
    virtual void onOrderCanceled(const Order& order);

    // Override (optional) to get notified when order has been rejected. 
    // The default implementation is empty.
    virtual void onOrderFailed(const Order& order);

    // Override (optional) to get notified when time elapsed.
    // The default implementation is empty.
    virtual void onTimeElapsed(const DateTime& prevDateTime, const DateTime& currDateTime);

protected:
    // Create a market order.
    Order createMarketOrder(
        Order::Action action,
        const char* instrument,
        int quantity,
        bool onClose = false,
        bool goodTillCanceled = false,
        bool allOrNone = false);

    // Create a limit order.
    Order createLimitOrder(
        Order::Action action,
        const char* instrument,
        double limitPrice,
        int quantity,
        bool goodTillCanceled = false,
        bool allOrNone = false);

    // Create a stop order.
    Order createStopOrder(
        Order::Action action,
        const char* instrument,
        double stopPrice,
        int quantity,
        bool goodTillCanceled = false,
        bool allOrNone = false);

    // Create a stop limit order.
    Order createStopLimitOrder(
        Order::Action action,
        const char* instrument,
        double stopPrice,
        double limitPrice,
        int quantity,
        bool goodTillCanceled = false,
        bool allOrNone = false);

    unsigned long   getId() const;
    const char*     getMainInstrument() const;
    const Contract& getDefaultContract() const;
    BarSeries&      getBarSeries(const char* instrument = '\0');
    const Bar&      getLastBar(const char* instrument = '\0') const;
    double          getLastPrice(const char* instrument = '\0') const;
    double          getTickSize(const char* instrument = '\0') const;

    // Returns the datetime for the current bar.
    const DateTime& getCurrentDateTime();

    // Returns the current market position size.
    // For example, we are at the short position for 100 contracts, then getPositionSize() will
    // return '-100', if we have a long position for 50 contracts, the getPositionSize() will
    // return the value '50'.
    // @param instrument: Instrument identifier.
    long            getPositionSize(const char* instrument = nullptr);
    Position*       getCurrentPosition(const char* instrument, int side);
    void            getAllPositions(Vector<Position*>& positions);
    void            getAllSubPositions(const char* instrument, SubPositionList& positions);
    // Close specified sub-position.
    // After finally accounting the gain and loss will equal to the result of the real world.
    // In the real world, sub-position was first-open first-close, we can not designate 
    // which sub-position to be closed first.
    void            closeSubPosition(int subPosId);
    void            closePosition(int posId);
    void            closeAllPositions();
    void            closeAllPositionsImmediately(double stopPrice);

    // Generates a buy `MarketOrder` or `LimitOrder` to enter a long position.
    // @param instrument: Instrument identifier.
    // @param quantity: Entry order quantity.
    // @param price: Limit price. If 0.0 then a `LimitOrder` will be generated.
    // @param immediately: True if the order should be executed at current bar.
    // @param signal: Entry signal name.
    // @return Order Id for follow-up tracking.
    unsigned long buy(
        const char* instrument,
        int quantity,
        double price = 0.0,
        bool immediately = false,
        const char* signal = nullptr);

    // Generates a sell `MarketOrder` or `LimitOrder` to exit(selling) a long position. 
    // This order cannot be sent, if the long position is not opened. Also, this
    // position cannot be reversed into short.
    unsigned long sell(
        const char* instrument,
        int quantity,
        double price = 0.0,
        bool immediately = false,
        const char* signal = nullptr);

    // Short position entry(selling). If the long position is currently opened, then it will be closed,
    // and the short one will be opened.
    unsigned long sellShort(
        const char* instrument,
        int quantity,
        double price = 0.0,
        bool immediately = false,
        const char* signal = nullptr);

    // Short position exit(buying). This order cannot be sent, if the short position is not opened.
    unsigned long buyToCover(
        const char* instrument,
        int quantity,
        double price = 0.0,
        bool immediately = false,
        const char* signal = nullptr);

    unsigned long   submitOrder(const Order& order);
    void            cancelOrder(unsigned long orderId);

    double          getAvailableCash() const;
    double          getEquity() const;
    bool            registerParameter(const char* name, int type);

    void            setEnv(const char* name, double value);
    void            unsetEnv(const char* name);
    void            monitorEnv(const char* name);
    
    // load historical data, currently only supports synchronized mode.
    // `onHistoricalData` will be called back immediately.
    int             loadHistoricalData(const DataRequest& request);

    bool            aggregateBarSeries(const TradingSession& session,
                           int inRes,  int inInterval,  BarSeries& input,
                           int outRes, int outInterval, BarSeries& output);

    void            printDebugMsg(const char* msgFmt, ...) const;

    long            openLong(int quantity, const char* instrument = nullptr, const char* signal = nullptr);
    long            closeLong(const char* instrument = nullptr, const char* signal = nullptr);
    long            openShort(int quantity, const char* instrument = nullptr, const char* signal = nullptr);
    long            closeShort(const char* instrument = nullptr, const char* signal = nullptr);

    long            openLongImmediately(int quantity, double price, const char* instrument = nullptr, const char* signal = nullptr);
    long            closeLongImmediately(double price, const char* instrument = nullptr, const char* signal = nullptr);
    long            openShortImmediately(int quantity, double price, const char* instrument = nullptr, const char* signal = nullptr);
    long            closeShortImmediately(double price, const char* instrument = nullptr, const char* signal = nullptr);

private:
    void            attach(Runtime* runtime);

private:
    // The environment which this strategy running inside.
    Runtime* m_runtime;
};

extern "C" typedef Strategy *StrategyCreator();

#define EXPORT_STRATEGY(strategy_name)                      \
    extern "C" STRATEGY_API Strategy* CreateStrategy()      \
    {                                                       \
        return new strategy_name;                           \
    }

// Only lambdas with no capture can be converted to a function pointer. 
// This is an extension of lambdas for only this particular case. 
// In general, lambdas are function objects, and you cannot convert a 
// function object to a function.
// The alternative for lambdas that have state(capture) is to use 
// std::function rather than a plain function pointer.
#define STRATEGY_CREATOR(strategy_name) \
    []() -> Strategy* { return new strategy_name; }

} // namespace xBacktest

#endif // STRATEGY_H