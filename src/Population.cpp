/*
 *  Population.cpp
 *  AsynchronousGA
 *
 *  Created by Bill Sellers on 19/10/2010.
 *  Copyright 2010 Bill Sellers. All rights reserved.
 *
 */

#include "Genome.h"
#include "Population.h"
#include "Random.h"
#include "Mating.h"

#include <iostream>
#include <fstream>
#include <memory>
#include <cmath>
#include <limits>
#include <algorithm>

//#define DEBUG_POPULATION

// this inserts an item into a sorted vec
template<typename T> typename std::vector<T>::iterator insert_sorted(std::vector<T> &vec, T const& item, bool unique)
{
    if (unique) // only insert if the item is not already there
    {
        auto iter = std::lower_bound(vec.begin(), vec.end(), item);
        // lower_bound
        // if a searching element exists: std::lower_bound() returns iterator to the element itself
        if (iter != vec.end() && *iter == item) return iter;
        // if a searching element doesn't exist:
        //    if all elements are greater than the searching element: lower_bound() returns an iterator to begin of the range
        //    if all elements are lower than the searching element: lower_bound() returns an iterator to end of the range
        //    otherwise, lower_bound() returns an iterator to the next greater element to the search elementof the range
        // in all these cases I can just insert the iterator
        return vec.insert(iter, item);
    }
    // upper_bound
    // if a searching element exists: std::upper_bound() returns an iterator to the next greater element of the searching element
    // if searching element doesn't exist:
    //     if all elements are greater than the searching element: upper_bound() returns an iterator to begin of the range
    //     if all elements are lower than the searching element: upper_bound() returns an iterator to end of the range
    //     otherwise, upper_bound() returns an iterator to the next greater element to the search element
    // in all these cases I can just insert the iterator
    return vec.insert (std::upper_bound(vec.begin(), vec.end(), item), item);
}

// this inserts an item into a sorted vec using a custom sorting function (less than function)
template<typename T, typename Pred> typename std::vector<T>::iterator insert_sorted(std::vector<T> & vec, T const& item, Pred pred, bool unique)
{
    if (unique) // only insert if the item is not already there
    {
        auto iter = std::lower_bound(vec.begin(), vec.end(), item, pred);
        // lower_bound
        // if a searching element exists: std::lower_bound() returns iterator to the element itself
        if (iter != vec.end() && *iter == item) return iter;
        // if a searching element doesn't exist:
        //    if all elements are greater than the searching element: lower_bound() returns an iterator to begin of the range
        //    if all elements are lower than the searching element: lower_bound() returns an iterator to end of the range
        //    otherwise, lower_bound() returns an iterator to the next greater element to the search elementof the range
        // in all these cases I can just insert the iterator
        return vec.insert(iter, item);
    }
    // upper_bound
    // if a searching element exists: std::upper_bound() returns an iterator to the next greater element of the searching element
    // if searching element doesn't exist:
    //     if all elements are greater than the searching element: upper_bound() returns an iterator to begin of the range
    //     if all elements are lower than the searching element: upper_bound() returns an iterator to end of the range
    //     otherwise, upper_bound() returns an iterator to the next greater element to the search element
    // in all these cases I can just insert the iterator
    return vec.insert (std::upper_bound(vec.begin(), vec.end(), item, pred), item);
}

// constructor
Population::Population()
{
}

// choose a parent from a population
Genome *Population::ChooseParent(size_t *parentRank)
{
    switch(m_selectionType)
    {
    // this type biases random choice to higher ranked individuals using the gamma function
    // this assumes a sorted genome
    case GammaBasedSelection:
        *parentRank = size_t(m_random.GammaBiasedRandomInt(0, int(m_population.size() - 1), m_gamma));
        return m_population[*parentRank].get();

    // in this version we do uniform selection and just choose a parent
    // at random
    case UniformSelection:
        *parentRank = size_t(m_random.RandomInt(0, int(m_population.size() - 1)));
        return m_population[*parentRank].get();

    // this type biases random choice to higher ranked individuals
    // this assumes a sorted genome
    // note - the distribution is the same as the old RankBasedSelection
    case SqrtBasedSelection:
        *parentRank = size_t(m_random.SqrtBiasedRandomInt(0, int(m_population.size() - 1)));
        return m_population[*parentRank].get();
    }
    return nullptr;
}

