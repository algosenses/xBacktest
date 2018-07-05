#ifndef XBACKTEST_POSITION_IMPL_H
#define XBACKTEST_POSITION_IMPL_H

#include <map>
#include <string>
#include <list>
#include "Bar.h"
#include "Order.h"
#include "Returns.h"

namespace xBacktest
{

class Runtime;

class PositionImpl
{
public:
    PositionImpl();

    ~PositionImpl();

    // @param runtime: The runtime that this position belongs to.
    // @param entryOrder: The order used to enter the position.
    void init(Position* abstraction, Runtime* runtime, const Order& entryOrder);

    Position* getAbstraction() const;

    unsigned long getId() const;

    unsigned long getEntryId() const;

    const DateTime& getEntryDateTime() const;
    
    const DateTime& getExitDateTime() const;

    const DateTime& getLastDateTime() const;

    int getExitType() const;

    double getAvgFillPrice() const;

    void getActiveOrders();

    void getTransactionRecords(TransactionList& outRecords);

    void getSubPositions(SubPositionList& outPos);

    // Returns the number of shares.
    // This will be a positive number for a long position, and a negative number for a short position.
    // NOTE: If the entry order was not filled, or if the position is closed, then the number of shares will be 0.
    int getShares() const;

    // Returns True if the entry order is active.
    bool entryActive() const;

    // Returns True if the entry order was filled.
    bool entryFilled() const;

    // Returns True if the exit order is active.
    bool exitActive(int subPosId) const;

    // Returns True if the exit order was filled.
    bool exitFilled() const;

    // Returns the orders used to enter the position.
    const vector<Order>& getEntryOrders() const;

    // Returns the orders used to exit the position.
    const vector<Order>& getExitOrders() const;

    // Returns the instrument used for this position.
    const string& getInstrument() const;

    // Calculates cumulative percentage returns up to this point.
    // If the position is not closed, these will be unrealized returns.
    // @param includeCommissions: True to include commissions in the calculation.
    double getReturn(bool includeCommissions = true) const;

    // Calculates PnL up to this point.
    // If the position is not closed, these will be unrealized PnL.
    // @param includeCommissions: True to include commissions in the calculation.
    double getPnL(bool includeCommissions = true) const;

    double getCommissions() const;

    double getSlippages() const;

    double getRealizedPnL() const;

    // Places a market order or stop order to close this position.
    // @param posId: Sub position which will be closed. If the id is 0, the earliest position closed first.
    // Note:
    // * If the position is closed (entry canceled or exit filled) this won't have any effect.
    // * If the exit order for this position is pending, an exception will be raised. The exit order should be canceled first.
    // * If the entry order is active, cancellation will be requested.
    void close(int signalType, int subPosId,  double price, bool immediately);

    // Returns True if the position is open.
    bool isOpen() const;

    // Returns True if the position's direction is long.
    bool isLong() const;

    void setStopLossAmount(double amount, int subPosId = 0);
    void setStopLossPrice(double price, int subPosId = 0);
    void setStopLossPercent(double pct, int subPosId = 0);

    void setStopProfitPct(double returns, int subPosId = 0);
    void setStopProfit(double price, int subPosId = 0);
    void setTrailingStop(double returns, double drawdown, int subPosId = 0);

    void setPercentTrailing(double amount, double percentage);

    // Returns the duration in open state.
    // Note:
    // * If the position is open, then the difference between the entry datetime and the datetime of the last bar is returned.
    // * If the position is closed, then the difference between the entry datetime and the exit datetime is returned.
    unsigned long getDuration() const;

    double getHighestPrice() const;
    double getLowestPrice() const;
    double getEntryTriggerPrice() const;
    double getExitTriggerPrice() const;

protected:
    friend class Runtime;
    
    void placeAndRegisterOrder(const Order& order);

    void setEntryDateTime(const DateTime& dateTime);

    void setExitDateTime(const DateTime& dateTime);

    void placeStopOrder(
        int signalType,
        int quantity, 
        double stopPrice, 
        double limitPrice, 
        int closePosId, 
        bool immediately);

    void onBarEvent(const Bar& bar);

    void onOrderEvent(const OrderEvent& orderEvent, vector<Position::Variation>& result);

    Order buildExitOrder(int quantity, double stopPrice, double limitPrice);

    Order buildExitLongOrder(int quantity, double stopPrice, double limitPrice);

    Order buildExitShortOrder(int quantity, double stopPrice, double limitPrice);

    double getAvgEntryPrice() const;

    double getAvgExitPrice() const;

private:
    typedef struct {
        enum {
            Unknown = 0,
            StopLoss,
            StopProfit,
        };
    } StopType;

    typedef struct {
        enum {
            Individual,
            Entirety,
        };
    } StopBoundary;

    typedef struct {
        enum {
            Fixed,
            Percentage,
        };
    } CalcMethod;

