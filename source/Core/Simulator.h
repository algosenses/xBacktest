#ifndef XBACKTEST_SIMULATOR_H
#define XBACKTEST_SIMULATOR_H

#include "Defines.h"
#include "Config.h"

namespace xBacktest
{

class Strategy;
class SimulatorImpl;

class DllExport Simulator
{
public:
    enum RunningMode {
        Backtest,
        Optimization,
    };

    static Simulator*  instance();
    void               release();
    BrokerConfig&      getBrokerConfig();
    EnvironmentConfig& getEnvironmentConfig();
    DataFeedConfig&    getDataFeedConfig();
    StrategyConfig     createNewStrategyConfig();
    void               setRunningMode(int mode);
    bool               loadScenario(const char* filename);
    bool               loadStrategy(StrategyConfig& config);
    bool               loadStrategy(StrategyCreator* creator);
    void               run();
    void               writeLogMsg(int level, const char* msgFmt, ...) const;
    int                version() const;

    // helper functions
    void               setMachineCPUNum(int num);
    void               setDataSeriesMaxLength(int length);
    bool               registerDataStream(const char* symbol, 
                                          int resolution, 
                                          int interval, 
                                          const char* filename, 
                                          int format = DATA_FILE_FORMAT_UNKNOWN);
    void               registerContract(const char* symbol, const Contract& contract);
    bool               enableComposer(const char* symbol, const TradingSession& session, int resolution, int interval = 1);
    void               setMultiplier(const char* symbol, double multiplier);
    void               setTickSize(const char* symbol, double size);
    void               setMarginRatio(const char* symbol, double ratio);
    void               setCommissionType(const char* symbol, int type);
    void               setCommission(const char* symbol, double comm);
    void               setSlippageType(const char* symbol, int type);
    void               setSlippage(const char* symbol, double slippage);
    void               setCash(double cash);
    void               useTickDataToStopPos(bool useTickData);
    void               enableReport(int mask);
    void               disableReport(int mask);
    void               setPositionsReportFile(const char* filename);

private:
    Simulator();
    Simulator(Simulator const&);
    Simulator& operator=(Simulator const&);
    ~Simulator();

private:
    static Simulator* m_instance;
    SimulatorImpl* m_implementor;
};

} // namespace xBacktest

#endif // XBACKTEST_SIMULATOR_H