// insert a genome into the population
// the population is always kept sorted by fitness
// the oldest genome is deleted unless it is in the
// immortal list
// the key is the numeric value of the fitness so there are rare cases when
// different genomes with the same fitness will not be accepted
int Population::InsertGenome(std::unique_ptr<Genome> genome, size_t targetPopulationSize)
{
    size_t originalSize = m_population.size();
    if (targetPopulationSize == 0) targetPopulationSize = originalSize; // not trying to change the size of the population

    std::vector<std::unique_ptr<Genome>>::iterator iter;
    if (m_minimizeScore) { iter = std::lower_bound(m_population.begin(), m_population.end(), genome, [](const std::unique_ptr<Genome> &lhs, const std::unique_ptr<Genome> &rhs) -> bool { return lhs->GetFitness() > rhs->GetFitness(); }); }
    else { iter = std::lower_bound(m_population.begin(), m_population.end(), genome, [](const std::unique_ptr<Genome> &lhs, const std::unique_ptr<Genome> &rhs) -> bool { return lhs->GetFitness() < rhs->GetFitness(); }); }
    // lower_bound
    // if a searching element exists: std::lower_bound() returns iterator to the element itself
    if (iter != m_population.end() && iter->get()->GetFitness() == genome->GetFitness())
    {
#ifdef DEBUG_POPULATION
        std::cerr << "InsertGenome genome->GetFitness()= " << genome->GetFitness() << " already in population\n";
#endif
        return std::distance(std::begin(m_population), iter); // so just return the index
    }
    size_t index = std::distance(std::begin(m_population), iter);
    // if a searching element doesn't exist:
    //    if all elements are greater than the searching element: lower_bound() returns an iterator to begin of the range
    //    if all elements are lower than the searching element: lower_bound() returns an iterator to end of the range
    //    otherwise, lower_bound() returns an iterator to the next greater element to the search elementof the range
    // in all these cases I can just insert the iterator
    Genome *genomePtr = genome.get();
    m_population.insert(iter, std::move(genome));
#ifdef DEBUG_POPULATION
    std::cerr << "m_population.size(()=" << m_population.size() << " genome->GetFitness()=" << genomePtr->GetFitness() << "\n";
#endif

    // ok now insert into the other internal lists
    if (m_parentsToKeep == 0)
    {
        m_ageList.push_back(genomePtr); // just add current genome to list by age
#ifdef DEBUG_POPULATION
        std::cerr << "InsertGenome adding to m_AgeList - no test; size=" << m_ageList.size() << "\n";
#endif
    }
    else
    {
        // need to worry about immortality
        if (m_immortalList.size() < m_parentsToKeep)
        {
            std::vector<Genome *>::iterator iter2;
            if (m_minimizeScore) { iter2 = std::lower_bound(m_immortalList.begin(), m_immortalList.end(), genomePtr, [](const Genome *lhs, const Genome *rhs) -> bool { return lhs->GetFitness() > rhs->GetFitness(); }); }
            else { iter2 = std::lower_bound(m_immortalList.begin(), m_immortalList.end(), genomePtr, [](const Genome *lhs, const Genome *rhs) -> bool { return lhs->GetFitness() < rhs->GetFitness(); }); }
            if (iter2 != m_immortalList.end() && *iter2 == genomePtr)
            {
                std::cerr << "Logic error Population::InsertGenome m_immortalList already contains genomePtr " << __LINE__ << "\n";
            }
            else
            {
                m_immortalList.insert(iter2, genomePtr);
            }
#ifdef DEBUG_POPULATION
            std::cerr << "InsertGenome adding to m_immortalList - no test; size=" << m_immortalList.size() << "\n";
#endif
        }
        else
        {
            if (m_minimizeScore)
            {
                if (genomePtr->GetFitness() < m_immortalList.front()->GetFitness())
                {
                    std::vector<Genome *>::iterator iter2 = std::lower_bound(m_immortalList.begin(), m_immortalList.end(), genomePtr, [](const Genome *lhs, const Genome *rhs) -> bool { return lhs->GetFitness() > rhs->GetFitness(); });
                    if (iter2 != m_immortalList.end() && *iter2 == genomePtr)
                    {
                        std::cerr << "Logic error Population::InsertGenome m_immortalList already contains genomePtr " << __LINE__ << "\n";
                    }
                    else
                    {
                        m_immortalList.insert(iter2, genomePtr);
                        m_ageList.push_back(m_immortalList.front());
                        m_immortalList.erase(m_immortalList.begin());
                    }
#ifdef DEBUG_POPULATION
                    std::cerr << "InsertGenome adding to m_immortalList; size=" << m_immortalList.size() << "\n";
#endif
                }
                else
                {
                    m_ageList.push_back(genomePtr);
#ifdef DEBUG_POPULATION
                    std::cerr << "InsertGenome adding to m_ageList after test; size=" << m_ageList.size() << "\n";
#endif
                }
            }
            else
            {
                if (genomePtr->GetFitness() > m_immortalList.front()->GetFitness())
                {
                    std::vector<Genome *>::iterator iter2 = std::lower_bound(m_immortalList.begin(), m_immortalList.end(), genomePtr, [](const Genome *lhs, const Genome *rhs) -> bool { return lhs->GetFitness() < rhs->GetFitness(); });
                    if (iter2 != m_immortalList.end() && *iter2 == genomePtr)
                    {
                        std::cerr << "Logic error Population::InsertGenome m_immortalList already contains genomePtr " << __LINE__ << "\n";
                    }
                    else
                    {
                        m_immortalList.insert(iter2, genomePtr);
                        m_ageList.push_back(m_immortalList.front());
                        m_immortalList.erase(m_immortalList.begin());
                    }
#ifdef DEBUG_POPULATION
                    std::cerr << "InsertGenome adding to m_immortalList; size=" << m_immortalList.size() << "\n";
#endif
                }
                else
                {
                    m_ageList.push_back(genomePtr);
#ifdef DEBUG_POPULATION
                    std::cerr << "InsertGenome adding to m_ageList after test; size=" << m_ageList.size() << "\n";
#endif
                }
            }
        }
    }

    // check population sizes
    while (m_population.size() > targetPopulationSize)
    {
        if (m_ageList.size() == 0)
        {
            std::cerr << "Logic error Population::InsertGenome m_ageList has zero size " << __LINE__ << "\n";
            m_population.erase(m_population.begin());
            continue;
        }
        Genome *genomeToDelete = *m_ageList.begin();
        m_ageList.erase(m_ageList.begin()); // equivalent to pop_front()
#ifdef DEBUG_POPULATION
        std::cerr << "InsertGenome reducing m_ageList size; size=" << m_ageList.size() << "\n";
#endif

        std::vector<std::unique_ptr<Genome>>::iterator iter3;
        if (m_minimizeScore) { iter3 = std::lower_bound(m_population.begin(), m_population.end(), genomeToDelete, [](const std::unique_ptr<Genome> &lhs, const Genome *rhs) -> bool { return lhs->GetFitness() > rhs->GetFitness(); }); }
        else { iter3 = std::lower_bound(m_population.begin(), m_population.end(), genomeToDelete, [](const std::unique_ptr<Genome> &lhs, const Genome *rhs) -> bool { return lhs->GetFitness() < rhs->GetFitness(); }); }
        // lower_bound
        // if a searching element exists: std::lower_bound() returns iterator to the element itself
        if (*iter3 && (*iter3)->GetFitness() == genomeToDelete->GetFitness())
        {
            m_population.erase(iter3);
#ifdef DEBUG_POPULATION
            std::cerr << "InsertGenome reducing m_population size; size=" << m_population.size() << "\n";
#endif
        }
        else
        {
            std::cerr << "Logic error Population::InsertGenome genome not found in m_Population " << __LINE__ << "\n";
            m_population.erase(m_population.begin());
            continue;
        }
    }
#ifdef DEBUG_POPULATION
    if (m_ageList.size() + m_immortalList.size() != m_population.size())
    {
         std::cerr << "Logic error Population::InsertGenome m_ageList.size() + m_immortalList.size() != m_population.size() " << __LINE__ << "\n";
    }
#endif
    return index;
}

