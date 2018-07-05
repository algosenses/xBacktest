#include "ConfigImpl.h"

namespace xBacktest
{

StrategyConfig::StrategyConfig()
{
    m_implementor = new StrategyConfigImpl();
}

StrategyConfig::StrategyConfig(const StrategyConfig& config)
{
    m_implementor  = new StrategyConfigImpl();
    *m_implementor = *config.m_implementor;
}

StrategyConfig::~StrategyConfig()
{
    delete m_implementor;
    m_implementor = nullptr;
}

StrategyConfig &StrategyConfig::operator=(const StrategyConfig& config)
{
    delete m_implementor;

    m_implementor  = new StrategyConfigImpl();
    *m_implementor = *config.m_implementor;

    return *this;
}

void StrategyConfig::setName(const string& name)
{
    return m_implementor->setName(name);
}

string StrategyConfig::getName() const
{
    return m_implementor->getName();
}

void StrategyConfig::setDescription(const string& desc)
{
    return m_implementor->setDescription(desc);
}

void StrategyConfig::setAuthor(const string& author)
{
    return m_implementor->setAuthor(author);
}

void StrategyConfig::setSharedLibrary(const string& library)
{
    return m_implementor->setSharedLibrary(library);
}

string StrategyConfig::getSharedLibrary() const
{
    return m_implementor->getSharedLibrary();
}

void StrategyConfig::setCreator(StrategyCreator* creator)
{
    return m_implementor->setCreator(creator);
}

StrategyCreator* StrategyConfig::getCreator() const
{
    return m_implementor->getCreator();
}

void StrategyConfig::subscribe(const string& instrument, int resolution, int interval)
{
    return m_implementor->subscribe(instrument, resolution, interval);
}

void StrategyConfig::subscribeDataStream(const string& name)
{
    return m_implementor->subscribeDataStream(name);
}

void StrategyConfig::subscribeAll()
{
    return m_implementor->subscribeAll();
}

bool StrategyConfig::isSubscribeAll() const
{
    return m_implementor->isSubscribeAll();
}

void StrategyConfig::setSessionTable(const SessionTable& session)
{
}

const string& StrategyConfig::getSubscribedInstrument() const
{
    return m_implementor->getSubscribedInstrument();
}

const InstrumentList& StrategyConfig::getSubscribedInstruments() const
{
    return m_implementor->getSubscribedInstruments();
}

const unordered_set<string>& StrategyConfig::getSubscribedDataStreams() const
{
    return m_implementor->getSubscribedDataStreams();
}

void StrategyConfig::setParameters(ParamTuple& tuple)
{
    return m_implementor->setParameters(tuple);
}

const ParamTuple& StrategyConfig::getParameters() const
{
    return m_implementor->getParameters();
}

StrategyConfig& StrategyConfig::registerParameter(const ParamItem& item)
{
    m_implementor->registerParameter(item);

    return *this;
}

StrategyConfig& StrategyConfig::setUserParameter(const string& name, int value)
{
    m_implementor->setUserParameter(name, value);

    return *this;
}

StrategyConfig& StrategyConfig::setUserParameter(const string& name, double value)
{
    m_implementor->setUserParameter(name, value);

    return *this;
}

StrategyConfig& StrategyConfig::setUserParameter(const string& name, bool value)
{
    m_implementor->setUserParameter(name, value);

    return *this;
}

StrategyConfig& StrategyConfig::setUserParameter(const string& name, const string& value)
{
    m_implementor->setUserParameter(name, value);

    return *this;
}

StrategyConfig& StrategyConfig::setOptimizationParameter(const string& name, double start, double end, double step)
{
    m_implementor->setOptimizationParameter(name, start, end, step);

    return *this;
}

bool StrategyConfig::check() const
{
    return m_implementor->check();
}

////////////////////////////////////////////////////////////////////////////////
DataFeedConfig::DataFeedConfig()
{
    m_implementor = new DataFeedConfigImpl();
}

DataFeedConfig::~DataFeedConfig()
{
    delete m_implementor;
    m_implementor = nullptr;
}

DataFeedConfig &DataFeedConfig::operator=(const DataFeedConfig& config)
{
    delete m_implementor;

    m_implementor  = new DataFeedConfigImpl();
    *m_implementor = *config.m_implementor;

    return *this;
}

void DataFeedConfig::registerStream(const string& symbol, int resolution, int interval, const string& filename, int fromat)
{
    return m_implementor->registerStream(symbol, resolution, interval, filename, fromat);
}

void DataFeedConfig::registerStream(DataStreamConfig& stream)
{
    return m_implementor->registerStream(stream);
}

void DataFeedConfig::setMultiplier(const string& symbol, double multiplier)
{
    return m_implementor->setMultiplier(symbol, multiplier);
}

void DataFeedConfig::setTickSize(const string& symbol, double size)
{
    return m_implementor->setTickSize(symbol, size);
}

void DataFeedConfig::setMarginRatio(const string& symbol, double ratio)
{
    return m_implementor->setMarginRatio(symbol, ratio);
}

void DataFeedConfig::setCommissionType(const string& symbol, int type)
{
    return m_implementor->setCommissionType(symbol, type);
}

void DataFeedConfig::setCommission(const string& symbol, double comm)
{
    return m_implementor->setCommission(symbol, comm);
}

void DataFeedConfig::setSlippageType(const string& symbol, int type)
{
    return m_implementor->setSlippageType(symbol, type);
}

void DataFeedConfig::setSlippage(const string& symbol, double slippage)
{
    return m_implementor->setSlippage(symbol, slippage);
}

const Contract DataFeedConfig::getContract(const string& symbol) const
{
    return m_implementor->getContract(symbol);
}

void DataFeedConfig::setBarStorage(DataStorage* storage)
{
    return m_implementor->setBarStorage(storage);
}

vector<DataStreamConfig>& DataFeedConfig::getStreams()
{
    return m_implementor->getStreams();
}

DataStorage* DataFeedConfig::getBarStorage()
{
    return m_implementor->getBarStorage();
}

////////////////////////////////////////////////////////////////////////////////
BrokerConfig::BrokerConfig()
{
    m_implementor = new BrokerConfigImpl();
}

BrokerConfig::~BrokerConfig()
{
    delete m_implementor;
    m_implementor = nullptr;
}

BrokerConfig &BrokerConfig::operator=(const BrokerConfig& config)
{
    delete m_implementor;

    m_implementor  = new BrokerConfigImpl();
    *m_implementor = *config.m_implementor;

    return *this;
}

void BrokerConfig::setCash(double cash)
{
    return m_implementor->setCash(cash);
}

double BrokerConfig::getCash() const
{
    return m_implementor->getCash();
}

bool BrokerConfig::setTradingDayEndTime(const string& time)
{
    return m_implementor->setTradingDayEndTime(time);
}

int BrokerConfig::getTradingDayEndTime() const
{
    return m_implementor->getTradingDayEndTime();
}

////////////////////////////////////////////////////////////////////////////////
EnvironmentConfig::EnvironmentConfig()
{
    m_implementor = new EnvironmentConfigImpl();
}

EnvironmentConfig::~EnvironmentConfig()
{
    delete m_implementor;
    m_implementor = nullptr;
}

EnvironmentConfig &EnvironmentConfig::operator=(const EnvironmentConfig& config)
{
    delete m_implementor;

    m_implementor  = new EnvironmentConfigImpl();
    *m_implementor = *config.m_implementor;

    return *this;
}

void EnvironmentConfig::setMachineCPUNum(int num)
{
    return m_implementor->setMachineCPUNum(num);
}

int EnvironmentConfig::getMachineCPUNum() const
{
    return m_implementor->getMachineCPUNum();
}

void EnvironmentConfig::setOptimizationMode(int mode)
{
    return m_implementor->setOptimizationMode(mode);
}

int EnvironmentConfig::getOptimizationMode() const
{
    return m_implementor->getOptimizationMode();
}

////////////////////////////////////////////////////////////////////////////////
ReportConfig::ReportConfig()
{
    m_implementor = new ReportConfigImpl();
}

ReportConfig::~ReportConfig()
{
    delete m_implementor;
    m_implementor = nullptr;
}

ReportConfig &ReportConfig::operator=(const ReportConfig& config)
{
    delete m_implementor;

    m_implementor  = new ReportConfigImpl();
    *m_implementor = *config.m_implementor;

    return *this;
}

void ReportConfig::enableReport(int mask)
{
    return m_implementor->enableReport(mask);
}

void ReportConfig::disableReport(int mask)
{
    return m_implementor->disableReport(mask); 
}

bool ReportConfig::isReportEnable(int mask) const
{
    return m_implementor->isReportEnable(mask);
}

void ReportConfig::setSummaryFile(const string& filename)
{
    return m_implementor->setSummaryFile(filename);
}

const string& ReportConfig::getSummaryFile() const
{
    return m_implementor->getSummaryFile();
}

void ReportConfig::setDailyMetricsFile(const string& filename)
{
    return m_implementor->setDailyMetricsFile(filename);
}

const string& ReportConfig::getDailyMetricsFile() const
{
    return m_implementor->getDailyMetricsFile();
}

void ReportConfig::setTradeLogFile(const string& filename)
{
    return m_implementor->setTradeLogFile(filename);
}

const string& ReportConfig::getTradeLogFile() const
{
    return m_implementor->getTradeLogFile();
}

void ReportConfig::setPositionsFile(const string& filename)
{
    return m_implementor->setPositionsFile(filename);
}

const string& ReportConfig::getPositionsFile() const
{
    return m_implementor->getPositionsFile();
}

void ReportConfig::setReturnsFile(const string& filename)
{
    return m_implementor->setReturnsFile(filename);
}

const string& ReportConfig::getReturnsFile() const
{
    return m_implementor->getReturnsFile();
}

const string& ReportConfig::getEquitiesFile() const
{
    return m_implementor->getEquitiesFile();
}

void ReportConfig::setOptimizationFile(const string& filename)
{
    return m_implementor->setOptimizationFile(filename);
}

const string& ReportConfig::getOptimizationFile() const
{
    return m_implementor->getOptimizationFile();
}

} // namespace xBacktest
