#include "Logger.h"
#include "Backtester.h"

namespace xBacktest
{

Backtester::Backtester(
    DataFeedConfig& dataFeedConfig,
    BrokerConfig&   brokerConfig,
    ReportConfig&   reportConfig,
    vector<StrategyConfig>& strategies)
    : m_dataFeedConfig(dataFeedConfig)
    , m_brokerConfig(brokerConfig)
    , m_reportConfig(reportConfig)
    , m_strategies(strategies)
{

}

void Backtester::init()
{
    m_barStorage = m_dataFeedConfig.getBarStorage();
}

Executor* Backtester::createNewExecutor(Utils::DefaultSemaphoreType& sema, const vector<StrategyConfig>& strategies)
{
    if (strategies.size() == 0) {
        return nullptr;
    }

    Executor* executor = new Executor();
    executor->setId(++m_nextExecutorId);
    executor->setBrokerConfig(m_brokerConfig);

    // Let executor calculate daily metrics.
    if (m_reportConfig.isReportEnable(ReportConfig::REPORT_DAILY_METRICS)) {
        executor->enableDailyMetricsReport();
        executor->setTradingDayEndTime(m_brokerConfig.getTradingDayEndTime());
    }

    // Initialize after setting cash done.
    executor->init();
    executor->registerDataStorage(m_barStorage);
    for (size_t i = 0; i < strategies.size(); i++) {
        executor->registerStrategy(strategies[i]);
    }
    executor->setSignalLamp(sema);

    return executor;
}

void Backtester::run()
{
    Logger_Info() << "Total strategies: " << m_strategies.size();

    Executor* executor = createNewExecutor(m_semaphore, m_strategies);
    executor->run();

    m_semaphore.wait();

    BacktestingMetrics metrics = { 0 };
    executor->calculatePerformanceMetrics(metrics);
    executor->printBacktestingReport(metrics);
    executor->saveBacktestingReport(m_reportConfig, metrics);

    executor->wait();

    delete executor;
}

} // namespace xBacktest
