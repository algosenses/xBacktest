#include "ConfigImpl.h"
#include "Utils.h"
#include "Optimizer.h"

namespace xBacktest
{

StrategyConfigImpl::StrategyConfigImpl()
{
    m_strategySourceType = SOURCE_FROM_OBJECT;
    m_name = DEFAULT_STRATEGY_NAME;

    m_instruments.clear();
    m_userParams.clear();
    m_subscribeAll = false;
}

StrategyConfigImpl::~StrategyConfigImpl()
{
}

void StrategyConfigImpl::setName(const string& name)
{
    m_name = name;
}

string StrategyConfigImpl::getName() const
{
    return m_name;
}

void StrategyConfigImpl::setDescription(const string& desc)
{
    m_description = desc;
}

void StrategyConfigImpl::setAuthor(const string& author)
{
    m_author = author;
}

void StrategyConfigImpl::setSharedLibrary(const string& library)
{
    m_sharedLibrary = library;
}

string StrategyConfigImpl::getSharedLibrary() const
{
    return m_sharedLibrary;
}

void StrategyConfigImpl::setCreator(StrategyCreator* creator)
{
    m_creator = creator;
}

StrategyCreator* StrategyConfigImpl::getCreator() const
{
    return m_creator;
}

void StrategyConfigImpl::subscribe(const string& instrument, int resolution, int interval)
{
    for (size_t i = 0; i < m_instruments.size(); i++) {
        if (m_instruments[i].instrument == instrument) {
            m_instruments[i].resolution = resolution;
            m_instruments[i].interval   = interval;
            return;
        }
    }

    InstrumentDesc desc;
    desc.instrument = instrument;
    desc.resolution = resolution;
    desc.interval   = interval;

    m_instruments.push_back(desc);
}

void StrategyConfigImpl::subscribeDataStream(const string& name)
{
    m_dataStreams.insert(name);
}

void StrategyConfigImpl::subscribeAll()
{
    m_subscribeAll = true;
}

bool StrategyConfigImpl::isSubscribeAll() const
{
    return m_subscribeAll;
}

const string& StrategyConfigImpl::getSubscribedInstrument() const
{
    static string nullStr = "";

    if (m_instruments.size() > 0) {
        return m_instruments[0].instrument;
    }

    return nullStr;
}

const InstrumentList& StrategyConfigImpl::getSubscribedInstruments() const
{
    return m_instruments;
}

const unordered_set<string>& StrategyConfigImpl::getSubscribedDataStreams() const 
{
    return m_dataStreams;
}

void StrategyConfigImpl::setParameters(ParamTuple& tuple)
{
    m_userParams = tuple;
}

const ParamTuple& StrategyConfigImpl::getParameters()
{
    return m_userParams;
}

void StrategyConfigImpl::registerParameter(const ParamItem& item)
{
    m_userParams.push_back(item);
}

void StrategyConfigImpl::setUserParameter(const string& name, int value)
{
    char str[64];
    memset(str, 0, sizeof(str));
    _snprintf(str, sizeof(str)-1, "%d", value);

    ParamItem item;
    item.type      = PARAM_TYPE_INT;
    item.name      = name;
    item.value     = str;
    item.start     = str;
    item.end       = str;
    item.step      = '1';

    for (size_t i = 0; i < m_userParams.size(); i++) {
        if (m_userParams[i].name == name) {
            m_userParams[i] = item;
            return;
        }
    }

    m_userParams.push_back(item);
}

void StrategyConfigImpl::setUserParameter(const string& name, double value)
{
    char str[64];
    memset(str, 0, sizeof(str));
    _snprintf(str, sizeof(str)-1, "%f", value);

    ParamItem item;
    item.type = PARAM_TYPE_DOUBLE;
    item.name = name;
    item.value = str;
    item.start = str;
    item.end = str;
    item.step = '1';

    for (size_t i = 0; i < m_userParams.size(); i++) {
        if (m_userParams[i].name == name) {
            m_userParams[i] = item;
            return;
        }
    }

    m_userParams.push_back(item);
}

void StrategyConfigImpl::setUserParameter(const string& name, bool value)
{
    ParamItem item;
    item.type = PARAM_TYPE_BOOL;
    item.name = name;
    item.value = value ? "true" : "false";
    item.start = '0';
    item.end = '0';
    item.step = '1';

    for (size_t i = 0; i < m_userParams.size(); i++) {
        if (m_userParams[i].name == name) {
            m_userParams[i] = item;
            return;
        }
    }

    m_userParams.push_back(item);
}

void StrategyConfigImpl::setUserParameter(const string& name, const string& value)
{
    ParamItem item;
    item.type  = PARAM_TYPE_STRING;
    item.name  = name;
    item.value = value;
    item.start = '0';
    item.end   = '0';
    item.step  = '1';

    for (size_t i = 0; i < m_userParams.size(); i++) {
        if (m_userParams[i].name == name) {
            m_userParams[i] = item;
            return;
        }
    }

    m_userParams.push_back(item);
}

void StrategyConfigImpl::setOptimizationParameter(const string& name, double start, double end, double step)
{
    char str[64];
    memset(str, 0, sizeof(str));
    _snprintf(str, sizeof(str)-1, "%f", start);

    ParamItem item;
    item.type = PARAM_TYPE_DOUBLE;
    item.name = name;
    item.value = str;
    item.start = str;

    _snprintf(str, sizeof(str)-1, "%f", end);
    item.end = str;

    _snprintf(str, sizeof(str)-1, "%f", step);
    item.step = str;

    for (size_t i = 0; i < m_userParams.size(); i++) {
        if (m_userParams[i].name == name) {
            m_userParams[i] = item;
            return;
        }
    }

    m_userParams.push_back(item);
}

bool StrategyConfigImpl::check() const
{
    if (m_name.empty()) {
        return false;
    }

    if (m_sharedLibrary.empty()) {
    }

    if (m_instruments.size() == 0) {
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
void DataFeedConfigImpl::registerStream(const string& symbol, int resolution, int interval, const string& filename, int format)
{
    for (size_t i = 0; i < m_items.size(); i++) {
        if (m_items[i].name == symbol) {
            char str[128] = { 0 };
            sprintf(str, "Data stream '%s' already existed.", symbol.c_str());
            ASSERT(false, str);
        }
    }

    DataStreamConfig stream;
    stream.name       = symbol;
    stream.uri        = filename;
    stream.format     = format;
    stream.resolution = resolution;
    stream.interval   = interval;

    strncpy(stream.contract.productId, symbol.c_str(), sizeof(stream.contract.productId) - 1);
    stream.contract.commType   = 0;
    stream.contract.commission = 0;
    stream.contract.multiplier = 1;
    stream.contract.tickSize   = 1;

    m_items.push_back(stream);
}

void DataFeedConfigImpl::registerStream(DataStreamConfig& stream)
{
    m_items.push_back(stream);
}

void DataFeedConfigImpl::setMultiplier(const string& symbol, double multiplier)
{
    for (size_t i = 0; i < m_items.size(); i++) {
        if (m_items[i].name == symbol) {
            m_items[i].contract.multiplier = multiplier;
            break;
        }
    }
}

void DataFeedConfigImpl::setTickSize(const string& symbol, double size)
{
    for (size_t i = 0; i < m_items.size(); i++) {
        if (m_items[i].name == symbol) {
            m_items[i].contract.tickSize = size;
            break;
        }
    }
}

void DataFeedConfigImpl::setMarginRatio(const string& symbol, double ratio)
{
    for (size_t i = 0; i < m_items.size(); i++) {
        if (m_items[i].name == symbol) {
            m_items[i].contract.marginRatio = ratio;
            break;
        }
    }
}

void DataFeedConfigImpl::setCommissionType(const string& symbol, int type)
{
    for (size_t i = 0; i < m_items.size(); i++) {
        if (m_items[i].name == symbol) {
            m_items[i].contract.commType = type;
            break;
        }
    }
}

void DataFeedConfigImpl::setCommission(const string& symbol, double comm)
{
    for (size_t i = 0; i < m_items.size(); i++) {
        if (m_items[i].name == symbol) {
            m_items[i].contract.commission = comm;
            break;
        }
    }
}

void DataFeedConfigImpl::setSlippageType(const string& symbol, int type)
{
    for (size_t i = 0; i < m_items.size(); i++) {
        if (m_items[i].name == symbol) {
            m_items[i].contract.slippageType = type;
            break;
        }
    }
}

void DataFeedConfigImpl::setSlippage(const string& symbol, double slippage)
{
    for (size_t i = 0; i < m_items.size(); i++) {
        if (m_items[i].name == symbol) {
            m_items[i].contract.slippage = slippage;
            break;
        }
    }
}

const Contract DataFeedConfigImpl::getContract(const string& symbol) const
{
    for (size_t i = 0; i < m_items.size(); i++) {
        if (m_items[i].name == symbol) {
            return m_items[i].contract;
        }
    }

    return Contract();
}

void DataFeedConfigImpl::setBarStorage(DataStorage* storage)
{
    m_storage = storage;
}

vector<DataStreamConfig>& DataFeedConfigImpl::getStreams()
{
    return m_items;
}

DataStorage* DataFeedConfigImpl::getBarStorage()
{
    return m_storage;
}

////////////////////////////////////////////////////////////////////////////////
BrokerConfigImpl::BrokerConfigImpl()
{
    m_cash = DEFAULT_BROKER_CASH;
    m_tradingDayEndTime = DEFAULT_TRADING_DAY_END_TIME;
}

void BrokerConfigImpl::setCash(double cash)
{
    m_cash = cash;
}

double BrokerConfigImpl::getCash() const
{
    return m_cash;
}

bool BrokerConfigImpl::setTradingDayEndTime(const string& time)
{
    if (time.length() != strlen("hh:mm:ss")) {
        return false;
    }

    char hourStr[3] = { 0 };
    char minStr[3] = { 0 };
    char secStr[3] = { 0 };

    const char *p = time.c_str();
    memcpy(hourStr, p, 2);
    p += 3;
    memcpy(minStr, p, 2);
    p += 3;
    memcpy(secStr, p, 2);

    int hour = atoi(hourStr);
    if (hour > 23) {
        return false;
    }

    int min = atoi(minStr);
    if (min > 59) {
        return false;
    }

    int sec = atoi(secStr);
    if (sec > 59) {
        return false;
    }

    m_tradingDayEndTime = hour * 10000 + min * 100 + sec;

    return true;
}

int BrokerConfigImpl::getTradingDayEndTime() const
{
    return m_tradingDayEndTime;
}

////////////////////////////////////////////////////////////////////////////////
EnvironmentConfigImpl::EnvironmentConfigImpl()
{
    m_coreNum = Utils::getMachineCPUNum();
    m_optimizationMode = Optimizer::Exhaustive;
}

void EnvironmentConfigImpl::setMachineCPUNum(int num)
{
    m_coreNum = num;
}

int EnvironmentConfigImpl::getMachineCPUNum() const
{
    return m_coreNum;
}

void EnvironmentConfigImpl::setOptimizationMode(int mode)
{
    m_optimizationMode = mode;
}

int EnvironmentConfigImpl::getOptimizationMode() const
{
    return m_optimizationMode;
}

////////////////////////////////////////////////////////////////////////////////
ReportConfigImpl::ReportConfigImpl()
{
    m_mask = ReportConfig::REPORT_SUMMARY | ReportConfig::REPORT_POSITION;

    m_summaryFile      = DEFAULT_SUMMARY_FILENAME;
    m_dailyMetricsFile = DEFAULT_DAILY_METRICS_FILENAME;
    m_tradesFile       = DEFAULT_TRADE_RECORDS_FILENAME;
    m_positionsFile    = DEFAULT_POSITION_RECORDS_FILENAME;
    m_returnsFile      = DEFAULT_RETURN_RECORDS_FILENAME;
    m_equitiesFile     = DEFAULT_EQUITIES_FILENAME;
    m_optimizationFile = DEFAULT_OPTIMIZATION_FILENAME;
}

void ReportConfigImpl::enableReport(int mask)
{
    m_mask |= mask;
}

void ReportConfigImpl::disableReport(int mask)
{
    m_mask &= (~mask);
}

bool ReportConfigImpl::isReportEnable(int mask) const
{
    return (m_mask & mask) != 0;
}

void ReportConfigImpl::setSummaryFile(const string& filename)
{
    m_summaryFile = filename;
}

const string& ReportConfigImpl::getSummaryFile() const
{
    return m_summaryFile;
}

void ReportConfigImpl::setDailyMetricsFile(const string& filename)
{
    m_dailyMetricsFile = filename;
}

const string& ReportConfigImpl::getDailyMetricsFile() const
{
    return m_dailyMetricsFile;
}

void ReportConfigImpl::setTradeLogFile(const string& filename)
{
    m_tradesFile = filename;
}

const string& ReportConfigImpl::getTradeLogFile() const
{
    return m_tradesFile;
}

void ReportConfigImpl::setPositionsFile(const string& filename)
{
    m_positionsFile = filename;
}

const string& ReportConfigImpl::getPositionsFile() const
{
    return m_positionsFile;
}

void ReportConfigImpl::setReturnsFile(const string& filename)
{
    m_returnsFile =  filename;
}

const string& ReportConfigImpl::getReturnsFile() const
{
    return m_returnsFile;
}

void ReportConfigImpl::setEquitiesFile(const string& filename)
{
    m_equitiesFile = filename;
}

const string& ReportConfigImpl::getEquitiesFile() const
{
    return m_equitiesFile;
}

void ReportConfigImpl::setOptimizationFile(const string& filename)
{
    m_optimizationFile = filename;
}

const string& ReportConfigImpl::getOptimizationFile() const
{
    return m_optimizationFile;
}

} // namespace xBacktest