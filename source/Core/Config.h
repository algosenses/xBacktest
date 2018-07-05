#ifndef XBACKTEST_CONFIG_H
#define XBACKTEST_CONFIG_H

#include <string>
#include <vector>
#include <unordered_set>

#include "Defines.h"
#include "Export.h"
#include "Bar.h"

using std::string;
using std::vector;
using std::unordered_set;

namespace xBacktest
{

class DataStorage;
class StrategyConfigImpl;
class DataFeedConfigImpl;
class BrokerConfigImpl;
class EnvironmentConfigImpl;
class ReportConfigImpl;

////////////////////////////////////////////////////////////////////////////////
class DllExport StrategyConfig
{
    friend class SimulatorImpl;

public:
    StrategyConfig(const StrategyConfig&);
    ~StrategyConfig();
    StrategyConfig &operator=(const StrategyConfig &);

    void   setName(const string& name);
    string getName() const;
    void   setDescription(const string& desc);
    void   setAuthor(const string& author);
    void   setSharedLibrary(const string& library);
    string getSharedLibrary() const;
    void   setCreator(StrategyCreator* creator);
    StrategyCreator* getCreator() const;
    StrategyConfig& registerParameter(const ParamItem& item);
    StrategyConfig& setUserParameter(const string& name, int value);
    StrategyConfig& setUserParameter(const string& name, double value);
    StrategyConfig& setUserParameter(const string& name, bool value);
    StrategyConfig& setUserParameter(const string& name, const string& value);
    StrategyConfig& setOptimizationParameter(const string& name, double start, double end, double step);
    // Subscribe instrument.
    void   subscribe(const string& instrument, int resolution = Bar::TICK, int interval = 0);
    // Subscribe data stream, one data stream may contains many instruments.
    void   subscribeDataStream(const string& name);
    void   subscribeAll();
    bool   isSubscribeAll() const;
    void   setSessionTable(const SessionTable& session);
    bool   check() const;
    // Get the first instrument.
    const string& getSubscribedInstrument() const;
    const InstrumentList& getSubscribedInstruments() const;
    const unordered_set<string>& getSubscribedDataStreams() const;
    void  setParameters(ParamTuple& tuple);
    const ParamTuple& getParameters() const;

private:
    StrategyConfig();

private:
    StrategyConfigImpl* m_implementor;
};

////////////////////////////////////////////////////////////////////////////////
typedef struct {
    string   name;
    int      resolution;
    int      interval;
    bool     realtime;
    int      type;
    string   uri;
    int      format;
    Contract contract;
} DataStreamConfig;

class DllExport DataFeedConfig
{
    friend class SimulatorImpl;

public:
    void registerStream(const string& symbol, int resolution, int interval, const string& filename, int format);
    void registerStream(DataStreamConfig& stream);
    void setMultiplier(const string& symbol, double multiplier);
    void setTickSize(const string& symbol, double size);
    void setMarginRatio(const string& symbol, double ratio);
    void setCommissionType(const string& symbol, int type);
    void setCommission(const string& symbol, double comm);
    void setSlippageType(const string& symbol, int type);
    void setSlippage(const string& symbol, double slippage);
    const Contract getContract(const string& symbol) const;
    void setBarStorage(DataStorage* storage);
    vector<DataStreamConfig>& getStreams();
    DataStorage* getBarStorage();

private:
    DataFeedConfig();
    ~DataFeedConfig();
    DataFeedConfig(const DataFeedConfig &);
    DataFeedConfig &operator=(const DataFeedConfig &);

private:
    DataFeedConfigImpl* m_implementor;
};

////////////////////////////////////////////////////////////////////////////////
class DllExport BrokerConfig
{
    friend class SimulatorImpl;

public:
    void   setCash(double cash);
    double getCash() const;
    bool   setTradingDayEndTime(const string& time);
    int    getTradingDayEndTime() const;

private:
    BrokerConfig();
    ~BrokerConfig();
    BrokerConfig(const BrokerConfig &);
    BrokerConfig &operator=(const BrokerConfig &);

private:
    BrokerConfigImpl* m_implementor;
};

////////////////////////////////////////////////////////////////////////////////
class DllExport EnvironmentConfig
{
    friend class SimulatorImpl;

public:
    void setMachineCPUNum(int num);
    int  getMachineCPUNum() const;
    void setOptimizationMode(int mode);
    int  getOptimizationMode() const;

private:
    EnvironmentConfig();
    ~EnvironmentConfig();
    EnvironmentConfig(const EnvironmentConfig &);
    EnvironmentConfig &operator=(const EnvironmentConfig &);

private:
    EnvironmentConfigImpl* m_implementor;
};

////////////////////////////////////////////////////////////////////////////////
class DllExport ReportConfig
{
    friend class SimulatorImpl;

public:
    enum {
        REPORT_SUMMARY       = 0x01,
        REPORT_TRADES        = 0x02,
        REPORT_POSITION      = 0x04,
        REPORT_RETURNS       = 0x08,
        REPORT_EQUITIES      = 0x10,
        REPORT_DAILY_METRICS = 0x20,
        REPORT_OPTIMIZATION  = 0x40
    };

    void enableReport(int mask);
    void disableReport(int mask);
    bool isReportEnable(int mask) const;
    void setSummaryFile(const string& filename);
    const string& getSummaryFile() const;
    void setDailyMetricsFile(const string& filename);
    const string& getDailyMetricsFile() const;
    void setTradeLogFile(const string& filename); 
    const string& getTradeLogFile() const;
    void setPositionsFile(const string& filename);
    const string& getPositionsFile() const;
    void setReturnsFile(const string& filename);
    const string& getReturnsFile() const;
    const string& getEquitiesFile() const;
    void setOptimizationFile(const string& filename);
    const string& getOptimizationFile() const;

private:
    ReportConfig();
    ~ReportConfig();
    ReportConfig(const ReportConfig &) {}
    ReportConfig &operator=(const ReportConfig &);

private:
    ReportConfigImpl* m_implementor;
};

} // namespace xBacktest

#endif // XBACKTEST_CONFIG_H