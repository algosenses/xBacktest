#ifndef GENETIC_ALGO_H
#define GENETIC_ALGO_H

#include <vector>
#include <string>
#include "Random.h"
#include "Simulator.h"

using std::vector;
using std::string;

namespace xBacktest
{

#define DEFAULT_CROSSOVER_PROBABILITY       (0.8)
#define DEFAULT_MUTATION_PROBABILITY        (0.05)
#define DEFAULT_POPULATION_SIZE             (50)
#define DEFUALT_MAX_GENERATION              (10000)
#define DEFAULT_STAGNATION_AGES             (10)

class Optimizer;

class Population
{
public:
    typedef struct {
        unsigned long chromosome;
        double fitness;
        double score;
        int age;
    } Elitist;

    Population();
    ~Population();    
    
    void init(int size, int spaceSize, double cp, double mp, int maxGen);
    void setOptimizer(Optimizer* optimizer);
    void run();
    const Elitist& getElitist() const;
    int getAge() const;

private:
    double score(const SimplifiedMetrics& result);
    int calcChromLength(int searchSpaceSize);
    void evaluate();
    int select();
    void cross(unsigned long& chrom1, unsigned long& chrom2);
    void mutate(unsigned long& chrom);
    void reproduceElitist();
    void evolve();

private:
    int m_size;
    int m_searchSpaceSize;
    int m_chromosomeLength;
    double m_crossoverProbability;
    double m_mutationProbability;
    int m_maxGeneration;
    int m_age;
    int m_stagnationAges;
    double m_bestScore;

    vector<unsigned long> m_individuals;
    vector<double> m_fitness;
    vector<double> m_scores;
    vector<double> m_selectorProbability;
    vector<unsigned long> m_newIndividuals;
    Elitist m_elitist;

    Utils::RandomInteger m_randomIntegerGenerator;
    Utils::RandomDouble m_randomDoubleGenerator;

    Optimizer* m_optimizer;
};

} // namespace xBacktest

#endif // GENETIC_ALGO_H