#ifndef XBACKTEST_SIMULATOR_IMPL_H
#define XBACKTEST_SIMULATOR_IMPL_H

#include "Config.h"

namespace xBacktest
{

#define VER_MAJOR_NUM       (0)
#define VER_MINOR_NUM       (1)

class StrategyImpl;
class Backtester;
class Optimizer;
class DataStorage;

class SimulatorImpl
{
public:
    SimulatorImpl();
    ~SimulatorImpl();

    BrokerConfig&      getBrokerConfig();
    EnvironmentConfig& getEnvironmentConfig();
    DataFeedConfig&    getDataFeedConfig();
    StrategyConfig     createNewStrategyConfig();
    void               setRunningMode(int mode);
    bool               loadScenario(const string& filename);
    bool               loadStrategy(StrategyConfig& config);
    bool               loadStrategy(StrategyCreator* creator);
    void               run();
    void               writeLogMsg(int level, const char* msg) const;
    int                version() const;

    // helper functions
    void               setMachineCPUNum(int num);
    void               setDataSeriesMaxLength(int length);
    bool               registerDataStream(const string& symbol,  // 
                                          int resolution,
                                          int interval,
                                          const string& filename,
                                          int format = DATA_FILE_FORMAT_UNKNOWN);
    void               setMultiplier(const string& symbol, double multiplier);
    void               setTickSize(const string& symbol, double size);
    void               setMarginRatio(const string& symbol, double ratio);
    void               setCommissionType(const string& symbol, int type);
    void               setCommission(const string& symbol, double comm);
    void               setSlippageType(const string& symbol, int type);
    void               setSlippage(const string& symbol, double slippage);
    void               setCash(double cash);
    void               useTickDataToStopPos(bool useTickData);
    void               enableReport(int mask);
    void               disableReport(int mask);
    void               setPositionsReportFile(const string& filename);

private:
    bool parseScenario(const string& filename, StrategyConfig& config);
    bool loadSharedLibrary(StrategyConfig& config);
    bool loadDataFeed();
    bool sanityCheck(const StrategyConfig& config);
    bool initRunningMode(vector<StrategyConfig>& strategies);
    void runBacktest();
    void runOptimizing();

private:
    int m_runningMode;
    int m_maxOptimizingThreadNum;
        
    // Common configuration.
    EnvironmentConfig m_envConfig;
    BrokerConfig      m_brokerConfig;
    DataFeedConfig    m_dataFeedConfig;
    ReportConfig      m_reportConfig;

#if _WIN32  // Defined for applications for Win32 and Win64. Always defined.
    HMODULE m_module;
#endif

    DataStorage* m_storage;
    Backtester*  m_backtester;
    Optimizer*   m_optimizer;

    vector<StrategyConfig> m_strategyConfigs;
};

} // namespace xBacktest

#endif // XBACKTEST_SIMULATOR_IMPL_H