    // Record `profit level` first, when profit touch a specified level and
    // drawdown reached, stop profit condition is triggered, then we 
    // generate a stop order and send it to broker.
    struct ProfitLevel {
        double returns;
        double drawdown;
        bool   triggered;
        static bool lowerProfitPct(const ProfitLevel& p1, const ProfitLevel& p2)
        {
            return p1.returns < p2.returns;
        }
    };

    // Auto check stop loss/profit condition.
    typedef struct _StopCondition {
        int    type;
        bool   active;
        int    profitCalcMethod;
        int    drawdownCalcMethod;
        bool   stopImmediately;
        int    posId;
        // positive for long position, negative for short position.
        int    shares;
        double avgFillPrice;
        double lossPct;
        double lossAmount;
        double highestPrice;
        double lowestPrice;
        vector<struct ProfitLevel> profitLevels;

        _StopCondition()
        {
            active = false;
            highestPrice = 0.0;
            lowestPrice = 0.0;
            profitCalcMethod = CalcMethod::Percentage;
            drawdownCalcMethod = CalcMethod::Percentage;
            stopImmediately = true;
        }

        void resetStopProfit() { 
            highestPrice = avgFillPrice;
            lowestPrice = avgFillPrice;            
            for (size_t i = 0; i < profitLevels.size(); i++) {
                profitLevels[i].triggered = false;
            }
        }
    } StopCondition;

    void buy(const Order& order, const OrderExecutionInfo& execInfo, vector<Position::Variation>& variations);
    void sell(const Order& order, const OrderExecutionInfo& execInfo, vector<Position::Variation>& variations);
    void sellShort(const Order& order, const OrderExecutionInfo& execInfo, vector<Position::Variation>& variations);
    void cover(const Order& order, const OrderExecutionInfo& execInfo, vector<Position::Variation>& variations);
    void createOrUpdateSubPos(const OrderEvent& orderEvent, vector<Position::Variation>& variations);
    void update(const OrderEvent& orderEvent);
    bool checkStopLoss(const Bar& bar, StopCondition& condition);
    bool checkProfitDynamicDrawDown(const Bar& bar, StopCondition& condition);
    bool checkProfitTarget(const Bar& bar, StopCondition& condition);
    // Check if exiting (stop loss or stop profit) is needed.
    void checkStopCondition(const Bar& bar, StopCondition& condition);

    void exit(int signalType, int quantity, double stopPrice, double limitPrice, int subPosId);
    // Places a special Intra-Bar order to close this position.
    // This function is use to stop loss/profit only.
    void exitImmediately(int signalType, int quantity, double stopPrice, double limitPrice, int subPosId);
    void mapOrdIdToSubPos(int subPosId, int orderId);
    int  getSubPosIdByOrd(int orderId) const;
    void unsetSubPosOrdIdMap(int subPosId);

    bool canPlaceOrder(const Order& order);
    void updateOrderList(vector<Order>& list, const Order& order);

    double roundUp(double price);
    double roundDown(double price);

    void printPositionDetail();

private:
    string m_instrument;
    double m_multiplier;
    bool   m_allOrNone;

    vector<Order> m_entryOrders;
    vector<Order> m_exitOrders;
    vector<Order> m_allOrders;
    int m_entryShares;
    int m_exitShares;

    unsigned long m_entryOrdId;

    double m_entryTriggeredPrice;
    double m_exitTriggeredPrice;
    double m_histHighestPrice;
    double m_histLowestPrice;
    int m_duration;

    int      m_shares;
    int      m_entryType;
    int      m_exitType;
    double   m_cost;
    double   m_cash;
    double   m_avgFillPrice;
    double   m_netPnL;
    double   m_realizedPnL;
    double   m_commissions;
    double   m_slippages;
    DateTime m_entryDateTime;
    DateTime m_exitDateTime;

    typedef struct {
        unsigned long entryId;  // Position ID, in practice is the id of entry order.
        DateTime      entryDateTime;
        double        entryTrigger;
        double        entryPrice;
        int           entryType;
        unsigned long exitId;
        DateTime      exitDateTime;
        double        exitTrigger;
        double        exitPrice;
        int           exitType;
        int           entryShares;
        int           currShares;
        double        cash;
        double        cost;
        double        commissions;
        double        slippages;
        double        realizedPnL;
        double        histHighestPrice;
        double        histLowestPrice;
        int           duration;
    } SubPosItem;

    list<SubPosItem> m_subPosList;
    vector<StopCondition> m_stopConditions;

    typedef struct {
        bool valid;
        int  orderId;
        int  posId;
    } SubPosOrderMappingItem;
    vector<SubPosOrderMappingItem> m_subPosOrderMap;

    vector<Transaction> m_transactions;

    int m_direction;

    unsigned long m_id;
    Bar m_lastBar;

    Position* m_abstraction;
    Runtime* m_runtime;
    static volatile unsigned long m_nextPositionId;
};

} // namespace xBacktest

#endif // XBACKTEST_POSITION_IMPL_H