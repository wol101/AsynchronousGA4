/*
 *  Population.cpp
 *  AsynchronousGA
 *
 *  Created by Bill Sellers on 19/10/2010.
 *  Copyright 2010 Bill Sellers. All rights reserved.
 *
 */

#include <iostream>
#include <fstream>
#include <memory.h>
#include <map>
#include <cmath>
#include <limits>
#include <algorithm>

#include "Genome.h"
#include "Population.h"
#include "Random.h"
#include "Preferences.h"

// constructor
Population::Population()
{
}


// initialise the population
void Population::InitialisePopulation(size_t populationSize, const Genome &genome)
{
    m_population.clear();
    m_populationIndex.resize(populationSize);
    for (size_t i = 0; i < populationSize; i++)
    {
        m_population[double(i)] = genome;
        m_populationIndex[i] = double(i);
    }
}

// choose a parent from a population
Genome *Population::ChooseParent(size_t *parentRank, Random *random)
{
    // this type biases random choice to higher ranked individuals using the gamma function
    // this assumes a sorted genome
    if (m_selectionType == GammaBasedSelection)
    {
        *parentRank = random->GammaBiasedRandomInt(0, m_population.size() - 1, m_gamma);
        return &m_population[m_populationIndex[*parentRank]];
    }

    // in this version we do uniform selection and just choose a parent
    // at random
    if (m_selectionType == UniformSelection)
    {
        *parentRank = random->RandomInt(0, m_population.size() - 1);
        return &m_population[m_populationIndex[*parentRank]];
    }

    // this type biases random choice to higher ranked individuals
    // this assumes a sorted genome
    if (m_selectionType == RankBasedSelection)
    {
        *parentRank = random->RankBiasedRandomInt(1, m_population.size()) - 1;
        return &m_population[m_populationIndex[*parentRank]];
    }

    // this type biases random choice to higher ranked individuals
    // this assumes a sorted genome
    if (m_selectionType == SqrtBasedSelection)
    {
        *parentRank = random->SqrtBiasedRandomInt(0, m_population.size() - 1);
        return &m_population[m_populationIndex[*parentRank]];
    }

    // should never get here
    std::cerr << "Logic error Population::ChooseParent " << __LINE__ << "\n";
    return 0;
}

// insert a genome into the population
// the population is always kept sorted by fitness
// the oldest genome is deleted unless it is in the
// immortal list
// the key is the numeric value of the fitness so there are rare cases when
// different genomes with the same fitness will not be accepted
int Population::InsertGenome(Genome &&genome, size_t targetPopulationSize)
{
    size_t originalSize = m_population.size();
    if (targetPopulationSize == 0) targetPopulationSize = originalSize; // not trying to change the size of the population

    double fitness = genome.GetFitness();
    if (m_minimizeScore) { fitness = -fitness; } // now all the sorting will hapen in reverse
    auto findGenome = m_population.find(fitness);
    if (findGenome != m_population.end())
    {
//#define DEBUG_POPULATION
#ifdef DEBUG_POPULATION
        std::cerr << "InsertGenome fitness = " << fitness << " already in population\n";
#endif
        return __LINE__; // nothing inserted because the fitness already exists
    }
    m_population[fitness] = genome;
#ifdef DEBUG_POPULATION
    std::cerr << "m_population.size(()=" << m_population.size() << " genome.GetFitness()=" << genome.GetFitness() << " fitness=" << fitness << "\n";
#endif

    // ok now insert into the other internal lists
    m_populationIndex.insert(std::upper_bound(m_populationIndex.begin(), m_populationIndex.end(), fitness), fitness); // insert at upper bound to minimise shifting

    if (m_parentsToKeep == 0)
    {
        m_ageList.push_back(fitness); // just add current genome to list by age
#ifdef DEBUG_POPULATION
        std::cerr << "InsertGenome adding to m_AgeList - no test; size = " << m_ageList.size() << "\n";
#endif
    }
    else
    {
        // need to worry about immortality
        if (m_immortalListIndex.size() < m_parentsToKeep)
        {
            m_immortalListIndex.insert(std::upper_bound(m_immortalListIndex.begin(), m_immortalListIndex.end(), fitness), fitness);
#ifdef DEBUG_POPULATION
            std::cerr << "InsertGenome adding to m_immortalListIndex - no test; size = " << m_immortalListIndex.size() << "\n";
#endif
        }
        else
        {
            if (fitness > m_immortalListIndex[0])
            {
                m_immortalListIndex.insert(std::upper_bound(m_immortalListIndex.begin(), m_immortalListIndex.end(), fitness), fitness);
                m_ageList.push_back(m_immortalListIndex.front());
                m_immortalListIndex.erase(m_immortalListIndex.begin());
#ifdef DEBUG_POPULATION
                std::cerr << "InsertGenome m_immortalListIndex to m_ageList bump; m_ageList.size() = " << m_ageList.size() << " m_immortalListIndex.size() = " << m_immortalListIndex.size() << "\n";
#endif
            }
            else
            {
                m_ageList.push_back(fitness);
#ifdef DEBUG_POPULATION
                std::cerr << "InsertGenome adding to m_ageList after test; size = " << m_ageList.size() << "\n";
#endif
            }
        }
    }

    // check population sizes
    while (m_population.size() > targetPopulationSize)
    {
        if (m_ageList.size() == 0)
        {
            std::cerr << "Logic error Population::InsertGenome m_ageList has zero size " << __LINE__ << "\n";
            m_population.erase(*m_populationIndex.begin());
            m_populationIndex.erase(m_populationIndex.begin());
            continue;
        }
        double genomeToDelete = *m_ageList.begin();
        m_ageList.erase(m_ageList.begin()); // equivalent to pop_front()
        auto genomeIt = m_population.find(genomeToDelete);
        if (genomeIt != m_population.end())
        {
            m_population.erase(genomeIt);
        }
        else
        {
            std::cerr << "Logic error Population::InsertGenome genome not found in m_Population " << __LINE__ << "\n";
            m_population.erase(m_population.begin());
            m_populationIndex.erase(m_populationIndex.begin());
            continue;
        }
        auto fitnessIt = std::lower_bound(m_populationIndex.begin(), m_populationIndex.end(), genomeToDelete); // delete at lower bound because this value matches (whereas upper bound does not)
        if (fitnessIt != m_populationIndex.end() && *fitnessIt == genomeToDelete)
        {
            m_populationIndex.erase(fitnessIt);
        }
        else
        {
            std::cerr << "Logic error Population::InsertGenome genome not found in m_PopulationIndex " << __LINE__ << "\n";
            m_populationIndex.clear(); // need to rebuild the index by getting all the indexes and sorting (quicker than inserting at a sorted location each time)
            for (auto &&it : m_population) { m_populationIndex.push_back(it.first); }
            std::sort(m_populationIndex.begin(), m_populationIndex.end());
        }
    }
    return 0;
}

