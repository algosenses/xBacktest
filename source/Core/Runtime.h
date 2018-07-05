#ifndef XBACKTEST_RUNTIME_H
#define XBACKTEST_RUNTIME_H

#include "Thread.h"
#include "Lock.h"
#include "Event.h"
#include "Order.h"
#include "BarFeed.h"
#include "Composer.h"
#include "Backtesting.h"
#include "Strategy.h"
#include "Trades.h"
#include "Sharpe.h"
#include "Drawdown.h"
#include "Returns.h"
#include "Simulator.h"

namespace xBacktest
{

class Simulator;
class Dispatcher;
class Process;

// Strategy runtime environment
class Runtime
{
    friend class StrategyImpl;

public:
    enum State {
        Idle,
        Running,
        Pause,
        Stop,
    };

    Runtime(Process* process);
    ~Runtime();

    void setId(unsigned long id);
    unsigned long getId() const;
    void setName(const char* name);
    const char* getName() const;
    BacktestingBroker* getBroker();
    void addActiveDateTime(const DateTime& begin, const DateTime& end);
    bool isActivate() const;
    void setCash(double cash);
    double getAvailableCash() const;
    double getEquity() const;
    const Bar& getLastBar(const char* instrument) const;
    double getLastPrice(const char* instrument) const;
    double getTickSize(const char* instrument) const;
    void setMainInstrument(const char* instrument);
    const char* getMainInstrument() const;
    void subscribeAll();
    bool isSubscribeAll() const;
    const Contract& getContract(const char* instrument = nullptr) const;
    void registerInstrument(const char* instrument);
    void registerContracts(const vector<Contract>& contracts);
    void setStrategyObject(Strategy* strategy);
    void setParameters(const ParamTuple& params);
    long getPositionSize(const char* instrument = nullptr) const;
    Position* getCurrentPosition(const char*instrument, int side);
    void getAllPositions(Vector<Position*>& positions);
    void getAllSubPositions(const char* instrument, SubPositionList& posList);
    void closeSubPosition(int subPosId);
    void closePosition(int posId);
    void closeAllPositions();
    void closeAllPositionsImmediately(double price);
    // load historical data, currently only supports synchronized mode.
    // `onHistoricalData` will be called back immediately.
    int  loadData(const DataRequest& request);
    void writeDebugMsg(const char* msg);
    void reset();
    bool aggregateBarSeries(const TradingSession& session,
                            int inRes,  int inInterval,  BarSeries& input,
                            int outRes, int outInterval, BarSeries& output);
    unsigned long buy(
        const char* instrument, 
        int quantity, 
        double price, 
        bool immediately, 
        const char* signal);

    unsigned long sell(
        const char* instrument, 
        int quantity, 
        double price, 
        bool immediately, 
        const char* signal);

    unsigned long sellShort(
        const char* instrument, 
        int quantity, 
        double price, 
        bool immediately, 
        const char* signal);

    unsigned long buyToCover(
        const char* instrument, 
        int quantity, 
        double price, 
        bool immediately, 
        const char* signal);

    // Callback functions
    void onCreate();
    void onStart();
    void onBarEvent(const Bar& bar);
    // Return True if runtime accept this order event.
    bool onOrderEvent(const OrderEvent* evt);
    void onTimeElapsed(const DateTime& prevDateTime, const DateTime& currDateTime);
    void onStop();
    void onDestroy();
    void onHistoricalData(const Bar& bar, bool isCompleted);

    // Creates a Market order.
    // A market order is an order to buy or sell a stock at the best available price.
    // Generally, this type of order will be executed immediately. However, the price at which a market order will be executed
    // is not guaranteed.
    //
    // @param action: The order action.
    // @type action: Order.Action.BUY, or Order.Action.BUY_TO_COVER, or Order.Action.SELL or Order.Action.SELL_SHORT.
    // @param instrument: Instrument identifier.
    // @param quantity: Order quantity.
    // @param onClose: True if the order should be filled as close to the closing price as possible (Market-On-Close order). Default is False.
    Order createMarketOrder(
                        Order::Action action, 
                        const char* instrument, 
                        int quantity, 
                        bool onClose = false,
                        bool goodTillCanceled = false,
                        bool allOrNone = false);

    // Creates a Limit order.
    // A limit order is an order to buy or sell a stock at a specific price or better.
    // A buy limit order can only be executed at the limit price or lower, and a sell limit order can only be executed at the
    // limit price or higher.
    //
    // @param action: The order action.
    // @type action: Order.Action.BUY, or Order.Action.BUY_TO_COVER, or Order.Action.SELL or Order.Action.SELL_SHORT.
    // @param instrument: Instrument identifier.
    // @param limitPrice: The order price.
    // @param quantity: Order quantity.
    Order createLimitOrder(
                        Order::Action action,
                        const char* instrument,
                        double limitPrice,
                        int quantity,
                        bool goodTillCanceled = false,
                        bool allOrNone = false);