// randomise the population
void Population::Randomise()
{
    for (auto &&iter : m_population) (*iter).Randomise(&m_random);
}

// reset the population size to a new value - needs at least one valid genome in population
void Population::ResizePopulation(size_t size)
{
    if (m_population.size() == size) return;
    if (size > m_population.size())
    {
        size_t parentRank;
        switch (m_resizeControl)
        {
        case RandomiseResize:
            // fill in with random genomes
            for (size_t i = m_population.size(); i < size; i++)
            {
                auto g = std::make_unique<Genome>(*ChooseParent(&parentRank));
                g->Randomise(&m_random);
                if (m_minimizeScore) {g->SetFitness(std::nextafter(m_population.front()->GetFitness(), std::numeric_limits<double>::max()));}
                else { g->SetFitness(std::nextafter(m_population.front()->GetFitness(), -std::numeric_limits<double>::max())); }
                InsertGenome(std::move(g), size);
            }
            break;

        case MutateResize:
            // fill in with mutated genomes
            for (size_t i = m_population.size(); i < size; i++)
            {
                auto g = std::make_unique<Genome>(*ChooseParent(&parentRank));
                Mating mating(&m_random);
                int mutationCount = 0;
                while (mutationCount == 0)
                {
                    if (m_multipleGaussian)  mutationCount += mating.MultipleGaussianMutate(g.get(), m_gaussianMutationChance, m_bounceMutation);
                    else mutationCount += mating.GaussianMutate(g.get(), m_gaussianMutationChance, m_bounceMutation);
                }
                if (m_minimizeScore) {g->SetFitness(std::nextafter(m_population.front()->GetFitness(), std::numeric_limits<double>::max()));}
                else { g->SetFitness(std::nextafter(m_population.front()->GetFitness(), -std::numeric_limits<double>::max())); }
                InsertGenome(std::move(g), size);
            }
            break;

        }
    }
    else
    {
        size_t delta = m_population.size() - size;
        std::vector<std::unique_ptr<Genome>> population;
        std::swap(population, m_population);
        m_immortalList.clear();
        m_ageList.clear();
        for (size_t i = 0; i < size; i++)
        {
            InsertGenome(std::move(population[delta + size]), size);
        }
    }
}