// randomise the population
void Population::Randomise(Random *random)
{
    for (auto iter = m_population.begin(); iter != m_population.end(); iter++)
        iter->second.Randomise(random);
}

// reset the population size to a new value - needs at least one valid genome in population
void Population::ResizePopulation(size_t size, Random *random)
{
    if (m_population.size() == size) return;
    if (size > m_population.size())
    {
        switch (m_resizeControl)
        {
        case RandomiseResize:
            // fill in with random genomes
            for (size_t i = m_population.size(); i < size; i++)
            {
                Genome g = m_population.begin()->second;
                g.Randomise(random);
                g.SetFitness(std::nextafter(m_populationIndex.back(), std::numeric_limits<double>::max()));
                double fitness = g.GetFitness();
                if (m_minimizeScore) { fitness = -fitness; } // now all the sorting will hapen in reverse
                m_populationIndex.push_back(fitness);
                m_population[fitness] = std::move(g);
            }
            break;

        case MutateResize:
            // fill in with mutated genomes
            for (size_t i = m_population.size(); i < size; i++)
            {
                Genome g = m_population.begin()->second;
                Mating mating(random);
                while (mating.GaussianMutate(&g, 1.0, true) == 0);
                double fitness = g.GetFitness();
                if (m_minimizeScore) { fitness = -fitness; } // now all the sorting will hapen in reverse
                m_populationIndex.push_back(fitness);
                m_population[fitness] = std::move(g);
            }
            break;

        }
    }
    else
    {
        size_t delta = m_population.size() - size;
        for (size_t i = 0; i < delta; i++)
        {
            double fitness = m_populationIndex.front();
            m_populationIndex.erase(m_populationIndex.begin());
            m_population.erase(fitness);
        }
    }
}

// set the circular flags for the genomes in the population
void Population::SetGlobalCircularMutation(bool circularMutation)
{
    for (auto iter = m_population.begin(); iter != m_population.end(); ++iter)
        iter->second.SetGlobalCircularMutationFlag(circularMutation);
}

// output a subpopulation as a new population
// note: outputs population with fittest first
int Population::WritePopulation(const char *filename, size_t nBest)
{
    if (nBest > m_population.size()) nBest = m_population.size();

    try
    {
        std::ofstream outFile;
        outFile.exceptions (std::ios::failbit|std::ios::badbit);
        outFile.open(filename);
        outFile << nBest << "\n";
        size_t count = 0;
        for (auto iter = m_population.rbegin(); iter != m_population.rend(); ++iter)
        {
            outFile << iter->second;
            count++;
            if (count >= nBest) break;
        }
        outFile.close();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << "\n";
    }
    catch (...)
    {
        return __LINE__;
    }
    return 0;
}

// read a population
// this can be quite slow because it re-sorts everything which may not be necessary
// and it may not preserve existing fitnesses if they are not all unique
int Population::ReadPopulation(const char *filename)
{
    std::ifstream inFile;
    inFile.exceptions (std::ios::failbit|std::ios::badbit|std::ios::eofbit);
    try
    {
        inFile.open(filename);

        m_population.clear();
        m_populationIndex.clear();
        m_immortalListIndex.clear();
        m_ageList.clear();
        Random random;

        size_t populationSize;
        inFile >> populationSize;
        bool warningEmitted = false;
        for (size_t i = 0; i < populationSize; i++)
        {
            Genome genome;
            inFile >> genome;
            double fitness = genome.GetFitness();
            if (m_minimizeScore) { fitness = -fitness; } // now all the sorting will hapen in reverse
            if (i > 0 && m_population.find(fitness) != m_population.end())
            {
                while (m_population.find(fitness) != m_population.end())
                {
                    fitness = random.RandomDouble(0, 1);
                }
                genome.SetFitness(fitness); // this line forces all the genomes to be inserted since they might not have valid fitnesses
                if (!warningEmitted)
                {
                    std::cerr << "Warning: population contains duplicate fitness values. Setting to fake values. index first detected = " << i << "\n";
                    warningEmitted = true;
                }
            }
            InsertGenome(std::move(genome), populationSize);
        }
        inFile.close();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << "\n";
    }
    catch (...)
    {
        return __LINE__;
    }
    return 0;
}

