#ifndef BACK_TESTING_H
#define BACK_TESTING_H

#include <list>
#include <unordered_map>
#include "Broker.h"
#include "BarFeed.h"
#include "FillStrategy.h"
#include "Returns.h"

using std::unordered_map;

namespace xBacktest
{

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Back testing broker.
class BacktestingBroker : public BaseBroker
{
public:
	BacktestingBroker();
    virtual ~BacktestingBroker();

    void init(double cash = DEFAULT_BROKER_CASH);
    void initFillStrategy();
    void setAllowFractions(bool allowFractions);
    bool getAllowFractions() const;
    void setAllowNegativeCash(bool allowNegativeCash);
    double getCash() const;
    //@param cash: The amount of cash available.
    void setCash(double cash = 1000000);
    // Returns the available cash.
    double getAvailableCash() const;
    double getMargin() const;
    double getPosProfit() const;
    double getTotalCommission() const;
    double getTotalSlippage() const;
    double getMaxMarginRequired() const;
    void registerContract(const Contract& contract);
    void setFillStrategy();
    FillStrategy& getFillStrategy(int resolution);
    void enableTradingDayNotification(bool enable);
    void setTradingDayEndTime(int timenum);
    // Returns a sequence with the orders that are still active.
    // @param instrument: An optional instrument identifier to return only the active orders for the given instrument.
    void getActiveOrders(const string& instrument = "");
    void getPendingOrders();
    int  getShares(const string& instrument) const;
    int  getLongShares(const string& instrument) const;
    int  getShortShares(const string& instrument) const;
    const Contract& getContract(const string& instrument) const;
    void getPositions();
    void getActiveInstruments();
    void getValue();
    // Returns the portfolio value (cash + shares).
    double getEquity() const;

    long long getTradingPeriod(int& year, int& month, int& day, int& hour, int& minute);

	void placeOrder(const Order& order);
    void cancelOrder(unsigned long orderId);

    void onBarEvent(const Bar& bar);
    void onPostBarEvent(const Bar& bar);
    
    StgyAnalyzer* getNamedAnalyzer(const string& name);
    void attachAnalyzerEx(StgyAnalyzer& strategyAnalyzer, const string& name = "");
    // Adds a analyzer.
    void attachAnalyzer(StgyAnalyzer& analyzer);

private:
    void updateEquityWithBar(const Bar& bar, bool onClose = true);

    // Calculates the commission for an order execution.
    // @param order: The order being executed.
    // @param price: The price for each share.
    // @param quantity: The order size.
    // @param multiplier: The instrument multiplier.
    double calculateCommission(const Order& order, double price, int quantity, double multiplier = 1.0);

    // Calculates the slippages for an order execution.
    double calculateSlippage(const Order& order, double price, int quantity, double multiplier = 1.0);

	// Tries to commit an order execution. 
    // Returns True if the order was committed, or False if there is not enough cash.
	bool commitOrderExecution(Order* orderRef, const Bar& bar, const FillInfo& fillInfo);

    void saveCurrBar(const Bar& bar);

    bool getLastBar(const string& instrument, Bar& outBar) const;

    // Return True if further processing is needed.
    bool preProcessOrder(Order* orderRef, const Bar& bar);
    bool postProcessOrder(Order* orderRef, const Bar& bar);
    bool processOrder(Order* orderRef, const Bar& bar);

    void onBarImpl(Order* orderRef, const Bar& bar);
    bool checkExpired(Order* orderRef, const DateTime& datetime);
    void processOrders(const Bar& bar);
    void notifyAnalyzers(const Bar& bar);

    void printPositionList(Order* orderRef);

private:
    // Customer equity include two parts (static and dynamic part):
    // 1. cash (static part, will remain unchanged when the last transaction is done)
    // 2. portfolio value (dynamic part, will follow the ever-changing market)
    // A transaction cutting out partial cash and change them into portfolio or vice versa.

    // Equal to 'cash + portfolio value'
    double m_equity;    
    double m_cash;
    double m_portfolioValue;
    double m_margin;
    // Equal to 'equity - margin'
    double m_availableCash;
    // Position profit
    double m_posProfit;
    
    // Maximum margin required during trading period.
    double m_maxMarginRequired;

    double m_totalCommissions;
    double m_totalSlippages;

    typedef struct {
        int    shares;
        double price;
    } SubPosItem;

    typedef struct {
        int    totalShares;
        double avgPrice;
        double lastPrice;
        list<SubPosItem> subPositions;
    } BrokerPos;
    map<string, BrokerPos> m_positions;

    // Active order map must be a 'Ordered Map', 
    // because of orders were processed FIFO.
    map<unsigned long, Order> m_activeOrders;
    unordered_map<unsigned long, Order> m_orderRecords;
    FillStrategy* m_barFillStrategy;
    FillStrategy* m_tickFillStrategy;
    bool m_allowFractions;
    bool m_allowNegativeCash;

    unordered_map<string, Contract> m_contracts;
    unordered_map<string, Bar> m_lastBars;

    DateTime m_firstBarDateTime;
    DateTime m_lastBarDateTime;

    bool m_notifyNewTradingDay;
    int  m_tradingDayEndTime;

    vector<StgyAnalyzer *> m_analyzers;
    map<string, StgyAnalyzer*> m_namedAnalyzers;
};

} // namespace xBacktest

#endif // BACK_TESTING_H