#ifndef POSITION_H
#define POSITION_H

#include <map>
#include <string>
#include "Vector.h"
#include "Order.h"

namespace xBacktest
{

class PositionImpl;

typedef struct {
    string   instrument;
    DateTime entryDateTime;
    double   entryTrigger;
    double   entryPrice;
    int      entryType;
    DateTime exitDateTime;
    double   exitTrigger;
    double   exitPrice;
    int      exitType;
    int      shares;
    double   commissions;
    double   slippages;
    double   realizedPnL;
    double   highestPrice;
    double   lowestPrice;
    int      duration;
} Transaction;

typedef Vector<Transaction> TransactionList;

// SubPosition is a handy abstraction, thus we can close specified sub-position at strategy side.
// In the real world, sub-position was first-open first-close, we can not designate 
// which sub-position to be closed.
typedef struct {
    unsigned long id;
    DateTime datetime;
    double   price;
    int      shares;
    double   histHighestPrice;
    double   histLowestPrice;
    int      age;
} SubPosition;

typedef Vector<SubPosition> SubPositionList;

// Class for positions.
// Positions are higher level abstractions for placing orders.
// They are essentially a bunch of entry-exit orders and allow
// to track returns and PnL easier that placing orders manually.
// One position contains at least one sub-position.
//
// NOTE: This class should not be used directly.
class DllExport Position
{
    friend class Runtime;

public:
    typedef struct {
        enum {
            Unknown = 0,
            // Entry trades
            EntryLong,
            IncreaseLong,
            EntryShort,
            IncreaseShort,
            // Exit trades
            ReduceLong,
            ExitLong,
            ReduceShort,
            ExitShort
        };
    } Action;

    enum Direction {
        LongPos,
        ShortPos,
    };

    typedef struct {
        int      id;           // This id can be used to fetch sub position data. 
        int      action;
        DateTime datetime;
        int      origShares;
        int      currShares;
        double   price;
        double   commissions;
        double   slippages;
        double   realizedPnL;
    } Variation;

    unsigned long getId() const;

    unsigned long getEntryId() const;

    const DateTime& getEntryDateTime() const;
    
    const DateTime& getExitDateTime() const;

    const DateTime& getLastDateTime() const;

    int getExitType() const;

    double getAvgFillPrice() const;

    void getActiveOrders();

    // Returns the number of shares.
    // This will be a positive number for a long position, and a negative number for a short position.
    // NOTE: If the entry order was not filled, or if the position is closed, then the number of shares will be 0.
    int getShares() const;

    int getActiveShares() const;

    void getTransactionRecords(TransactionList& outRecords);

    void getSubPositions(SubPositionList& outPos);

    // Returns True if the entry order is active.
    bool entryActive();

    // Returns True if the entry order was filled.
    bool entryFilled();

    // Returns True if the exit order is active.
    bool exitActive(int subPosId = 0);

    // Returns True if the exit order was filled.
    bool exitFilled();

    // Returns the instrument used for this position.
    const string& getInstrument() const;

    // Calculates cumulative percentage returns up to this point.
    // If the position is not closed, these will be unrealized returns.
    // @param includeCommissions: True to include commissions in the calculation.
    double getReturn(bool includeCommissions = true);

    // Calculates PnL up to this point.
    // If the position is not closed, these will be unrealized PnL.
    // @param includeCommissions: True to include commissions in the calculation.
    double getPnL(bool includeCommissions = true);

    double getCommissions() const;

    double getSlippages() const;

    double getRealizedPnL() const;

    // Places a market order to close this position.
    // @param subPosId: The sub-position id. If this is 0, then close entire position.
    //
    // Note:
    // * If the position is closed (entry canceled or exit filled) this won't have any effect.
    // * If the exit order for this position is pending, an exception will be raised. The exit order should be canceled first.
    // * If the entry order is active, cancellation will be requested.
    void close(int subPosId = 0);

    void closeImmediately(int subPosId, double price);

    // Returns True if the position is open.
    bool isOpen() const;

    // Returns True if the position's direction is long.
    bool isLong() const;

    // Closes out the entire position or the entry if profit reaches the specified currency value; 
    // generates the appropriate Limit exit order depending on whether the position is long or short.
    // amount - specifying the profit target amount.
    void setProfitTarget(double amount);

    // Closes out the entire position or the entry if the specified percentage of the maximum profit 
    // is lost after the profit has reached the specified value; 
    // generates the appropriate Limit exit order depending on whether the position is Long or Short.
    //
    // For example, if the specified profit is $100 and the specified percentage is 50, and the profit 
    // has reached the maximum of $120, the position will be closed once the profit falls back to $60.
    // profit - specifying the currency value of the profit that must be reached first.
    // percentage - specifying the maximum loss of profit in percent.
    void setPercentTrailing(double amount, double percentage);

    void setStopLossAmount(double amount, int subPosId = 0);
    void setStopLossPrice(double price, int subPosId = 0);
    void setStopLossPercent(double pct, int subPosId = 0);
    
    void setStopProfit(double price, int subPosId = 0);
    void setStopProfitPct(double returns, int subPosId = 0);
    void setTrailingStop(double returns, double drawdown, int subPosId = 0);
    // 
    void setBreakEvent();
    void setProfitTarget();
    void setDollarTrailing();
    void setPercentTrailing();

    // Returns the duration in open state.
    // Note:
    // * If the position is open, then the difference between the entry datetime and the datetime of the last bar is returned.
    // * If the position is closed, then the difference between the entry datetime and the exit datetime is returned.
    unsigned long getDuration() const;

private:
    Position();
    ~Position();
    Position(const Position &);
    Position &operator=(const Position &);

    PositionImpl* getImplementor() const;

private:
    PositionImpl* m_implementor;
};

} // namespace xBacktest

#endif // POSITION_H