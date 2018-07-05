#include <cstddef>
#include <cassert>
#include <sstream>
#include <fstream>
#include <numeric>
#include <algorithm>
#include <thread>
#include <unordered_set>
#include "Executor.h"
#include "DataStorage.h"
#include "Runtime.h"
#include "Logger.h"
#include "Errors.h"
#include "Utils.h"

namespace xBacktest
{

////////////////////////////////////////////////////////////////////////////////
Process::Process(Executor* executor,  const StrategyConfig& config)
    : m_executor(executor)
    , m_config(config)
{
    REQUIRE(config.getCreator() != nullptr, "Strategy creator is null.");

    m_name = config.getName();
    m_dataStreamIds.clear();
    m_runtimeList.clear();
    m_subscribeAll = false;
}

Process::~Process()
{
    for (size_t i = 0; i < m_runtimeList.size(); i++) {
        delete m_runtimeList[i];
    }
    m_runtimeList.clear();
}

const string& Process::getName() const
{
    return m_name;
}

void Process::registerDataStreamId(int id)
{
    m_dataStreamIds.insert(id);
}

const unordered_set<int>& Process::getDataStreamIds() const
{
    return m_dataStreamIds;
}

void Process::subscribeInstrument(const string& instrument)
{
    m_instruments.insert(instrument);
}

void Process::subscribeAll()
{
    m_subscribeAll = true;
}

const unordered_set<string>& Process::getInstruments() const
{
    return m_instruments;
}

void Process::registerContract(const Contract& contract)
{
    for (size_t i = 0; i < m_contracts.size(); i++) {
        if (m_contracts[i].instrument == contract.instrument) {
            m_contracts[i] = contract;
            return;
        }
    }

    m_contracts.push_back(contract);
}

void Process::processNewBar(int dataStreamId, int feedId, const Bar& bar)
{
    if (!m_subscribeAll && 
        (m_dataStreamIds.size() > 0 && 
         m_dataStreamIds.find(dataStreamId) == m_dataStreamIds.end())) {
        return;
    }

    size_t i;
    for (i = 0; i < m_runtimeList.size(); i++) {
        Runtime* runtime = m_runtimeList[i];
        assert(runtime != nullptr);
        if (runtime->isSubscribeAll() || 
            !_stricmp(runtime->getMainInstrument(), bar.getInstrument())) {
            runtime->onBarEvent(bar);
            return;
        }
    }

    // Create a new runtime for new instrument trading.
    if (i == m_runtimeList.size()) {
        Runtime* runtime = new Runtime(this);
        runtime->setId(getNextRuntimeId());
        runtime->setStrategyObject(m_config.getCreator()());
        runtime->registerContracts(m_contracts);

        if (m_subscribeAll) {
            runtime->subscribeAll();
        } else {
            runtime->setMainInstrument(bar.getInstrument());
            auto& instruments = m_config.getSubscribedInstruments();
            if (instruments.size() > 0) {
                for (auto& item : instruments) {
                    runtime->registerInstrument(item.instrument.c_str());
                }
            } else {
                // User don't assign the subscribed instrument.
                runtime->registerInstrument(bar.getInstrument());
            }
        }

        // Set permissible trading period.
        if (!m_subscribeAll) {
            vector<SessionItem>& table = m_executor->getSessionTable();
            for (size_t i = 0; i < table.size(); i++) {
                if (table[i].instrument == runtime->getMainInstrument()) {
                    runtime->addActiveDateTime(table[i].begin, table[i].end);
                }
            }
        } else {
            DateTime begin;
            DateTime end;
            m_executor->getSessionTimeRange(begin, end);
            runtime->addActiveDateTime(begin, end);
        }

        m_runtimeList.push_back(runtime);
        
        runtime->onCreate();
        runtime->setParameters(m_config.getParameters());
        runtime->onStart();
        runtime->onBarEvent(bar);
    }
}

void Process::processNewOrder(const OrderEvent& evt)
{
    for (size_t i = 0; i < m_runtimeList.size(); i++) {
        Runtime* runtime = m_runtimeList[i];
        if (runtime != nullptr) {
            if (runtime->onOrderEvent(&evt)) {
                return;
            }
        }
    }
}

void Process::processTimeElapsed(const DateTime& prevDateTime, const DateTime& nextDateTime)
{
    for (size_t i = 0; i < m_runtimeList.size(); i++) {
        Runtime* runtime = m_runtimeList[i];
        if (runtime != nullptr) {
            runtime->onTimeElapsed(prevDateTime, nextDateTime);
            return;
        }
    }
}

void Process::processHistoricalData(int dataStreamId, const Bar& bar)
{
    if (m_dataStreamIds.find(dataStreamId) == m_dataStreamIds.end()) {
        return;
    }

    for (size_t i = 0; i < m_runtimeList.size(); i++) {
        Runtime* rt = m_runtimeList[i];
        assert(rt != nullptr);
        if (!_stricmp(rt->getMainInstrument(), bar.getInstrument())) {
            rt->onHistoricalData(bar, false);
        }
    }
}

Executor* Process::getExecutor()
{
    return m_executor;
}

unsigned long Process::getNextOrderId()
{
    return m_executor->getNextOrderId();
}

unsigned long Process::getNextRuntimeId()
{
    return m_executor->getNextRuntimeId();
}

int Process::loadData(const DataRequest& request)
{
    return m_executor->loadData(request);
}

void Process::getTransactionRecords(vector<Transaction>& outRecords)
{
    for (size_t i = 0; i < m_runtimeList.size(); i++) {
        m_runtimeList[i]->getTransactionRecords(outRecords);
    }

    return;
}

void Process::stop()
{
    for (size_t i = 0; i < m_runtimeList.size(); i++) {
        m_runtimeList[i]->onStop();
    }
}

void Process::destroy()
{
    for (size_t i = 0; i < m_runtimeList.size(); i++) {
        m_runtimeList[i]->onDestroy();
    }
}

////////////////////////////////////////////////////////////////////////////////
Executor::Executor()
{
    m_dispatcher = new Dispatcher();
    m_dispatcher->getStartEvent().subscribe(this);
    m_dispatcher->getIdleEvent().subscribe(this);
    m_dispatcher->getTimeElapsedEvent().subscribe(this);

    m_backtestBroker = new BacktestingBroker();
    m_backtestBroker->init();
    m_backtestBroker->getOrderUpdatedEvent().subscribe(this);

    m_processList.clear();

    m_id           = 0;
    m_tag          = -1;
    m_cash         = 0.0;
    m_monitor      = nullptr;
    m_dataStorage  = nullptr;
    m_state        = Idle;

    m_historicalDataReqId = 0;
    m_nextOrderId         = 0;
    m_nextRuntimeId       = 0;
}

Executor::~Executor()
{
    // wait for thread completed.
    wait();

    delete m_dispatcher;
    m_dispatcher = nullptr;

    delete m_backtestBroker;
    m_backtestBroker = nullptr;
    
    for (size_t i = 0; i < m_processList.size(); i++) {
        delete m_processList[i];
    }
    m_processList.clear();

    for (auto& feed : m_clonedBarFeeds) {
        delete feed;
    }

    m_clonedBarFeeds.clear();
}

void Executor::init()
{
    m_backtestBroker->attachAnalyzerEx(m_returnsAnalyzerBase, "ReturnsAnalyzerBase");
    m_backtestBroker->attachAnalyzer(m_retAnalyzer);
    m_backtestBroker->attachAnalyzer(m_tradesAnalyzer);
    m_backtestBroker->attachAnalyzer(m_sharpeRatioAnalyzer);
    m_backtestBroker->attachAnalyzer(m_drawDownAnalyzer);
}

void Executor::setId(unsigned long id)
{
    m_id = id;
}

unsigned long Executor::getId() const
{
    return m_id;
}

void Executor::setTag(unsigned long tag)
{
    m_tag = tag;
}

unsigned long Executor::getTag() const
{
    return m_tag;
}

void Executor::setCash(double cash)
{
    assert(m_backtestBroker != nullptr);
    assert(cash >= 0);
    m_cash = cash;
    m_backtestBroker->setCash(cash);
}

void Executor::setBrokerConfig(const BrokerConfig& config)
{
    assert(m_backtestBroker != nullptr);

    m_cash = config.getCash();
    assert(m_cash >= 0);
    
    m_backtestBroker->initFillStrategy();
    m_backtestBroker->setCash(m_cash);
}

double Executor::getAvailableCash() const
{
    return m_backtestBroker->getAvailableCash();
}

double Executor::getEquity() const
{
    return m_backtestBroker->getEquity();
}

void Executor::setSignalLamp(Utils::DefaultSemaphoreType& sema)
{
    m_semaphore = &sema;
}

bool Executor::isStopped()
{
    bool ret = false;
    m_stateMutex.Lock();
    if (m_state == Stop) {
        ret = true;
    }
    m_stateMutex.Unlock();

    return ret;
}

void Executor::registerDataStorage(DataStorage* storage)
{
    assert(storage != nullptr);
    assert(m_dataStorage == nullptr);

    m_dataStorage = storage;

    vector<DataStream*> streams;
    m_dataStorage->getAllDataStream(streams);
    REQUIRE(streams.size() > 0, "Need at least one data stream!");

    m_clonedBarFeeds.clear();
    for (auto& stream : streams) {
        vector<BarFeed*> feeds;
        stream->cloneSharedBarFeed(feeds);
        for (auto& feed : feeds) {
            m_clonedBarFeeds.push_back(feed);
        }
    }

    registerBarFeeds(m_clonedBarFeeds);

    if (m_sessionTable.size() == 0) {
        createSessionTable();
    }
}

vector<SessionItem>& Executor::getSessionTable()
{
    return m_sessionTable;
}

void Executor::getSessionTimeRange(DateTime& begin, DateTime& end)
{
    begin = m_earliestDateTime;
    end = m_latestDataTime;
}

bool Executor::createSessionTable()
{
//    Logger_Info() << "Create session table using Default Rule...";

    if (m_clonedBarFeeds.size() == 0) {
        return false;
    }

    m_sessionTable.clear();

    for (size_t i = 0; i < m_clonedBarFeeds.size(); i++) {
        BarFeed* feed = m_clonedBarFeeds[i];
        SessionItem item;
        item.id         = feed->getId();
        item.instrument = feed->getInstrument();

        const vector<BarFeed::TradablePeriod>& periods = feed->getTradablePeriods();

        for (size_t j = 0; j < periods.size(); j++) {
            item.begin = periods[j].begin;
            item.end   = periods[j].end;

            // Session end at last bar by default.
            if (feed->getResolution() == Bar::MINUTE) {
                long long ticks = item.end.ticks();
                long long minutes = ticks / (60 * 1000);
                minutes -= feed->getInterval();
                item.end = DateTime(minutes * 60 * 1000);
            } else if (feed->getResolution() == Bar::SECOND) {
                long long ticks = item.end.ticks();
                long long secs = ticks / (1000);
                secs -= feed->getInterval();
                item.end = DateTime(secs * 1000);
            } else if (feed->getResolution() == Bar::DAY) {
                item.end = item.end.yesterday();
                
            } else {
                ASSERT(false, "Unsupported bar resolution.");
            }

            m_sessionTable.push_back(item);
        }
    }

    REQUIRE(m_sessionTable.size() > 0, "Session table is empty.");

    m_earliestDateTime = m_sessionTable[0].begin;
    m_latestDataTime = m_sessionTable[0].end;
    for (size_t i = 1; i < m_sessionTable.size(); i++) {
        if (m_earliestDateTime > m_sessionTable[i].begin) {
            m_earliestDateTime = m_sessionTable[i].begin;
        }

        if (m_latestDataTime < m_sessionTable[i].end) {
            m_latestDataTime = m_sessionTable[i].end;
        }
    }

    return true;
}

void Executor::registerBarFeeds(vector<BarFeed*>& feeds)
{
    for (size_t i = 0; i < feeds.size(); i++) {
        BarFeed* feed = feeds[i];
        if (feed != nullptr) {
            REQUIRE(feed->getDataStreamId() > 0, "Data stream id must be greater than 0!");
            REQUIRE(feed->getId() > 0,           "Bar feed id must be greater than 0!");

            m_backtestBroker->registerContract(feed->getContract());

            feed->getNewBarEvent().subscribe(this);
            m_dispatcher->addSubject(feed);
        }
    }
}

void Executor::registerStrategy(const StrategyConfig& config)
{
    InstrumentList instruments = config.getSubscribedInstruments();
    const unordered_set<string>& dataStreams = config.getSubscribedDataStreams();
    if ((!config.isSubscribeAll() && instruments.size() == 0) || dataStreams.size() == 0) {
        ASSERT(true, "Strategy has no instrument.");
    }

    Process* process = new Process(this, config);

    if (!config.isSubscribeAll()) {
        // Register data streams.
        for (auto& ds : dataStreams) {
            int id = m_dataStorage->getDataStreamId(ds);
            if (id <= 0) {
                char str[64];
                sprintf(str, "Data stream '%s' can not be found.", ds.c_str());
                ASSERT(true, str);
                delete process;
                return;
            }
            process->registerDataStreamId(id);
            vector<BarFeed*>& feeds = m_dataStorage->getDataStream(id)->getBarFeeds();
            for (auto& feed : feeds) {
                process->registerContract(feed->getContract());
            }
        }
        // Register instruments.
        for (auto& item : instruments) {
            process->subscribeInstrument(item.instrument);
            int id = m_dataStorage->getDataStreamId(item.instrument);
            ASSERT(id > 0, "Data stream can not be found.");
            process->registerDataStreamId(id);
            vector<BarFeed*>& feeds = m_dataStorage->getDataStream(id)->getBarFeeds();
            for (auto& feed : feeds) {
                process->registerContract(feed->getContract());
            }
        }
    } else {
        process->subscribeAll();
        for (size_t i = 0; i < m_clonedBarFeeds.size(); i++) {
            process->registerContract(m_clonedBarFeeds[i]->getContract());
        }
    }
    
    m_processList.push_back(process);
}

BacktestingBroker* Executor::getBroker() const
{
    return m_backtestBroker;
}

void Executor::dataCallBack(const DateTime& datetime, void* context)
{
    assert(context != nullptr);

    Executor* executor = (Executor*)context;

    BarFeed::BarEventCtx* ctx = (BarFeed::BarEventCtx*)context;
    int dataStreamId = ctx->dataStreamId;
    Bar& bar = ctx->bar;

    for (size_t i = 0; i < executor->m_processList.size(); i++) {
        executor->m_processList[i]->processHistoricalData(dataStreamId, bar);
    }
}

int Executor::loadData(const DataRequest& request)
{
    if (m_dataStorage == nullptr) {
        return 0;
    }

    BarFeed* feed = m_dataStorage->createSharedBarFeed(request.instrument, request.resolution);
    if (feed == nullptr) {
        return 0;
    }

    int ret = feed->loadData(++m_historicalDataReqId, request, this, dataCallBack);
    delete feed;

    return ret;
}

void Executor::writeDebugMsg(const char* msg)
{
    if (msg) {
        Logger_Info() << msg;
    }
}

void Executor::threadProc(void *const context)
{
    Executor *executor = (Executor*)context;
    assert(executor != nullptr);

    Logger_Info() << "Start executor [ID:" << executor->m_dispatcher->getId() << "] [ThreadID:" << std::this_thread::get_id() << "]...";

    executor->m_dispatcher->run();

    for (size_t i = 0; i < executor->m_processList.size(); i++) {
        executor->m_processList[i]->stop();
    }

    for (size_t i = 0; i < executor->m_processList.size(); i++) {
        executor->m_processList[i]->destroy();
    }

    executor->m_stateMutex.Lock();
    executor->m_state = Stop;
    executor->m_stateMutex.Unlock();

    if (executor->m_semaphore) {
        executor->m_semaphore->signal();
    }
}

void Executor::run()
{
    assert(m_dispatcher != nullptr);

    m_stateMutex.Lock();
    if (m_state == Running) {
        m_stateMutex.Unlock();
        return;
    }

    m_thread.Start(threadProc, this);

    m_state = Running;
    m_stateMutex.Unlock();
}

void Executor::wait()
{
    if (m_thread.Running()) {
        m_thread.Join();
    }
}

void Executor::enableDailyMetricsReport()
{
    m_backtestBroker->enableTradingDayNotification(true);
}

void Executor::disableDailyMetricsReport()
{
    m_backtestBroker->enableTradingDayNotification(false);
}

void Executor::setTradingDayEndTime(int timenum)
{
    return m_backtestBroker->setTradingDayEndTime(timenum);
}

void Executor::calculatePerformanceMetrics(BacktestingMetrics& metrics)
{
    vector<Returns::Ret>& cumReturns = (vector<Returns::Ret>&)m_retAnalyzer.getCumulativeReturns();
    if (cumReturns.size() > 0) {
        metrics.cumReturn = cumReturns[cumReturns.size() - 1].ret * 100;
    }

    metrics.initialCapital = m_cash;
    metrics.finalPortfolioValue = m_backtestBroker->getEquity();
    metrics.totalNetProfits = metrics.finalPortfolioValue - metrics.initialCapital;
//    metrics.totalNetProfits = m_tradesAnalyzer.getTotalNetProfits();
    metrics.maxDD = m_drawDownAnalyzer.getMaxDrawDown(false);
    metrics.retOnMaxDD = metrics.totalNetProfits / metrics.maxDD;

    double maxMargin = m_backtestBroker->getMaxMarginRequired();
    // This field calculates the amount of money you must have in your
    // account to trade the strategy.
    metrics.acctSizeRequired = max(maxMargin, metrics.maxDD);
    metrics.retOnAcctSizeRequired = metrics.totalNetProfits / metrics.acctSizeRequired;

    metrics.sharpeRatio         = m_sharpeRatioAnalyzer.getSharpeRatio(0.05);
    metrics.maxDDPercentage     = m_drawDownAnalyzer.getMaxDrawDown();
    metrics.maxDD               = m_drawDownAnalyzer.getMaxDrawDown(false);
    metrics.maxDDBegin          = m_drawDownAnalyzer.getMaxDrawDownBegin();
    metrics.maxDDEnd            = m_drawDownAnalyzer.getMaxDrawDownEnd();
    metrics.longestDDDuration   = (int)m_drawDownAnalyzer.getLongestDrawDownDuration();
    metrics.longestDDDBegin     = m_drawDownAnalyzer.getLongestDDBegin();
    metrics.longestDDDEnd       = m_drawDownAnalyzer.getLongestDDEnd();

    int year, month, day, hour, minute;
    metrics.tradingPeriod = m_backtestBroker->getTradingPeriod(year, month, day, hour, minute);
    metrics.totalTrades   = m_tradesAnalyzer.getCount();

    double years = (double)(metrics.tradingPeriod / 1000) / (365 * 24 * 60 * 60);
//    analysis.annualReturn = std::pow(1 + analysis.cumReturn / 100, 1 / years) - 1;
    metrics.annualReturn = metrics.cumReturn / years;

    double months = (double)(metrics.tradingPeriod / 1000) / (30 * 24 * 60 * 60);
    metrics.monthlyReturn = metrics.cumReturn / months;

    if (m_tradesAnalyzer.getCount() > 0) {
        const vector<Trades::TradeProfit>& allProfits = m_tradesAnalyzer.getAll();

        double sum = std::accumulate(allProfits.begin(), allProfits.end(), 0.0,
            [](double sum, const Trades::TradeProfit& curr) { return sum + curr.value; });

        const vector<Trades::TradeProfit>& profits = m_tradesAnalyzer.getProfits();
        const vector<double>& losses = m_tradesAnalyzer.getLosses();

        DrawDownCalculator calculator;
        calculator.reset();
        calculator.calculate(allProfits, m_cash);
        // Max. Close To Close Drawdown.
        metrics.maxCTCDD = calculator.getMaxDrawDown();

        metrics.winningTrades = profits.size();
        metrics.losingTrades = losses.size();
        metrics.percentProfitable = (double)metrics.winningTrades / metrics.totalTrades;
        metrics.avgProfit = sum / allProfits.size();

        const auto& minmaxProfit = std::minmax_element(allProfits.begin(), allProfits.end(), 
            [] (const Trades::TradeProfit& lhs, const Trades::TradeProfit& rhs) 
            {
                return lhs.value < rhs.value; 
            } );
        metrics.minProfit = minmaxProfit.first->value;
        metrics.maxProfit = minmaxProfit.second->value;

        metrics.grossProfit = std::accumulate(profits.begin(), profits.end(), 0.0,
            [](double sum, const Trades::TradeProfit& curr) { return sum + curr.value; });

        metrics.grossLoss = std::accumulate(losses.begin(), losses.end(), 0.0);
        metrics.avgWinningTrade = metrics.grossProfit / metrics.winningTrades;
        metrics.avgLosingTrade = metrics.grossLoss / metrics.losingTrades;
        metrics.ratioAvgWinAvgLoss = abs(metrics.avgWinningTrade / metrics.avgLosingTrade);
    }

    metrics.slippagePaid   = m_backtestBroker->getTotalSlippage();
    metrics.commissionPaid = m_backtestBroker->getTotalCommission();

}

void Executor::formatOutput(stringstream& ss, const BacktestingMetrics& metrics)
{    
    int year, month, day, hour, minute;
    m_backtestBroker->getTradingPeriod(year, month, day, hour, minute);

    ss << "===========================================================" << endl;
    ss << "             Initial Capital:  " << std::fixed << std::setprecision(2) << metrics.initialCapital << endl;
    ss << "       Final Portfolio Value:  " << metrics.finalPortfolioValue << endl;
    ss << "                  Net Profit:  " << metrics.totalNetProfits << endl;
    ss << "          Cumulative Returns:  " << std::fixed << std::setprecision(5) << metrics.cumReturn << " %" << endl;
    ss << "       Annual Rate of Return:  " << metrics.annualReturn << " %" << endl;
    ss << "      Monthly Rate of Return:  " << metrics.monthlyReturn << " %" << endl;
    ss << "     Return on Max. Drawdown:  " << metrics.retOnMaxDD << endl;
    ss << "       Account Size Required:  " << metrics.acctSizeRequired << endl;
    ss << "Return on Acct Size Required:  " << metrics.retOnAcctSizeRequired << endl;
    ss << "                Sharpe Ratio:  " << metrics.sharpeRatio << endl;
    ss << "           Max. Drawdown (%):  " << metrics.maxDDPercentage * 100 << " %" << endl;
    ss << "               Max. Drawdown:  " << metrics.maxDD << endl;
    ss << "         Max. Drawdown Begin:  " << metrics.maxDDBegin.toString() << endl;
    ss << "           Max. Drawdown End:  " << metrics.maxDDEnd.toString() << endl;
    ss << "Max. Close To Close Drawdown:  " << metrics.maxCTCDD * 100 << " %" << endl;
    ss << "   Longest Drawdown Duration:  " << metrics.longestDDDuration << " days" << endl;
    ss << "      Longest Drawdown Begin:  " << metrics.longestDDDBegin.toString() << endl;
    ss << "        Longest Drawdown End:  " << metrics.longestDDDEnd.toString() << endl;

    ss << "-----------------------------------------------------------" << endl;

    ss << "              Trading Period:  "
        << year << " Yrs, "
        << month << " Mths, "
        << day << " Dys, "
        << hour << " Hrs, "
        << minute << " Mins." << endl;

    ss << "                Total Trades:  " << metrics.totalTrades << endl;
    if (metrics.totalTrades > 0) {
        ss << "              Winning Trades:  " << metrics.winningTrades << endl;
        ss << "               Losing Trades:  " << metrics.losingTrades << endl;
        ss << "          Percent Profitable:  " << metrics.percentProfitable * 100 << "%" << endl;
        ss << "      Ratio AvgWin / AvgLoss:  " << metrics.ratioAvgWinAvgLoss << endl;
        ss << "                 Avg. Profit:  " << metrics.avgProfit << endl;
        ss << "                 Max. Profit:  " << metrics.maxProfit << endl;
        ss << "                 Min. Profit:  " << metrics.minProfit << endl;
        ss << "                Gross Profit:  " << metrics.grossProfit << endl;
        ss << "                  Gross Loss:  " << metrics.grossLoss << endl;
        ss << "       Average Winning Trade:  " << metrics.avgWinningTrade << endl;
        ss << "        Average Losing Trade:  " << metrics.avgLosingTrade << endl;
        ss << "               Slippage Paid:  " << metrics.slippagePaid << endl;
        ss << "             Commission Paid:  " << metrics.commissionPaid << endl;
    }
    ss << "===========================================================";
}

void Executor::printBacktestingReport(const BacktestingMetrics& metrics)
{
    if (m_processList.size() == 0) {
        return;
    }

    stringstream ss;
    formatOutput(ss, metrics);

    Logger_Info() << "Strategy Performance Summary:" << endl << ss.str();
}

void Executor::savePerformanceSummary(const string& file,  const BacktestingMetrics& metrics)
{
    Logger_Info() << "Write performance summary into file '" << file << "'.";
    ofstream out(file, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        return;
    }

    stringstream ss;
    formatOutput(ss, metrics);
    out << ss.str();

    out.close();
}

void Executor::saveReturnRecords(const string& file)
{
    Logger_Info() << "Write return records into file '" << file << "'.";

    ofstream out(file, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        return;
    }

    out << "DateTime, Return" << endl;
    vector<Returns::Ret>& cumReturns = (vector<Returns::Ret>&)m_retAnalyzer.getCumulativeReturns();
    int length = cumReturns.size();
    for (int i = 0; i < length; i++) {
        out << cumReturns[i].datetime.toString() << ", " << cumReturns[i].ret << endl;
    }

    out.close();
}

void Executor::saveEquityRecords(const string& file)
{
    Logger_Info() << "Write equity records into file '" << file << "'.";

    ofstream out(file, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        return;
    }

    out << "DateTime, Equity" << endl;
    vector<Returns::Equity>& equities = (vector<Returns::Equity>&)m_retAnalyzer.getEquities();
    int length = equities.size();
    for (int i = 0; i < length; i++) {
        out << equities[i].datetime.toString() << ", " << std::fixed << equities[i].equity << endl;
    }

    out.close();
}

void Executor::saveTradeRecords(const string& file)
{
    Logger_Info() << "Write transaction records into file '" << file << "'.";

    vector<TradingRecord> trades;
    m_tradesAnalyzer.getAllTradeRecords(trades);

    if (trades.size() == 0) {
        return;
    }

    ofstream out(file, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        return;
    }

    out << "DateTime,Type,Quantity,Price" << endl;
    for (size_t i = 0; i < trades.size(); i++) {
        string type = "";
        switch (trades[i].type) {
        case Position::Action::EntryLong:
            type = "EntryLong";
            break;
        case Position::Action::IncreaseLong:
            type = "IncreaseLong";
            break;
        case Position::Action::ReduceLong:
            type = "ReduceLong";
            break;
        case Position::Action::ExitLong:
            type = "ExitLong";
            break;
        case Position::Action::EntryShort:
            type = "EntryShort";
            break;
        case Position::Action::IncreaseShort:
            type = "IncreaseShort";
            break;
        case Position::Action::ReduceShort:
            type = "ReduceShort";
            break;
        case Position::Action::ExitShort:
            type = "ExitShort";
            break;
        default:
            break;
        }
        out << trades[i].datetime.toString() << "," << type << "," << trades[i].quantity << "," << trades[i].price << endl;
    }
 
    out.close();

    return;
}

void Executor::saveTransactionRecords(const string& file)
{
    Logger_Info() << "Write transaction records into file '" << file << "'.";

    vector<Transaction> history;
    for (size_t i = 0; i < m_processList.size(); i++) {
        m_processList[i]->getTransactionRecords(history);
    }

    if (history.size() > 0) {
        ofstream out(file, std::ios::out | std::ios::trunc);
        if (!out.is_open()) {
            return;
        }

        // Sort transaction records by exit datetime.
        std::sort(history.begin(), history.end(), [](const Transaction& a, const Transaction& b)
        {
            return (a.exitDateTime < b.exitDateTime ? true : a.entryDateTime < b.entryDateTime);
        });

        out << "Instrument,"
            << "EntryDate,EntryTime,EntryPrice,EntryType,"
            << "ExitDate,ExitTime,ExitPrice,ExitType,"
            << "Quantity,Commissions,Slippages,Realized PnL,Cumulative PnL"
            << ",Run-up,Drawdown,Duration" << endl;

        double cumPnL = 0;

        char datetime[64];
        for (size_t i = 0; i < history.size(); i++) {
            const Transaction& record = history[i];

            out << record.instrument << ",";

            memset(datetime, 0, sizeof(datetime));
#if 1
            strncpy(datetime, record.entryDateTime.toString().c_str(), sizeof(datetime)-1);
            char* p = strchr(datetime, ' ');
            if (p != nullptr) {
                *p++ = '\0';

                // remove millisecond
                char* q = strchr(p, '.');
                if (q != nullptr) {
                    *q = '\0';
                }

                out << datetime << "," << p << ",";
            }
#else
            sprintf(datetime, "%d,%06d", record.entryDateTime.dateNum(), record.entryDateTime.timeNum());
            out << datetime << ",";
#endif
            out << record.entryPrice << ",";

            string entryStr = " ";
            switch (record.entryType) {
            case SignalType::EntryLong:
                entryStr = "EntryLong";
                break;

            case SignalType::EntryShort:
                entryStr = "EntryShort";
                break;

            case SignalType::IncreaseLong:
                entryStr = "IncreaseLong";
                break;

            case SignalType::IncreaseShort:
                entryStr = "IncreaseShort";
                break;
            }
            out << entryStr << ",";

            if (record.exitDateTime.isValid()) {
                memset(datetime, 0, sizeof(datetime));
#if 1
                strncpy(datetime, record.exitDateTime.toString().c_str(), sizeof(datetime)-1);
                char* p = strchr(datetime, ' ');
                if (p != nullptr) {
                    *p++ = '\0';
                    
                    // remove millisecond
                    char* q = strchr(p, '.');
                    if (q != nullptr) {
                        *q = '\0';
                    }

                    out << datetime << "," << p << ",";
                }
#else
                sprintf(datetime, "%d,%06d", record.exitDateTime.dateNum(), record.exitDateTime.timeNum());
                out << datetime << ",";
#endif
                out << record.exitPrice << ",";
                string exitStr = " ";
                switch (record.exitType) {
                case SignalType::ExitLong:
                    exitStr = "ExitLong";
                    break;
                case SignalType::ExitShort:
                    exitStr = "ExitShort";
                    break;
                case SignalType::StopLoss:
                    exitStr = "StopLoss";
                    break;
                case SignalType::TakeProfit:
                    exitStr = "TakeProfit";
                    break;
                }
                out << exitStr << ",";

                /*
                unsigned long age = (record.exitDateTime - record.entryDateTime).ticks();
                char ageStr[16];
                sprintf(ageStr, "%.03f", (float)age / 1000);
                out << ageStr << ",";
                */

                out << record.shares << ",";
                out << record.commissions << ",";
                out << record.slippages << ",";
                out << std::fixed << record.realizedPnL << ",";

                cumPnL += record.realizedPnL;

                out << std::fixed << cumPnL;
            } else {
                out << " , ," << record.shares << ", ,0, ," << std::fixed << cumPnL;
            }

            out << "," << record.highestPrice << "," << record.lowestPrice << "," << record.duration << std::endl;
        }
        out.close();
    }
}

void Executor::saveDailyMetrics(const string& file)
{
    Logger_Info() << "Write daily metrics into file '" << file << "'.";

    const vector<DailyMetrics>& dailyMetrics = m_tradesAnalyzer.getAllDailyMetrics();
    if (dailyMetrics.size() == 0) {
        return;
    }

    ofstream out(file, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        return;
    }

    out << "Date,TradeNum,TradedVolume,TradeCost,RealizedProfit,TodayPos,Margin,PosProfit,CumRealizedProfit" << endl;
    for (size_t i = 0; i < dailyMetrics.size(); i++) {
        out << dailyMetrics[i].tradingDay.dateNum() << ",";
        out << dailyMetrics[i].tradesNum << ",";
        out << dailyMetrics[i].tradedVolume << ",";
        out << dailyMetrics[i].tradeCost << ",";
        out << dailyMetrics[i].closedProfit << ",";
        out << dailyMetrics[i].todayPosition << ",";
        out << dailyMetrics[i].margin << ",";
        out << dailyMetrics[i].posProfit << ",";
        out << std::fixed << dailyMetrics[i].cumRealizedProfit;
        out << endl;
    }

    out.close();
}

void Executor::saveBacktestingReport(const ReportConfig& config, const BacktestingMetrics& metrics)
{
    if (config.isReportEnable(ReportConfig::REPORT_SUMMARY) && 
        !config.getSummaryFile().empty()) {
        string filename = Utils::getFileBaseName(config.getSummaryFile());
        for (size_t i = 0; i < m_processList.size(); i++) {
            filename += "_";
            filename += m_processList[i]->getName();
        }
        savePerformanceSummary(filename + ".txt", metrics);
    }

    if (config.isReportEnable(ReportConfig::REPORT_DAILY_METRICS) &&
        !config.getDailyMetricsFile().empty()) {
        string filename = Utils::getFileBaseName(config.getDailyMetricsFile());
        for (size_t i = 0; i < m_processList.size(); i++) {
            filename += "_";
            filename += m_processList[i]->getName();
        }
        saveDailyMetrics(filename + ".csv");
    }

    if (config.isReportEnable(ReportConfig::REPORT_TRADES) &&
        !config.getTradeLogFile().empty()) {
        saveTradeRecords(config.getTradeLogFile());
    }

    if (config.isReportEnable(ReportConfig::REPORT_POSITION) && 
        !config.getPositionsFile().empty()) {
        string filename = Utils::getFileBaseName(config.getPositionsFile());
        for (size_t i = 0; i < m_processList.size(); i++) {
            filename += "_";
            filename += m_processList[i]->getName();
        }
        saveTransactionRecords(filename + ".csv");
    }

    if (config.isReportEnable(ReportConfig::REPORT_RETURNS) && 
        !config.getReturnsFile().empty()) {
        saveReturnRecords(config.getReturnsFile());
    }

    if (config.isReportEnable(ReportConfig::REPORT_EQUITIES) && 
        !config.getEquitiesFile().empty()) {
        saveEquityRecords(config.getEquitiesFile());
    }
}

SimplifiedMetrics Executor::getSimplifiedMetrics()
{
    vector<Returns::Ret>& cumReturns = (vector<Returns::Ret>&)m_retAnalyzer.getCumulativeReturns();
    SimplifiedMetrics metrics = { 0 };
    if (cumReturns.size() > 0) {
        metrics.cumReturns = cumReturns[cumReturns.size() - 1].ret * 100;
        metrics.sharpeRatio = m_sharpeRatioAnalyzer.getSharpeRatio(0.05);
        metrics.totalNetProfits = m_tradesAnalyzer.getTotalNetProfits();
        metrics.maxDrawDown = m_drawDownAnalyzer.getMaxDrawDown(false);
        metrics.retOnMaxDD = metrics.totalNetProfits / metrics.maxDrawDown;
    }

    return metrics;
}

void Executor::onNewBarEvent(int dataStreamId, int feedId, const Bar& bar)
{
    // It is VERY important that the broker get bars before the strategy.
    // This is to avoid executing orders placed in the current tick.
    m_backtestBroker->onBarEvent(bar);

    // Let strategies process this bar.
    for (size_t i = 0; i < m_processList.size(); i++) {
        m_processList[i]->processNewBar(dataStreamId, feedId, bar);
    }

    // Process on-the-spot(intra-bar) orders, those order must be handled
    // in current bar.
    m_backtestBroker->onPostBarEvent(bar);
}

void Executor::onNewOrderEvent(const OrderEvent& evt)
{
    for (size_t i = 0; i < m_processList.size(); i++) {
        m_processList[i]->processNewOrder(evt);
    }
}

void Executor::onTimeElapsedEvent(const DateTime& prevDateTime, const DateTime& nextDateTime)
{
    for (size_t i = 0; i < m_processList.size(); i++) {
        m_processList[i]->processTimeElapsed(prevDateTime, nextDateTime);
    }
}

void Executor::onEvent(int type, const DateTime& datetime, const void *context)
{
    switch (type) {
    case Event::EvtDispatcherStart:
        break;

    case Event::EvtDispatcherIdle:
        break;

    case Event::EvtDispatcherTimeElapsed: {
        const DateTime& prevDateTime = *((DateTime*)context);
        onTimeElapsedEvent(prevDateTime, datetime);
        break;
    }

    case Event::EvtNewBar: {
        BarFeed::BarEventCtx* ctx = (BarFeed::BarEventCtx*)context;
        int dataStreamId = ctx->dataStreamId;
        int feedId = ctx->barFeedId;
        Bar& bar = ctx->bar;
        onNewBarEvent(dataStreamId, feedId, bar);

        break;
    }

    case Event::EvtOrderUpdate: {
        OrderEvent& evt = *((OrderEvent *)context);
        onNewOrderEvent(evt);
        break;
    }

    default:
        break;
    }
}

unsigned long Executor::getNextOrderId()
{
    return ++m_nextOrderId;
}

unsigned long Executor::getNextRuntimeId()
{
    return ++m_nextRuntimeId;
}

} // namespace xBacktest