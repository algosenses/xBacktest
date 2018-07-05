#ifndef XBACKTEST_CONFIG_IMPL_H
#define XBACKTEST_CONFIG_IMPL_H

#include <string>
#include <vector>
#include <unordered_set>

#include "Config.h"

namespace xBacktest
{

class DataStorage;

////////////////////////////////////////////////////////////////////////////////
#define DEFAULT_STRATEGY_NAME         "Strategy"
#define DEFAULT_SYMBOL_NAME           "Symbol"

class StrategyConfigImpl
{
public:
    StrategyConfigImpl();
    ~StrategyConfigImpl();

    void   setName(const string& name);
    string getName() const;
    void   setDescription(const string& desc);
    void   setAuthor(const string& author);
    void   setSharedLibrary(const string& library);
    string getSharedLibrary() const;
    void   setCreator(StrategyCreator* creator);
    StrategyCreator* getCreator() const;
    void   registerParameter(const ParamItem& item);
    void   setUserParameter(const string& name, int value);
    void   setUserParameter(const string& name, double value);
    void   setUserParameter(const string& name, bool value);
    void   setUserParameter(const string& name, const string& value);
    void   setOptimizationParameter(const string& name, double start, double end, double step);
    void   subscribe(const string& instrument, int resolution = Bar::TICK, int interval = 0);
    void   subscribeDataStream(const string& name);
    void   subscribeAll();
    bool   isSubscribeAll() const;
    const string& getSubscribedInstrument() const;
    const InstrumentList& getSubscribedInstruments() const;
    const unordered_set<string>& getSubscribedDataStreams() const;
    void  setParameters(ParamTuple& tuple);
    const ParamTuple&  getParameters();
    bool   check() const;

private:
    enum {
        SOURCE_FROM_FILE,
        SOURCE_FROM_OBJECT,
    };
    int m_strategySourceType;

    string m_name;
    string m_description;
    string m_author;
    string m_sharedLibrary;
    bool   m_subscribeAll;
    InstrumentList m_instruments;
    std::unordered_set<string> m_dataStreams;
    ParamTuple m_userParams;
    StrategyCreator* m_creator;
};

////////////////////////////////////////////////////////////////////////////////
class DataFeedConfigImpl
{
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
    vector<DataStreamConfig> m_items;

    DataStorage* m_storage;
};

////////////////////////////////////////////////////////////////////////////////
class BrokerConfigImpl
{
public:
    BrokerConfigImpl();
    void   setCash(double cash);
    double getCash() const;
    bool   setTradingDayEndTime(const string& time);
    int    getTradingDayEndTime() const;

private:
    double m_cash;
    int m_tradingDayEndTime;
};

////////////////////////////////////////////////////////////////////////////////
class EnvironmentConfigImpl
{
public:
    EnvironmentConfigImpl();
    void setMachineCPUNum(int num);
    int  getMachineCPUNum() const;
    void setOptimizationMode(int mode);
    int  getOptimizationMode() const;

private:
    int m_coreNum;
    int m_optimizationMode;
};

////////////////////////////////////////////////////////////////////////////////
#define DEFAULT_SUMMARY_FILENAME          "Summary.txt"
#define DEFAULT_DAILY_METRICS_FILENAME    "DailyMetrics.csv"
#define DEFAULT_TRADE_RECORDS_FILENAME    "Trades.csv"
#define DEFAULT_POSITION_RECORDS_FILENAME "Positions.csv"
#define DEFAULT_RETURN_RECORDS_FILENAME   "Returns.csv"
#define DEFAULT_EQUITIES_FILENAME         "Equities.csv"
#define DEFAULT_OPTIMIZATION_FILENAME     "Optimization.csv"

class ReportConfigImpl
{
public:
    ReportConfigImpl();
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
    void setEquitiesFile(const string& filename);
    const string& getEquitiesFile() const;
    void setOptimizationFile(const string& filename);
    const string& getOptimizationFile() const;

private:
    unsigned long m_mask;
    string m_summaryFile;
    string m_dailyMetricsFile;
    string m_tradesFile;
    string m_positionsFile;
    string m_returnsFile;
    string m_equitiesFile;
    string m_optimizationFile;
};

} // namespace xBacktest

#endif // XBACKTEST_CONFIG_IMPL_H