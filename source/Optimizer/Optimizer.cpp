#include <iostream>
#include <sstream>
#include "Runtime.h"
#include "Optimizer.h"
#include "GeneticAlgo.h"
#include "Lock.h"
#include "Logger.h"

namespace xBacktest
{

Optimizer::Optimizer(DataFeedConfig& dataFeedConfig,
                     BrokerConfig&   brokerConfig,
                     vector<StrategyConfig>& strategies)
    : m_dataFeedConfig(dataFeedConfig)
    , m_brokerConfig(brokerConfig)
    , m_strategies(strategies)
{
    m_optimizationMode = Exhaustive;
    m_nextExecutorId = 0;
    m_barStorage = m_dataFeedConfig.getBarStorage();;
}

void Optimizer::setMaxThreadNum(int num)
{
    m_threadNum = num;
}

void Optimizer::setOptimizationMode(int mode)
{
    m_optimizationMode = mode;
}

void Optimizer::initParamSpace(ParamContext& paramCtx)
{
    paramCtx.spaceRowNum = 1;

    size_t size = paramCtx.paramTuple.size();
    if (size == 0) {
        paramCtx.spaceRowNum = 0;
        return;
    }

    vector<ParamRange> ranges;

    for (size_t i = 0; i < size; i++) {
        ParamTuple tuple;
        double start = atof(paramCtx.paramTuple[i].start.c_str());
        double end   = atof(paramCtx.paramTuple[i].end.c_str());
        double step  = atof(paramCtx.paramTuple[i].step.c_str());

        ParamRange range;
        range.start = start;
        range.end   = end;
        range.step  = step;
        if (step == 0) {
            range.count = 1;
        } else {
            range.count = (long)((end - start) / step + 1);
            if (range.count < 0) {
                range.count = -1 * range.count;
            }
        }
        ranges.push_back(range);

        paramCtx.spaceRowNum *= range.count;
    }

    int w = 1;
    vector<int> weights;
    for (unsigned int i = 0; i < ranges.size(); i++) {
        weights.push_back(w);
        w *= ranges[i].count;
    }

    // inverse weights table
    for (int i = weights.size() - 1; i >= 0; --i) {
        paramCtx.weightTable.push_back(weights[i]);
    }
}

ParamTuple Optimizer::getParamTuple(const ParamContext& paramCtx, long position)
{
    assert(position >= 0 && position < paramCtx.spaceRowNum);

    vector<int> groupIdxs;
    size_t i = 0;
    while (position >= 0) {
        groupIdxs.push_back(position / paramCtx.weightTable[i]);
        position -= (position / paramCtx.weightTable[i]) * paramCtx.weightTable[i];
        i++;
        if (i >= paramCtx.weightTable.size()) {
            assert(position == 0);
            break;
        }
    }

    ParamTuple tuple;
    int size = paramCtx.paramTuple.size();
    for (unsigned int i = 0; i < paramCtx.paramTuple.size(); i++) {
        ParamItem param;
        param.name = paramCtx.paramTuple[i].name;
        param.type = paramCtx.paramTuple[i].type;
        double value = atof(paramCtx.paramTuple[i].start.c_str()) + groupIdxs[size-i-1] * atof(paramCtx.paramTuple[i].step.c_str());
        std::ostringstream strs;
        strs << std::fixed << value;
//        strs << value;
        param.value = strs.str();

        tuple.push_back(param);
    }

    return tuple;
}

vector<ParamTuple> Optimizer::getParamTuples(long position)
{
    int size = m_strategies.size();
    vector<ParamTuple> tuples;

    vector<int> groupIdxs;
    size_t i = 0;
    while (position >= 0) {
        groupIdxs.push_back(position / m_allParamWeightTable[i]);
        position -= (position / m_allParamWeightTable[i]) * m_allParamWeightTable[i];
        i++;
        if (i >= m_allParamWeightTable.size()) {
            assert(position == 0);
            break;
        }
    }

    assert(groupIdxs.size() == m_allParamCtx.size());
    for (size_t i = 0; i < groupIdxs.size(); i++) {
        tuples.push_back(getParamTuple(m_allParamCtx[i], groupIdxs[i]));
    }

    return tuples;
}

void Optimizer::init()
{
    m_totalParamSpaceRowNum = 1;

    int w = 1;

    for (size_t i = 0; i < m_strategies.size(); i++) {
        ParamContext ctx;
        ctx.stratName  = m_strategies[i].getName();
        ctx.paramTuple = m_strategies[i].getParameters();
        ctx.spaceRowNum = 0;
        m_allParamCtx.push_back(ctx);
        initParamSpace(m_allParamCtx.back());
        m_totalParamSpaceRowNum *= m_allParamCtx.back().spaceRowNum;
        m_allParamWeightTable.push_back(w);
        w *= m_allParamCtx.back().spaceRowNum;
    }

    std::reverse(m_allParamWeightTable.begin(), m_allParamWeightTable.end());

    Logger_Info() << "Parameters Space Size: " << m_totalParamSpaceRowNum;
}

Executor* Optimizer::createNewExecutor(Utils::DefaultSemaphoreType& sema, const vector<StrategyConfig>& configs)
{
    Executor* executor = new Executor();
    executor->setId(++m_nextExecutorId);
    executor->setBrokerConfig(m_brokerConfig);
    executor->disableDailyMetricsReport();
    executor->init();
    executor->registerDataStorage(m_barStorage);
    for (size_t i = 0; i < configs.size(); i++) {
        executor->registerStrategy(configs[i]);
    }
    executor->setSignalLamp(sema);

    return executor;
}

void Optimizer::run()
{
    if (m_optimizationMode == Exhaustive) {
        runExhaustive();
    } else if (m_optimizationMode == Genetic) {
        runGenetic();
    }
}

void Optimizer::runBatch(vector<unsigned long>& inputs, vector<SimplifiedMetrics>& results)
{
    size_t num = inputs.size();
    if (num == 0) {
        return;
    }

    results.clear();
    results.resize(num);

    size_t inputParamNum = num;
    unsigned long id = 0;

    Utils::Mutex mutex;
    Utils::Lock lock(mutex);

    Utils::DefaultSemaphoreType sema;

    vector<Executor *> executorSlots(m_threadNum);
    for (unsigned int i = 0; i < m_threadNum; i++) {
        executorSlots[i] = nullptr;
    }

    unsigned int paramIdx = 0;
    for (unsigned int k = 0; k < m_threadNum && paramIdx < inputParamNum; k++) {
        if (executorSlots[k] == nullptr) {
            vector<StrategyConfig> strategies;
            vector<ParamTuple> tuples = getParamTuples(inputs[paramIdx]);
            assert(tuples.size() == m_strategies.size());

            for (size_t ii = 0; ii < m_strategies.size(); ii++) {
                StrategyConfig config(m_strategies[ii]);
                config.setParameters(tuples[ii]);
                strategies.push_back(config);
            }

            Executor* executor = createNewExecutor(sema, strategies);
            executor->setTag(paramIdx);
            executorSlots[k] = executor;
            executor->run();
            paramIdx++;
        }
    }

    int activeRTNum = 0;
    for (unsigned int i = 0; i < executorSlots.size(); i++) {
        if (executorSlots[i] != nullptr) {
            activeRTNum++;
        }
    }

    while (activeRTNum > 0) {
        sema.wait();
        for (unsigned int j = 0; j < executorSlots.size(); j++) {
            if (executorSlots[j] == nullptr || !executorSlots[j]->isStopped()) {
                continue;
            }

            results[executorSlots[j]->getTag()] = executorSlots[j]->getSimplifiedMetrics();

            executorSlots[j]->wait();

            delete executorSlots[j];
            executorSlots[j] = nullptr;

            if (paramIdx < inputParamNum) {
                vector<StrategyConfig> strategies;
                vector<ParamTuple> tuples = getParamTuples(inputs[paramIdx]);
                assert(tuples.size() == m_strategies.size());

                for (size_t ii = 0; ii < m_strategies.size(); ii++) {
                    StrategyConfig config(m_strategies[ii]);
                    config.setParameters(tuples[ii]);
                    strategies.push_back(config);
                }

                Executor* executor = createNewExecutor(sema, strategies);
                executor->setTag(paramIdx);
                executorSlots[j] = executor;
                executor->run();
                paramIdx++;
            } else {
                activeRTNum--;
            }
        }
    }
}

void Optimizer::runExhaustive()
{
    size_t paramTuplesNum = m_totalParamSpaceRowNum;
    unsigned long id = 0;

    Utils::DefaultSemaphoreType semaphore;

    vector<Executor *> execSlots(m_threadNum);
    for (unsigned int i = 0; i < m_threadNum; i++) {
        execSlots[i] = nullptr;
    }

    unsigned int paramIdx = 0;
    for (unsigned int k = 0; k < m_threadNum && paramIdx < paramTuplesNum; k++) {
        if (execSlots[k] == nullptr) {
            vector<StrategyConfig> strategies;
            vector<ParamTuple> tuples = getParamTuples(paramIdx);
            assert(tuples.size() == m_strategies.size());

            for (size_t ii = 0; ii < m_strategies.size(); ii++) {
                StrategyConfig config(m_strategies[ii]);
                config.setParameters(tuples[ii]);
                strategies.push_back(config);
            }
            Executor* executor = createNewExecutor(semaphore, strategies);
            executor->setTag(paramIdx);
            execSlots[k] = executor;
            executor->run();
            paramIdx++;
        }
    }

    int activeExecutorNum = 0;
    for (unsigned int i = 0; i < execSlots.size(); i++) {
        if (execSlots[i] != nullptr) {
            activeExecutorNum++;
        }
    }


    while (activeExecutorNum > 0) {

        semaphore.wait();

        for (unsigned int j = 0; j < execSlots.size(); j++) {
            if (execSlots[j] == nullptr || !execSlots[j]->isStopped()) {
                continue;
            }

            BacktestingMetrics metrics = { 0 };
            execSlots[j]->calculatePerformanceMetrics(metrics);
            m_metrics.insert(std::make_pair(execSlots[j]->getTag(), metrics));

            execSlots[j]->wait();

            delete execSlots[j];
            execSlots[j] = nullptr;

            if (paramIdx < paramTuplesNum) {
                vector<StrategyConfig> strategies;
                vector<ParamTuple> tuples = getParamTuples(paramIdx);
                assert(tuples.size() == m_strategies.size());

                for (size_t ii = 0; ii < m_strategies.size(); ii++) {
                    StrategyConfig config(m_strategies[ii]);
                    config.setParameters(tuples[ii]);
                    strategies.push_back(config);
                }
                Executor* executor = createNewExecutor(semaphore, strategies);
                executor->setTag(paramIdx);
                execSlots[j] = executor;
                executor->run();
                paramIdx++;
            } else {
                activeExecutorNum--;
            }
        }
    }
}

void Optimizer::runGenetic()
{
    m_population.init(
        DEFAULT_POPULATION_SIZE,
        m_totalParamSpaceRowNum,
        DEFAULT_CROSSOVER_PROBABILITY,
        DEFAULT_MUTATION_PROBABILITY,
        DEFUALT_MAX_GENERATION);

    m_population.setOptimizer(this);

    m_population.run();
}

void Optimizer::saveOptimizationReport(const string& file)
{
    Logger_Info() << "Write optimization report into file '" << file << "'.";
    ofstream out(file, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        return;
    }

    bool writeTitle = false;

    for (auto it = m_metrics.begin(); it != m_metrics.end(); it++) {
        unsigned long paramId = it->first;

        vector<ParamTuple> tuples = getParamTuples(paramId);

        if (!writeTitle) {
            out << "[PARAMETERS],";
            for (size_t k = 0; k < tuples.size(); k++) {
                ParamTuple& tuple = tuples[k];
                for (size_t i = 0; i < tuple.size(); i++) {
                    out << tuple[i].name << ",";
                }
            }
            out << "[METRICS],CumReturn,AnnualReturn,TotalNetProfits,MDD,MDDPercentage,MDDBegin,MDDEnd,RetOnMaxDD,Sharpe,TotalTrades,PercentProfitable,RatioAvgWinAvgLoss" << std::endl;
            writeTitle = true;
        }

        out << ",";
        for (size_t k = 0; k < tuples.size(); k++) {
            ParamTuple& tuple = tuples[k];
            for (size_t i = 0; i < tuple.size(); i++) {
                out << tuple[i].value << ",";
            }
        }

        char datetime[64] = { 0 };

        out << ",";
        out << std::fixed << std::setprecision(2);
        out << it->second.cumReturn << ",";
        out << it->second.annualReturn << ",";
        out << it->second.totalNetProfits << ",";
        out << it->second.maxDD << ",";
        out << it->second.maxDDPercentage << ",";

        strncpy(datetime, it->second.maxDDBegin.toString().c_str(), sizeof(datetime)-1);
        char* p = strchr(datetime, '.');
        if (p != nullptr) {
            *p++ = '\0';
        }
        out << datetime << ",";

        strncpy(datetime, it->second.maxDDEnd.toString().c_str(), sizeof(datetime)-1);
        p = strchr(datetime, '.');
        if (p != nullptr) {
            *p++ = '\0';
        }
        out << datetime << ",";

        out << it->second.retOnMaxDD << ",";
        out << it->second.sharpeRatio << ",";
        out << it->second.totalTrades << ",";
        out << it->second.percentProfitable << ",";
        out << it->second.ratioAvgWinAvgLoss;
        out << std::endl;
    }
}

void Optimizer::printBestResult()
{
    if (m_optimizationMode == Exhaustive) {
        if (m_metrics.size() == 0) {
            return;
        }

        map<unsigned long, BacktestingMetrics>::const_iterator it = m_metrics.begin();
        double returns = it->second.cumReturn;
        unsigned long paramId = it->first;
        while (it != m_metrics.end()) {
            if (it->second.cumReturn > returns) {
                returns = it->second.cumReturn;
                paramId = it->first;
            }
            it++;
        }

        Logger_Info() << "Exhaustive search algorithm: search " << m_totalParamSpaceRowNum << " combinations.";
        Logger_Info() << "Best Returns: " << returns << " %";
        vector<ParamTuple> tuples = getParamTuples(paramId);

        Logger_Info() << "Best parameters as the following:";
        for (size_t k = 0; k < tuples.size(); k++) {
            Logger_Info() << "-----------------------------------------";
            Logger_Info() << "'Strategy': '" << m_strategies[k].getName() << "'";
            ParamTuple& tuple = tuples[k];
            for (size_t i = 0; i < tuple.size(); i++) {
                Logger_Info() << "'" << tuple[i].name << "' : '" << tuple[i].value << "'";
            }
        }
        Logger_Info() << "-----------------------------------------";
    } else if (m_optimizationMode == Genetic) {
        Population::Elitist elitist = m_population.getElitist();
        Logger_Info() << "Genetic algorithm: evolved " << m_population.getAge() << " ages.";
        Logger_Info() << "Best Returns: " << elitist.score << " %";
        vector<ParamTuple> tuples = getParamTuples(elitist.chromosome);
        Logger_Info() << "Best parameters as the following:";
        for (size_t k = 0; k < tuples.size(); k++) {
            Logger_Info() << "-----------------------------------------";
            Logger_Info() << "'Strategy': '" << m_strategies[k].getName() << "'";
            ParamTuple& tuple = tuples[k];
            for (size_t i = 0; i < tuple.size(); i++) {
                Logger_Info() << "'" << tuple[i].name << "' : '" << tuple[i].value << "'";
            }
        }
        Logger_Info() << "-----------------------------------------";
    }
}

} // namespace xBacktest