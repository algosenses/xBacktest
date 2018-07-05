#ifndef XBACKTEST_EXECUTOR_H
#define XBACKTEST_EXECUTOR_H

#include "Defines.h"
#include "Config.h"
#include "Thread.h"
#include "Lock.h"
#include "Semaphore.h"
#include "Condition.h"
#include "Event.h"
#include "Dispatcher.h"
#include "Order.h"
#include "BarFeed.h"
#include "DataStorage.h"
#include "Backtesting.h"
#include "Strategy.h"
#include "Trades.h"
#include "Sharpe.h"
#include "Drawdown.h"
#include "Returns.h"
#include "Simulator.h"

class Strategy;

namespace xBacktest
{

class Executor;

// One Strategy + One Parameter tuple = One Model.
// One Model + One Main instrument data stream = One Process.
class Process
{
public:
    Process(Executor* executor, const StrategyConfig& config);
    ~Process();
    const string& getName() const;
    void registerDataStreamId(int id);
    const unordered_set<int>& getDataStreamIds() const;
    void subscribeInstrument(const string& instrument);
    void subscribeAll();
    void registerContract(const Contract& contract);
    const unordered_set<string>& getInstruments() const;
    void processNewBar(int dataStreamId, int feedId, const Bar& bar);
    void processNewOrder(const OrderEvent& evt);
    void processTimeElapsed(const DateTime& prevDateTime, const DateTime& nextDateTime);
    void processHistoricalData(int dataStreamId, const Bar& bar);
    Executor* getExecutor();
    unsigned long getNextOrderId();
    unsigned long getNextRuntimeId();
    void getTransactionRecords(vector<Transaction>& outRecords);
    int  loadData(const DataRequest& request);
    void stop();
    void destroy();

private:
    Executor* m_executor;
    bool      m_subscribeAll;

    unordered_set<int>    m_dataStreamIds;
    unordered_set<string> m_instruments;
    vector<Contract>      m_contracts;

    StrategyConfig m_config;

    string m_name;

    vector<Runtime*> m_runtimeList;
};

////////////////////////////////////////////////////////////////////////////////
// Process executor, responsible for:
// 1. transfer bar from data feed into process
// 2. handle transaction using back-testing broker
// 3. record process's performance
// 4. load historical data
// 5. ...
////////////////////////////////////////////////////////////////////////////////
class Executor : public IEventHandler
{
public:
    enum State {
        Idle,
        Running,
        Pause,
        Stop,
    };

    Executor();
    ~Executor();

    void setId(unsigned long id);
    unsigned long getId() const;
    void setTag(unsigned long tag);
    unsigned long getTag() const;
    void registerDataStorage(DataStorage* storage);
    vector<SessionItem>& getSessionTable();
    void getSessionTimeRange(DateTime& begin, DateTime& end);
    void registerStrategy(const StrategyConfig& config);
    void setCash(double cash);
    void setBrokerConfig(const BrokerConfig& config);
    void init();
    double getAvailableCash() const;
    double getEquity() const;
    int loadData(const DataRequest& request);
    BacktestingBroker* getBroker() const;
    void setSignalLamp(Utils::DefaultSemaphoreType& sema);
    bool isStopped();
    SimplifiedMetrics getSimplifiedMetrics();
    void enableDailyMetricsReport();
    void disableDailyMetricsReport();
    void setTradingDayEndTime(int timenum);
    void calculatePerformanceMetrics(BacktestingMetrics& metrics);
    void printBacktestingReport(const BacktestingMetrics& metrics);
    void saveBacktestingReport(const ReportConfig& desc, const BacktestingMetrics& metrics);
    void writeDebugMsg(const char* msg);

    // Call once (and only once) to run the strategy.
    void run();
    // Stops a running strategy.
    void stop();

    void wait();

    void onEvent(int type, const DateTime& datetime, const void *context);

    // Orders in the same executor has a unique id.
    // Because executor has a unique broker.
    unsigned long getNextOrderId();
    
    unsigned long getNextRuntimeId();

private:
    bool createSessionTable();
    void registerBarFeeds(vector<BarFeed*>& feeds);
    void formatOutput(stringstream& oss, const BacktestingMetrics& metrics);
    void savePerformanceSummary(const string& file, const BacktestingMetrics& metrics);
    void saveReturnRecords(const string& file);
    void saveEquityRecords(const string& file);
    void saveTradeRecords(const string& file);
    void saveTransactionRecords(const string& file);
    void saveDailyMetrics(const string& file);
    void onNewBarEvent(int dataStreamId, int feedId, const Bar& bar);
    void onNewOrderEvent(const OrderEvent& evt);
    void onTimeElapsedEvent(const DateTime& prevDateTime, const DateTime& nextDateTime);
    static void threadProc(void *const context);
    static void dataCallBack(const DateTime& datetime, void* ctx);

private:
    unsigned long m_id;
    unsigned long m_tag;

    double m_cash;

    Utils::Mutex m_stateMutex;
    volatile int m_state;

    Utils::Condition* m_monitor;
    Utils::DefaultSemaphoreType* m_semaphore;

    // Reference to bar feeds which inside data storage.
    vector<BarFeed*> m_clonedBarFeeds;
    DateTime m_earliestDateTime;
    DateTime m_latestDataTime;

    unsigned long m_historicalDataReqId;

    // roll main instrument of futures (roll trigger)
    vector<SessionItem> m_sessionTable;

    // multi-process in one executor, use for strategy combined testing.
    vector<Process*> m_processList;

    // analyzers
    ReturnsAnalyzerBase  m_returnsAnalyzerBase;    // Underlying returns analyzer

    Returns              m_retAnalyzer;
    Trades               m_tradesAnalyzer;
    SharpeRatio          m_sharpeRatioAnalyzer;
    DrawDown             m_drawDownAnalyzer;

    DataStorage*       m_dataStorage;
    BacktestingBroker* m_backtestBroker;
    // Dedicated to dispatch bars.
    Dispatcher*        m_dispatcher;

    Utils::Thread m_thread;

    volatile unsigned long m_nextOrderId;
    volatile unsigned long m_nextRuntimeId;
};

} // namespace xBacktest

#endif // XBACKTEST_EXECUTOR_H