// set the circular flags for the genomes in the population
void Population::SetGlobalCircularMutation(bool circularMutation)
{
    for (auto &&iter : m_population) { iter->SetGlobalCircularMutationFlag(circularMutation); }
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
            outFile << **iter;
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

// read a population (requires unique fitnesses)
int Population::ReadPopulation(const char *filename)
{
    std::ifstream inFile;
    inFile.exceptions (std::ios::failbit|std::ios::badbit|std::ios::eofbit);
    try
    {
        inFile.open(filename);

        m_population.clear();
        m_immortalList.clear();
        m_ageList.clear();
        Random random;

        size_t populationSize;
        inFile >> populationSize;
        for (size_t i = 0; i < populationSize; i++)
        {
            auto genome = std::make_unique<Genome>();
            inFile >> *genome;
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

Genome Population::GetOffspring()
{
    Genome offspring;
    Genome *parent1, *parent2;
    Mating mating(&m_random);
    int mutationCount = 0;
    size_t parent1Rank, parent2Rank;
    while (mutationCount == 0) // this means we always get some mutation (no point in getting unmutated offspring)
    {
        parent1 = ChooseParent(&parent1Rank);
        offspring = *parent1;
        if (m_random.CoinFlip(m_crossoverChance))
        {
            parent2 = ChooseParent(&parent2Rank);
            mutationCount += mating.Mate(parent1, parent2, &offspring, m_crossoverType);
        }
        if (m_multipleGaussian)  mutationCount += mating.MultipleGaussianMutate(&offspring, m_gaussianMutationChance, m_bounceMutation);
        else mutationCount += mating.GaussianMutate(&offspring, m_gaussianMutationChance, m_bounceMutation);

        mutationCount += mating.FrameShiftMutate(&offspring, m_frameShiftMutationChance);
        mutationCount += mating.DuplicationMutate(&offspring, m_duplicationMutationChance);
    }
    return offspring;
}
