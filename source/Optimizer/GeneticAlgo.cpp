#include <sstream>
#include "Errors.h"
#include "GeneticAlgo.h"
#include "Optimizer.h"

namespace xBacktest
{

Population::Population()
{

}

Population::~Population()
{

}

void Population::init(int size, int spaceSize, double cp, double mp, int maxGen)
{
    m_size = size;
    m_searchSpaceSize = spaceSize;
    m_crossoverProbability = cp;
    m_mutationProbability = mp;
    m_maxGeneration = maxGen;

    m_age = 0;
    m_individuals.clear();
    m_fitness.clear();
    m_selectorProbability.clear();
    m_newIndividuals.clear();
    m_chromosomeLength = calcChromLength(m_searchSpaceSize);
    m_elitist.chromosome = 0;
    m_elitist.fitness = 0;
    m_elitist.age = 0;

    m_stagnationAges = 0;
    m_bestScore = 0;

    for (int i = 0; i < m_size; i++) {
        int random = m_randomIntegerGenerator.Generate(m_searchSpaceSize - 1);
        m_individuals.push_back(random);
        m_newIndividuals.push_back(0);
        m_fitness.push_back(0);
        m_selectorProbability.push_back(0);
    }

    m_optimizer = NULL;
}

void Population::setOptimizer(Optimizer* optimizer)
{
    m_optimizer = optimizer;
}

int Population::select()
{
    double t = m_randomDoubleGenerator.Generate(1);
    size_t i = 0;
    for (; i < m_selectorProbability.size(); i++) {
        if (m_selectorProbability[i] > t) {
            break;
        }
    }

    return i;
}

void Population::cross(unsigned long& chrom1, unsigned long& chrom2)
{
    double p = m_randomDoubleGenerator.Generate();

    unsigned long mask = ~0;

    if (chrom1 != chrom2 && p < m_crossoverProbability) {
        bool retry = true;

        while (retry) {
            int t = m_randomIntegerGenerator.Generate(1, m_chromosomeLength - 1);
            unsigned long h1 = chrom1 & (mask << t);
            unsigned long h2 = chrom2 & (mask << t);
            unsigned long l1 = chrom1 & (~(mask << t));
            unsigned long l2 = chrom2 & (~(mask << t));

            unsigned long temp1 = h1 | l2;
            unsigned long temp2 = h2 | l1;

            if (temp1 < m_searchSpaceSize && temp2 < m_searchSpaceSize) {
                retry = false;
                chrom1 = temp1;
                chrom2 = temp2;
            }
        }

        assert(chrom1 < m_searchSpaceSize && chrom2 < m_searchSpaceSize);
    }
}

void Population::mutate(unsigned long& chrom)
{
    double p = m_randomDoubleGenerator.Generate();
    if (p < m_mutationProbability) {
        bool retry = true;
        while (retry) {
            int t = m_randomIntegerGenerator.Generate(1, m_chromosomeLength);
            unsigned long mask = 0x1 << (t - 1);
            unsigned long temp = chrom ^ mask;

            if (temp < m_searchSpaceSize) {
                retry = false;
                chrom = temp;
            }
        }

        assert(chrom < m_searchSpaceSize);
    }
}

// Calculate score base weight ratio
double Population::score(const SimplifiedMetrics& result)
{
    return 1 * result.cumReturns + 0 * result.maxDrawDown + 0 * result.sharpeRatio;
}

int Population::calcChromLength(int searchSpaceSize)
{
    int length = 0;
    while (searchSpaceSize != 0) {
        length++;
        searchSpaceSize /= 2;
    }

    return length;
}

void Population::reproduceElitist()
{
    int j = -1;
    for (int i = 0; i < m_size; i++) {
        if (m_elitist.score < m_scores[i]) {
            m_elitist.score = m_scores[i];
            j = i;
        }
    }

    if (j != -1) {
        m_elitist.chromosome = m_individuals[j];
        m_elitist.fitness = m_fitness[j];
        m_elitist.age = m_age;
    }
}

void Population::evaluate()
{
    // calculate fitness
    vector<SimplifiedMetrics> results;
    m_optimizer->runBatch(m_individuals, results);
    
    ASSERT(m_individuals.size() == results.size(), "Evaluating population error.");

    double score_min = score(results[0]);
    double score_max = score_min;
    m_scores.clear();

    for (size_t i = 0; i < m_individuals.size(); i++) {
        double s = score(results[i]);
        if (s > score_max) {
            score_max = s;
        }

        if (s < score_min) {
            score_min = s;
        }

        m_scores.push_back(s);
    }

    double ft_sum = 0;
    m_fitness.clear();
    for (size_t i = 0; i < m_individuals.size(); i++) {
        double f = (score(results[i]) - score_min) / (score_max - score_min);
        m_fitness.push_back(f);
        ft_sum += f;
    }

    for (size_t i = 0; i < m_individuals.size(); i++) {
        m_selectorProbability[i] = m_fitness[i] / ft_sum;
    }

    for (size_t i = 1; i < m_individuals.size(); i++) {
        m_selectorProbability[i] += m_selectorProbability[i - 1];
    }
}

void Population::evolve()
{
    evaluate();

    int i = 0;
    while (true) {
        int idv1 = select();
        int idv2 = select();

        unsigned long chrom1 = m_individuals[idv1];
        unsigned long chrom2 = m_individuals[idv2];

        cross(chrom1, chrom2);

        mutate(chrom1);
        mutate(chrom2);

        m_newIndividuals[i]     = chrom1;
        m_newIndividuals[i + 1] = chrom2;

        i += 2;
        if (i >= m_size) {
            break;
        }
    }

    reproduceElitist();

    for (int i = 0; i < m_size; i++) {
        m_individuals[i] = m_newIndividuals[i];
    }
}

void Population::run()
{
    int i = 0;
    while (i < m_maxGeneration) {
        m_age = i;

        evolve();

        if (m_age == 0) {
            m_bestScore = m_elitist.score;
        } else {
            if (fabs(m_elitist.score - m_bestScore) < 0.000001) {
                m_stagnationAges++;
            } else {
                m_stagnationAges = 0;
                m_bestScore = m_elitist.score;
            }
        }

        if (m_stagnationAges >= DEFAULT_STAGNATION_AGES && m_age * DEFAULT_POPULATION_SIZE >= m_searchSpaceSize / 2) {
            break;
        }

        i++;
    }
}

const Population::Elitist& Population::getElitist() const
{
    return m_elitist;
}

int Population::getAge() const
{
    return m_age;
}

} // namespace xBacktest
