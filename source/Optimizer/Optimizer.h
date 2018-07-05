#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include <string>
#include <vector>
#include <map>
#include "Defines.h"
#include "Simulator.h"
#include "GeneticAlgo.h"
#include "Strategy.h"
#include "Executor.h"
#include "Condition.h"

namespace xBacktest
{

using std::string;
using std::vector;
using std::map;

class Optimizer
{
public:
    enum OptimizationMode {
        Exhaustive,
        Genetic,
    };

    Optimizer(DataFeedConfig& dataFeedConfig,
              BrokerConfig&   brokerConfig,
              vector<StrategyConfig>& strategies);

    void setMaxThreadNum(int num);
    void setOptimizationMode(int mode);
    void init();
    void saveOptimizationReport(const string& file);
    void printBestResult();
    void run();

private:
    friend class Population;

    typedef struct {
        string      stratName;
        ParamTuple  paramTuple;
        vector<int> weightTable;
        unsigned long spaceRowNum;
    } ParamContext;

    void initParamSpace(ParamContext& paramCtx);
    // Specify the position, return parameter tuple in the space of parameter combinations.
    // Instead of pre-generate all possible combinations, we calculate parameter on-demand.
    ParamTuple getParamTuple(const ParamContext& paramCtx, long position);
    vector<ParamTuple> getParamTuples(long position);

    Executor* createNewExecutor(Utils::DefaultSemaphoreType& sema, const vector<StrategyConfig>& configs);

    // Calculation of a batch of input parameters and return the results.
    void runBatch(vector<unsigned long>& inputs, vector<SimplifiedMetrics>& results);
    
    // Calculation of all input parameters in the specified limits with the specified step. 
    // This optimization mode requires a considerable amount of time, however, none of the 
    // parameter combinations will be missed.
    void runExhaustive();

    // Searches for the best input parameters of a genetic algorithm that helps to find the 
    // best inputs for the minimum amount of time. Also chooses the best range and steps for
    // signal inputs calculations.
    void runGenetic();

private:
    int m_optimizationMode;

    unsigned int m_threadNum;
    Utils::Condition m_monitor;

    std::vector<ParamContext> m_allParamCtx;
    unsigned long long m_totalParamSpaceRowNum;
    vector<int> m_allParamWeightTable;

    Population m_population;

    DataFeedConfig& m_dataFeedConfig;
    BrokerConfig&   m_brokerConfig;
    vector<StrategyConfig> m_strategies;

    DataStorage* m_barStorage;
    StrategyCreator* m_strategyCreator;
    volatile unsigned long m_nextExecutorId;
    map<unsigned long, BacktestingMetrics> m_metrics;
};

} // namespace xBacktest

#endif // OPTIMIZER_H
