#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

#include <iostream>
#include "tinyxml2.h"
#include "Utils.h"
#include "Timer.h"
#include "Logger.h"
#include "Simulator.h"
#include "SimulatorImpl.h"
#include "DataStorage.h"
#include "Backtester.h"
#include "Optimizer.h"

namespace xBacktest
{

using namespace tinyxml2;

SimulatorImpl::SimulatorImpl()
{
    m_runningMode = Simulator::Backtest;

    m_maxOptimizingThreadNum = m_envConfig.getMachineCPUNum();

#ifdef _WIN32
    m_module     = nullptr;
#endif
    m_backtester = nullptr;
    m_optimizer  = nullptr;

    m_storage = new DataStorage();
    Logger_Info() << "+=======================================================================+";
    Logger_Info() << "* ALGORITHMIC STRATEGY BACKTESTING ENGINE [build: " __DATE__ " " __TIME__ "] *";
    Logger_Info() << "+=======================================================================+";
    Logger_Info() << "Engine initializing...";
    Logger_Info() << "Detected CPU number: " << m_maxOptimizingThreadNum;
}

SimulatorImpl::~SimulatorImpl()
{
    delete m_backtester;
    m_backtester = nullptr;

    delete m_optimizer;
    m_optimizer = nullptr;

    delete m_storage;
    m_storage = nullptr;

#ifdef _WIN32
    if (m_module != nullptr) {
        FreeLibrary(m_module);
        m_module = nullptr;
    }
#endif

    Logger_Info() << "Engine release.";
}

BrokerConfig& SimulatorImpl::getBrokerConfig()
{
    return m_brokerConfig;
}

EnvironmentConfig& SimulatorImpl::getEnvironmentConfig()
{
    return m_envConfig;
}

DataFeedConfig& SimulatorImpl::getDataFeedConfig()
{
    return m_dataFeedConfig;
}

StrategyConfig SimulatorImpl::createNewStrategyConfig()
{
    return StrategyConfig();
}

void SimulatorImpl::setRunningMode(int mode)
{
    m_runningMode = mode;
}

bool SimulatorImpl::loadSharedLibrary(StrategyConfig& config)
{
#if _WIN32
    string entry = config.getSharedLibrary();
    m_module = LoadLibrary(entry.c_str());
    if (m_module == NULL) {
        Logger_Err() << "Load library '" << entry << "' failed!";
        return false;
    }

    StrategyCreator* pfnCreateStrategy;
    pfnCreateStrategy = (StrategyCreator*)GetProcAddress(m_module, "CreateStrategy");
    if (pfnCreateStrategy == NULL) {
        FreeLibrary(m_module);
        m_module = NULL;
        Logger_Err() << "Can not find entry 'CreateStrategy' in '" << entry << "'.";
        return false;
    }

    config.setCreator(pfnCreateStrategy);
#endif
    return true;
}

bool SimulatorImpl::loadDataFeed()
{
    Logger_Info() << "Data storage loading...";

    if (m_dataFeedConfig.getStreams().size() == 0) {
        Logger_Info() << "Can not found any data stream!";
        return false;
    }

    m_dataFeedConfig.setBarStorage(m_storage);

    std::reverse(m_dataFeedConfig.getStreams().begin(), m_dataFeedConfig.getStreams().end());

    size_t size = m_dataFeedConfig.getStreams().size();
    for (size_t i = 0; i < size; i++) {
        if (!m_dataFeedConfig.getStreams()[i].name.empty()) {
            if (!m_storage->loadDataStreamFile(
                m_dataFeedConfig.getStreams()[i].name,
                m_dataFeedConfig.getStreams()[i].uri,
                m_dataFeedConfig.getStreams()[i].format,
                m_dataFeedConfig.getStreams()[i].resolution,
                m_dataFeedConfig.getStreams()[i].interval,
                m_dataFeedConfig.getStreams()[i].contract)) {
                Logger_Err() << "Data loading failed!";
                return false;
            }
        }
    }

    Logger_Info() << "Loading data feed done.";

    return true;
}

bool SimulatorImpl::parseScenario(const string& filename, StrategyConfig& config)
{
    Logger_Info() << "Parse scenario '" << filename << "' start...";

    tinyxml2::XMLDocument doc;
    int ret = doc.LoadFile(filename.c_str());
    if (ret != XML_NO_ERROR) {
        return false;
    }

    //tinyxml2::XMLHandle docHandle(&doc);

    tinyxml2::XMLElement* scenarioElem = doc.FirstChildElement("scenario");
    if (!scenarioElem) {
        return false;
    }

    // Parse environment
    tinyxml2::XMLElement* envElem = scenarioElem->FirstChildElement("environment");
    if (envElem) {
        if (envElem->FirstChildElement("core")) {
            const char* core_num = envElem->FirstChildElement("core")->Attribute("num");
            if (core_num != NULL) {
                int coreNum = atoi(core_num);
                m_envConfig.setMachineCPUNum(coreNum);
                if (coreNum == 0) {
                    m_maxOptimizingThreadNum = Utils::getMachineCPUNum();
                } else {
                    m_maxOptimizingThreadNum = coreNum;
                }
            }
        }

        if (envElem->FirstChildElement("optimizing")) {
            const char* mode = envElem->FirstChildElement("optimizing")->Attribute("mode");
            if (mode != NULL && _stricmp(mode, "Genetic") == 0) {
                m_envConfig.setOptimizationMode(Optimizer::Genetic);
            } else {
                m_envConfig.setOptimizationMode(Optimizer::Exhaustive);
            }
        } else {
            m_envConfig.setOptimizationMode(Optimizer::Exhaustive);
        }
    } else {
        m_envConfig.setOptimizationMode(Optimizer::Exhaustive);
    }

    // Parse broker
    tinyxml2::XMLElement* borkerElem = scenarioElem->FirstChildElement("broker");
    if (borkerElem) {
        if (borkerElem->FirstChildElement("cash")) {
            const char* cash_str = borkerElem->FirstChildElement("cash")->GetText();
            m_brokerConfig.setCash(atof(cash_str));
        } else {
            m_brokerConfig.setCash(DEFAULT_BROKER_CASH);
        }
    }

    // Parse data streams
    int dsCount = 0;
    tinyxml2::XMLElement* dataStreamElem = scenarioElem->FirstChildElement("datastreams");
    if (dataStreamElem) {
        tinyxml2::XMLElement* streamElem = dataStreamElem->FirstChildElement("stream");
        while (streamElem) {
            DataStreamConfig ds;
            ds.name = streamElem->Attribute("name");
            const char *res = streamElem->Attribute("resolution");
            if (_stricmp(res, "week") == 0) {
                ds.resolution = Bar::WEEK;
            } else if (_stricmp(res, "day") == 0) {
                ds.resolution = Bar::DAY;
            } else if (_stricmp(res, "hour") == 0) {
                ds.resolution = Bar::HOUR;
            } else if (_stricmp(res, "minute") == 0) {
                ds.resolution = Bar::MINUTE;
            } else if (_stricmp(res, "second") == 0) {
                ds.resolution = Bar::SECOND;
            } else if (_stricmp(res, "trade") == 0) {
                ds.resolution = Bar::TICK;
            } else {
                return false;
            }

            ds.uri = streamElem->Attribute("source");

            const char *realtime = streamElem->Attribute("realtime");
            if (realtime != nullptr && _stricmp(realtime, "true") == 0) {
                ds.realtime = true;
            } else {
                ds.realtime = false;
            }
            
            const char* formatStr = streamElem->Attribute("format");
            if (formatStr != nullptr && !_stricmp(formatStr, "csv")) {
                ds.format = DATA_FILE_FORMAT_CSV;
            } else {
                ds.format = DATA_FILE_FORMAT_BIN;
            }

            // Parse contract
            tinyxml2::XMLElement* contractElem = streamElem->FirstChildElement("contract")->ToElement();
            if (contractElem) {
                Contract contract;
                tinyxml2::XMLElement* elem;
                elem = contractElem->FirstChildElement("multiplier");
                if (elem) {
                    contract.multiplier = atof(elem->GetText());
                }

                elem = contractElem->FirstChildElement("ticksize");
                if (elem) {
                    contract.tickSize = atof(elem->GetText());
                }

                elem = contractElem->FirstChildElement("commission");
                if (elem) {
                    if (_stricmp(elem->Attribute("type"), "trade_percentage") == 0) {
                        contract.commType = CommissionType::TRADE_PERCENTAGE;
                    }
                    contract.commission = atof(elem->Attribute("value"));
                }

                elem = contractElem->FirstChildElement("slippage");
                if (elem) {
                    if (_stricmp(elem->Attribute("type"), "trade_percentage") == 0) {
                        contract.slippageType = CommissionType::TRADE_PERCENTAGE;
                    }
                    contract.slippage = atof(elem->Attribute("value"));
                }

                elem = contractElem->FirstChildElement("productid");
                if (elem) {
                    strncpy(contract.productId, elem->GetText(), sizeof(contract.productId) - 1);
                }

                ds.contract = contract;
            } else {
                Logger_Err() << "Data stream '" << ds.name << "' has no contract.";
                return false;
            }

            m_dataFeedConfig.registerStream(ds);
            dsCount++;

            streamElem = streamElem->NextSiblingElement();
        }

        if (dsCount == 0) {
            Logger_Err() << "There has no data stream in scenario file.";
            return false;
        }

        std::reverse(m_dataFeedConfig.getStreams().begin(), m_dataFeedConfig.getStreams().end());
    }

    // Parse strategy
    tinyxml2::XMLElement* stgyElem = scenarioElem->FirstChildElement("strategy");
    if (stgyElem) {
        tinyxml2::XMLElement* elem;
        elem = stgyElem->FirstChildElement("name");
        if (elem) {
            config.setName(elem->GetText());
        }

        elem = stgyElem->FirstChildElement("description");
        if (elem) {
            config.setDescription(elem->GetText());
        }

        elem = stgyElem->FirstChildElement("author");
        if (elem) {
            config.setAuthor(elem->GetText());
        }

        elem = stgyElem->FirstChildElement("entry");
        if (elem) {
            const char* entry = elem->GetText();
            if (entry == nullptr) {
                return false;
            }
            config.setSharedLibrary(entry);
        } else {
            return false;
        }

        bool err = true;
        elem = stgyElem->FirstChildElement("datastream");
        if (elem) {
            const char *s = elem->GetText();
            if (s) {
                config.subscribeDataStream(elem->GetText());
                err = false;
            }
        }

        if (err) {
            // Use default data stream.
            config.subscribeDataStream(m_dataFeedConfig.getStreams()[0].name);
        }

        // Parse parameters
        tinyxml2::XMLElement* parameters = stgyElem->FirstChildElement("parameters");
        if (parameters) {
            tinyxml2::XMLElement* paramElem = parameters->FirstChildElement();
            while (paramElem) {
                ParamItem item;
                item.name = paramElem->Attribute("name");
                string type = paramElem->Attribute("type");
                item.type = PARAM_TYPE_DOUBLE;
                if (_stricmp(type.c_str(), "int") == 0) {
                    item.type = PARAM_TYPE_INT;
                } else if (_stricmp(type.c_str(), "double") == 0) {
                    item.type = PARAM_TYPE_DOUBLE;
                } else if (_stricmp(type.c_str(), "string") == 0) {
                    item.type = PARAM_TYPE_STRING;
                } else if (_stricmp(type.c_str(), "bool") == 0) {
                    item.type = PARAM_TYPE_BOOL;
                }
                item.value = paramElem->Attribute("value");
                item.start = item.value;
                item.end = item.value;
                item.step = 1;

                tinyxml2::XMLElement* optimizingElem = paramElem->FirstChildElement("optimizing");
                if (optimizingElem) {
                    // If there has optimizing node, switch to optimization mode.
                    m_runningMode = Simulator::Optimization;
                    item.start = optimizingElem->Attribute("start");
                    item.end = optimizingElem->Attribute("end");
                    item.step = optimizingElem->Attribute("step");
                }

                config.registerParameter(item);

                paramElem = paramElem->NextSiblingElement();
            }
        }
    }

    // Parse report
    tinyxml2::XMLElement* reportElem = scenarioElem->FirstChildElement("report");
    if (reportElem) {
        tinyxml2::XMLElement* elem;
        elem = reportElem->FirstChildElement("summary");
        if (elem) {
            if (elem->Attribute("output")) {
                m_reportConfig.setSummaryFile(elem->Attribute("output"));
            }

        }

        elem = reportElem->FirstChildElement("trade");
        if (elem) {
            if (elem->Attribute("output")) {
                m_reportConfig.setTradeLogFile(elem->Attribute("output"));
            }
        }

        elem = reportElem->FirstChildElement("position");
        if (elem) {
            if (elem->Attribute("output")) {
                m_reportConfig.setPositionsFile(elem->Attribute("output"));
            }
        }

        elem = reportElem->FirstChildElement("return");
        if (elem) {
            if (elem->Attribute("output")) {
                m_reportConfig.setReturnsFile(elem->Attribute("output"));
            }
        }
    }

    Logger_Info() << "Parse scenario done.";

    return true;
}

bool SimulatorImpl::sanityCheck(const StrategyConfig& config)
{
    if (config.getName().empty()) {
        Logger_Err() << "Strategy name is empty.";
        return false;
    }

    if (config.getCreator() == nullptr) {
        Logger_Err() << "Strategy creator is null.";
        return false;
    }

    if (m_dataFeedConfig.getStreams().empty()) {
        return false;
    }

    if (m_brokerConfig.getCash() <= 0) {
        return false;
    }

    return true;
}

bool SimulatorImpl::initRunningMode(vector<StrategyConfig>& strategies)
{
    if (m_runningMode == Simulator::Optimization) {
        if (m_optimizer == nullptr) {
            m_optimizer = new Optimizer(m_dataFeedConfig, m_brokerConfig, strategies);
            
            m_maxOptimizingThreadNum = m_envConfig.getMachineCPUNum();
            if (m_maxOptimizingThreadNum <= 0) {
                m_maxOptimizingThreadNum = Utils::getMachineCPUNum();
            }
            m_optimizer->setMaxThreadNum(m_maxOptimizingThreadNum);
            
            int mode = m_envConfig.getOptimizationMode();
            if (mode != Optimizer::Exhaustive && mode != Optimizer::Genetic) {
                mode = Optimizer::Exhaustive;
            }
            m_optimizer->setOptimizationMode(mode);
            
            m_optimizer->init();
        } else {
            return false;
        }
    } else {
        if (m_backtester == nullptr) {
            m_backtester = new Backtester(
                m_dataFeedConfig,
                m_brokerConfig,
                m_reportConfig,
                strategies);
            m_backtester->init();
        } else {
            return false;
        }
    }

    for (size_t i = 0; i < strategies.size(); i++) {
        Logger_Info() << "Load strategy '" << strategies[i].getName() << "' done!";
    }

    return true;
}

bool SimulatorImpl::loadScenario(const string& filename)
{
    StrategyConfig conf = createNewStrategyConfig();

    if (!parseScenario(filename, conf)) {
        return false;
    }

    if (!loadSharedLibrary(conf)) {
        return false;
    }

    if (!sanityCheck(conf)) {
        Logger_Err() << "Sanity check failed!";
        return false;
    }

    return true;
}

bool SimulatorImpl::loadStrategy(StrategyConfig& config)
{
    for (size_t i = 0; i < m_strategyConfigs.size(); i++) {
        if (m_strategyConfigs[i].getName() == config.getName()) {
            Logger_Err() << "Strategy with the same name '" << config.getName() << "' already existed.";
            Logger_Err() << "If you are running one same strategy on different data streams simultaneously, "
                            "please append data stream's name as suffix to the strategy name.";
            return false;
        }
    }

    if (!sanityCheck(config)) {
        Logger_Err() << "Sanity check failed!";
        return false;
    }    
    
    m_strategyConfigs.push_back(config);

    return true;
}

bool SimulatorImpl::loadStrategy(StrategyCreator* creator)
{
    StrategyConfig config = createNewStrategyConfig();
    config.setCreator(creator);

    if (!sanityCheck(config)) {
        Logger_Err() << "Sanity check failed!";
        return false;
    }

    m_strategyConfigs.push_back(config);

    return true;
}

void SimulatorImpl::runBacktest()
{
    Logger_Info() << "Run backtesting...";

    assert(m_backtester != nullptr);
    m_backtester->run();

    Logger_Info() << "Backtesting completed.";
}

void SimulatorImpl::runOptimizing()
{
    Logger_Info() << "Run optimizing...";

    assert(m_optimizer != nullptr);
    m_optimizer->run();
    m_optimizer->printBestResult();
    m_optimizer->saveOptimizationReport(m_reportConfig.getOptimizationFile());
    
    Logger_Info() << "Optimizing completed.";
}

void SimulatorImpl::run()
{
    if (!loadDataFeed()) {
        return;
    }

    if (m_strategyConfigs.size() == 0) {
        return;
    }

    if (!initRunningMode(m_strategyConfigs)) {
        return;
    }

    Utils::Timer timer;
    timer.Start();

    if (m_runningMode == Simulator::Backtest) {
        runBacktest();
    } else if (m_runningMode == Simulator::Optimization) {
        runOptimizing();
    }

    timer.Stop();
    Logger_Info() << "Elapsed time: " << timer.Seconds() << " secs.";
}

void SimulatorImpl::writeLogMsg(int level, const char* msg) const
{
    Logger_Info() << msg;
}

int SimulatorImpl::version() const
{
    return VER_MAJOR_NUM * 100 + VER_MINOR_NUM;
}

// helper functions
void SimulatorImpl::setMachineCPUNum(int num)
{
    return m_envConfig.setMachineCPUNum(num);
}

void SimulatorImpl::setDataSeriesMaxLength(int length)
{
    if (length <= 0) {
        return;
    }

    DataSeries::DEFAULT_MAX_LEN = length;
}

bool SimulatorImpl::registerDataStream(const string& symbol, int resolution, int interval, const string& filename, int format)
{
    if (format != DATA_FILE_FORMAT_UNKNOWN) {
        m_dataFeedConfig.registerStream(symbol, resolution, interval, filename, format);
        return true;
    }

    string ext = Utils::getFileExtension(filename);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    int fmt = DATA_FILE_FORMAT_CSV;
    if (ext == "csv") {
        fmt = DATA_FILE_FORMAT_CSV;
    } else if (ext == "bin") {
        fmt = DATA_FILE_FORMAT_BIN;
    } else if (ext == "ts") {
        fmt = DATA_FILE_FORMAT_TS;
    } else {
        Logger_Info() << "Unknown data file extension.";
        return false;
    }

    m_dataFeedConfig.registerStream(symbol, resolution, interval, filename, fmt);

    return true;
}

void SimulatorImpl::setMultiplier(const string& symbol, double multiplier)
{
    return m_dataFeedConfig.setMultiplier(symbol, multiplier);
}

void SimulatorImpl::setTickSize(const string& symbol, double size)
{
    return m_dataFeedConfig.setTickSize(symbol, size);
}

void SimulatorImpl::setMarginRatio(const string& symbol, double ratio)
{
    return m_dataFeedConfig.setMarginRatio(symbol, ratio);
}

void SimulatorImpl::setCommissionType(const string& symbol, int type)
{
    return m_dataFeedConfig.setCommissionType(symbol, type);
}

void SimulatorImpl::setCommission(const string& symbol, double comm)
{
    return m_dataFeedConfig.setCommission(symbol, comm);
}

void SimulatorImpl::setSlippageType(const string& symbol, int type)
{
    return m_dataFeedConfig.setSlippageType(symbol, type);
}

void SimulatorImpl::setSlippage(const string& symbol, double slippage)
{
    return m_dataFeedConfig.setSlippage(symbol, slippage);
}

void SimulatorImpl::setCash(double cash)
{
    return m_brokerConfig.setCash(cash);
}

void SimulatorImpl::useTickDataToStopPos(bool useTickData)
{
    // TODO:
}

void SimulatorImpl::enableReport(int mask)
{
    return m_reportConfig.enableReport(mask);
}

void SimulatorImpl::disableReport(int mask)
{
    return m_reportConfig.disableReport(mask);
}

void SimulatorImpl::setPositionsReportFile(const string& filename)
{
    return m_reportConfig.setPositionsFile(filename);
}

} // namespace xBacktest
