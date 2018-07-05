/*********************************************************************
 * Framework for trading strategies backtesting and optimization.    *
 *                                                                   *
 * $Author: zhuangshaobo $                                           *
 * $Mail:   zhuang@algo.trade $                                      *
 *********************************************************************/
#ifndef DEFINES_H
#define DEFINES_H

#include <string>
#include <vector>
#include <unordered_map>
#include "Export.h"
#include "DateTime.h"

using std::string;
using std::vector;

namespace xBacktest
{

enum LogLevel {
    Log_Trace,
    Log_Debug,
    Log_Info,
    Log_Warn,
    Log_Error,
    Log_Fatal,
};

#define DEFAULT_BROKER_CASH            (1000000)  // 100M

// for Chinese future exchange
#define DEFAULT_TRADING_DAY_END_TIME   (151500)

typedef struct {
    enum {
        Unknown = 0,
        Stock,
        Future,
        Forex,
        Option
    };
} SecurityType;

typedef struct {
    enum {
        NO_COMMISSION,
        FIXED_PER_TRADE,
        TRADE_PERCENTAGE    // percent of trade value
    };
} CommissionType;

typedef struct {
    enum {
        NO_SLIPPAGE,
        FIXED_PER_TRADE,
        TRADE_PERCENTAGE
    };
} SlippageType;

typedef long long int64;

typedef struct {
    enum {
        Unknown = 0,
        EntryLong,
        IncreaseLong,
        ReduceLong,
        ExitLong,
        EntryShort,
        IncreaseShort,
        ReduceShort,
        ExitShort,
        StopLoss,
        TakeProfit,
    };
} SignalType;

enum ParamType {
    PARAM_TYPE_STRING = 1,
    PARAM_TYPE_INT,
    PARAM_TYPE_DOUBLE,
    PARAM_TYPE_BOOL,
};

typedef struct {
    string name;
    int    type;
    string value;

    string start;
    string end;
    string step;
} ParamItem;

typedef struct {
    double start;
    double end;
    double step;
    long   count;
} ParamRange;

typedef std::vector<ParamItem>  ParamTuple;
typedef std::vector<ParamTuple> ParamSpace;

typedef struct _Contract {
    int    securityType;
    char   instrument[32];
    char   productId[32];
    int    exchange;
    double multiplier;
    double tickSize;
    double marginRatio;
    int    commType;
    double commission;
    int    slippageType;
    double slippage;
    int    openTime;
    int    closeTime;
    _Contract()
    {
        securityType    = SecurityType::Unknown;
        instrument[0]   = '\0';
        productId[0]    = '\0';
        multiplier      = 1;
        tickSize        = 1;
        marginRatio     = 0;
        commType        = CommissionType::TRADE_PERCENTAGE;
        commission      = 3.0 / 10000; // TODO:
        slippageType    = SlippageType::TRADE_PERCENTAGE;
        slippage        = 0;
        openTime        = 91500;  // 09:15:00 by default
        closeTime       = 145900; // 14:59:00 by default
    }
} Contract;

typedef struct {
    string   instrument;
    int      resolution;
    int      interval;
    Contract contract;
} InstrumentDesc;

typedef vector<InstrumentDesc> InstrumentList;

typedef struct {
    int      id;
    string   instrument;
    DateTime begin;
    DateTime end;
} SessionItem;

typedef vector<SessionItem> SessionTable;

typedef struct {
    long begin;
    long end;
} TradablePeriod;

typedef vector<TradablePeriod> TradingSession;

enum {
    DATA_FILE_FORMAT_UNKNOWN,
    DATA_FILE_FORMAT_CSV,
    DATA_FILE_FORMAT_BIN,
    DATA_FILE_FORMAT_TS
};

enum DataRequestType {
    BarsBack,
    DateTimeRange,
};

enum DataLoadMode {
    DataLoadSyncMode = 1,
    DataLoadAsyncMode,  // Not supported yet.
};

typedef struct _DataRequest {
    int      type;
    string   instrument;
    int      resolution;
    int      interval;
    DateTime from;
    DateTime to;
    bool     allOrNone;
    int      count;
    int      mode;

    _DataRequest()
    {
        resolution = 0;
        interval   = 1;
        allOrNone  = true;
        count      = 0;
        mode       = DataLoadSyncMode;
    }

} DataRequest;

typedef struct {
    double    initialCapital;
    long long tradingPeriod;
    double    finalPortfolioValue;
    // 'Account Size Required' calculates the amount of money you must 
    // have in your account to trade the strategy.
    double   acctSizeRequired;
    double   cumReturn;
    double   annualReturn;
    double   monthlyReturn;
    double   totalNetProfits;
    double   maxDD;                  // Max. Drawdown.
    double   maxDDPercentage;
    double   maxCTCDD;               // Max. Close To Close Drawdown.
    DateTime maxDDBegin;
    DateTime maxDDEnd;
    int      longestDDDuration;      // Longest Drawdown Duration, in days
    DateTime longestDDDBegin;
    DateTime longestDDDEnd;
    double   retOnMaxDD;             // Return on max drawdown.
    // The sum of money you would make compared to the sum of money required
    // to trade the strategy, after considering the margin and margin calls.
    // This value is calculated by dividing the net profit by the account
    // size required.
    // For leverage trading this value is more important than the Total Net 
    // Profit (e.g., buy or sell futures contracts or stocks on margin).
    // When trading using leverage, you must a sizable margin deposit.
    // The margin size is a certain percent of the overall transaction cost.
    // In addition, when trading on leverage, you need sufficient money to 
    // counteract equity dips - the same as in futures and stock trading is 
    // named margin calls.
    double   retOnAcctSizeRequired;  // Return on Account Size Required.

    // Nobel laureate William Sharpe introduced the Sharpe Ratio, in 1996, 
    // under the name reward-to-variability ratio. This ratio is perhaps the
    // best known of the return to risk measures. The formula for the Sharpe
    // ratio is SR = (MR -RFR) / SD, where MR is the average return for period
    // (monthly), RFR is the risk-free rate of return. SD is the standard 
    // deviation of returns. Thus, this formula yields a value that could be
    // loosely defined as return per unit risked if we accept the premise 
    // that variability is risk. The higher Sharpe ratio the smoother the
    // equity curve on a monthly basis. Having a smooth equity curve is a very
    // important objective for many traders. That is why this ratio is widely
    // used both at individual market and at a portfolio level.
    double   sharpeRatio;

    // The total number of trades (both winning and losing) generated by
    // a strategy. The total number of trades is important for a number of
    // reasons. For example, no matter how large is the strategies Total
    // Net Profit, one must be sure the value is statistically valid, i.e. 
    // the number of trades is large enough. Also important is the relation
    // between the number of trades and time; even good, profitable trades
    // may be taking place too rarely or too frequently for your needs.
    long     totalTrades;

    // The number of positions currently opened.
    long     totalOpenTrades;
    // The total number of winning trades generated by a strategy.
    long     winningTrades;
    // The total number of losing trades generated by a strategy.
    long     losingTrades;

    // The percentage of winning trades generated by a strategy.
    // Calculated by dividing the number of winning trades by total
    // number of trades generated by a strategy. Percent profitable is not a
    // very reliable measure by itself since approaches to profitability can
    // differ. A strategy could have many small winning trades, making the 
    // percent profitable high with a small average winning trade, or a few big
    // winning trades accounting for a low percent profitable and a big average
    // winning trade. Many successful strategies have a percent profitability 
    // below 50% but are still profitable due to proper loss control.
    double   percentProfitable;

    // The Gross Profits divided by the number of Winning Trades.
    double   avgWinningTrade; 

    // The Gross Loss divided by the number of Losing Trades.
    double   avgLosingTrade;

    // The average value of how many $ you win for every $ you lose.
    // This is calculated by dividing the average winning trades by the average
    // losing trade. This field is not a very meaningful value by itself, because
    // strategies can have different approaches to profitability. A strategy may
    // attempt trading at every possibility in order to capture many small profits
    // yet have an average losing trade greater than the average winning trade.
    // The higher this value is the better, but it should be regarded together with
    // the percentage of winning trades and the net profit.
    double   ratioAvgWinAvgLoss;

    double   avgProfit;
    double   maxProfit;
    double   minProfit;
    double   grossProfit;
    double   grossLoss;
    double   slippagePaid;
    double   commissionPaid;
} BacktestingMetrics;

typedef struct {
    double cumReturns;
    double totalNetProfits;
    double sharpeRatio;
    double maxDrawDown;
    double retOnMaxDD;
} SimplifiedMetrics;

typedef struct {
    DateTime tradingDay;
    double   closedProfit;
    double   realizedProfit;
    double   posProfit;
    int      tradesNum;
    int      openVolume;
    int      tradedVolume;
    int      todayPosition;    
    double   commissions;
    double   slippages;
    double   tradeCost;

    double   cash;
    double   equity;
    double   profit;
    double   margin;

    double   cumRealizedProfit;
    int      cumTradesNum;
    int      cumTradedVolume;
    int      cumCloseVolume;
    double   cumCommissions;
    double   cumSlippages;
    double   cumTradeCost;
} DailyMetrics;

class Strategy;

extern "C" typedef Strategy *StrategyCreator();

} // namespace xBacktest

#endif // DEFINES_H