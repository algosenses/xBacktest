#ifndef BACKTESTER_H
#define BACKTESTER_H

#include "Defines.h"
#include "Semaphore.h"
#include "Executor.h"

namespace xBacktest
{

class Backtester
{
public:
    Backtester(DataFeedConfig& dataFeedConfig,
               BrokerConfig&   brokerConfig,
               ReportConfig&   reportConfig,
               vector<StrategyConfig>& strategies);

    void registerStrategy(const StrategyConfig& config);
    void init();
    void run();

private:
    Executor* createNewExecutor(Utils::DefaultSemaphoreType& sema, const vector<StrategyConfig>& strategies);

private:
    Utils::DefaultSemaphoreType m_semaphore;
    DataStorage* m_barStorage;

    DataFeedConfig& m_dataFeedConfig;
    BrokerConfig&   m_brokerConfig;
    ReportConfig&   m_reportConfig;

    vector<StrategyConfig>  m_strategies;

    volatile unsigned long m_nextExecutorId;
};

} // namespace xBacktest

#endif // BACKTESTER_H
