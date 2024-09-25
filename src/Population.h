/*
 *  Population.h
 *  AsynchronousGA
 *
 *  Created by Bill Sellers on 19/10/2010.
 *  Copyright 2010 Bill Sellers. All rights reserved.
 *
 */

#ifndef POPULATION_H
#define POPULATION_H

#include "Genome.h"

#include <map>

class Random;

enum SelectionType
{
    UniformSelection,
    RankBasedSelection,
    SqrtBasedSelection,
    GammaBasedSelection
};

enum ResizeControl
{
    RandomiseResize,
    MutateResize
};

class Population
{
public:
    Population();

    Genome *GetFirstGenome() { return m_population.begin()->get(); }
    Genome *GetLastGenome() { return m_population.rbegin()->get(); }
    Genome *GetGenome(size_t i) { return m_population[i].get(); }
    size_t GetPopulationSize() { return m_population.size(); }

    void SetSelectionType(SelectionType type) { m_selectionType = type; }
    void SetParentsToKeep(size_t parentsToKeep) { m_parentsToKeep = parentsToKeep; if (m_parentsToKeep < 0) m_parentsToKeep = 0; }
    void SetGlobalCircularMutation(bool circularMutation);
    void SetResizeControl(ResizeControl control) { m_resizeControl = control; }
    void SetGamma(double gamma) { m_gamma = gamma; }
    void SetMinimizeScore(bool minimizeScore) { m_minimizeScore = minimizeScore; }

    Genome *ChooseParent(size_t *parentRank, Random *random);
    void Randomise(Random *random);
    int InsertGenome(std::unique_ptr<Genome> genome, size_t targetPopulationSize);
    void ResizePopulation(size_t size, Random *random);

    int ReadPopulation(const char *filename);
    int WritePopulation(const char *filename, size_t nBest);

protected:

    std::vector<std::unique_ptr<Genome>> m_population; // list of genomes sorted by fitness
    std::vector<double> m_immortalList; // sorted vector
    std::vector<double> m_ageList; // apparently a vector will be faster than a deque on a modern cpu
    SelectionType m_selectionType = RankBasedSelection;
    size_t m_parentsToKeep = 0;
    ResizeControl m_resizeControl = MutateResize;
    double m_gamma = 1.0;
    bool m_minimizeScore = false;
};


#endif // POPULATION_H