    // Creates a Stop order.
    // A stop order, also referred to as a stop-loss order, is an order to buy or sell a stock once the price of the stock
    // reaches a specified price, known as the stop price.
    // When the stop price is reached, a stop order becomes a market order.
    // A buy stop order is entered at a stop price above the current market price. Investors generally use a buy stop order
    // to limit a loss or to protect a profit on a stock that they have sold short.
    // A sell stop order is entered at a stop price below the current market price. Investors generally use a sell stop order
    // to limit a loss or to protect a profit on a stock that they own.
    //
    // @param action: The order action.
    // @type action: Order.Action.BUY, or Order.Action.BUY_TO_COVER, or Order.Action.SELL or Order.Action.SELL_SHORT.
    // @param instrument: Instrument identifier.
    // @param stopPrice: The trigger price.
    // @param quantity: Order quantity.
    Order createStopOrder(
                        Order::Action action,
                        const char* instrument,
                        double stopPrice,
                        int quantity,
                        bool goodTillCanceled = false,
                        bool allOrNone = false); 

    // http://www.sec.gov/answers/stoplim.htm
    // http://www.interactivebrokers.com/en/trading/orders/stopLimit.php
    // Creates a Stop-Limit order.
    // A stop-limit order is an order to buy or sell a stock that combines the features of a stop order and a limit order.
    // Once the stop price is reached, a stop-limit order becomes a limit order that will be executed at a specified price
    // (or better). The benefit of a stop-limit order is that the investor can control the price at which the order can be executed.
    //
    // @param action: The order action.
    // @type action: Order.Action.BUY, or Order.Action.BUY_TO_COVER, or Order.Action.SELL or Order.Action.SELL_SHORT.
    // @param instrument: Instrument identifier.
    // @param stopPrice: The trigger price.
    // @param limitPrice: The price for the limit order.
    // @param quantity: Order quantity.
    // @return A `StopLimitOrder` subclass.
    Order createStopLimitOrder(
                        Order::Action action,
                        const char* instrument,
                        double stopPrice,
                        double limitPrice,
                        int quantity,
                        bool goodTillCanceled = false,
                        bool allOrNone = false);


    unsigned long submitOrder(const Order& order);

    void placeOrder(const Order& order);

    void cancelOrder(unsigned long orderId);

    unsigned long getNextOrderId();

    Order createOrder(Order::Action action, 
                      const char* instrument, 
                      double stopPrice, 
                      double limitPrice, 
                      int quantity);
    void registerOrder(const Order& order);
    void createOrUpdatePosition(const OrderEvent& evt);
    void notifyPositions(const Bar& bar);
    void getTransactionRecords(vector<Transaction>& outRecords);
    bool registerParameter(const char* name, int type);
    BarSeries& getBarSeries(const char* instrument);
    void updateBarSeries(const Bar& bar);
    void inactivate();
    bool checkActive(const DateTime& datetime);

private:
    Process* m_process;
    
    unsigned long m_id;
    string     m_name;
    string     m_mainInstrument;
    Bar        m_lastBar;
    Strategy*  m_strategyObj;
    ParamTuple m_paramTuple;

    double m_cash;
    bool m_subscribeAll;
    vector<InstrumentDesc> m_symbols;
    std::unordered_map<string, Contract> m_contracts;
    unordered_map<string, Bar> m_lastBars;

    vector<BarComposer*> m_composers;

    volatile bool m_activated;
    volatile int m_state;

    typedef struct {
        // Only after this date time, runtime allowed to do a deal.
        DateTime begin;
        // Only before this date time, runtime allowed to do a deal.
        DateTime end;
    } ActivePeriod;

    vector<ActivePeriod> m_activePeriods;

    // mapping order id to order
    unordered_map<unsigned long, Order> m_orders;

    unordered_map<string, Position*> m_longPosList;
    unordered_map<string, Position*> m_shortPosList;
    vector<Position*> m_closedPosList;

    // Record dummy position size when running in inactive period.
    int m_dummyShares;

    unordered_map<string, BarSeries*> m_barSeries;

    Event m_barsProcessedEvent;

    typedef struct {
        string name;
        int type;
    } PItem;
    vector<PItem> m_parameters;
};

} // namespace xBacktest

#endif // XBACKTEST_RUNTIME_H