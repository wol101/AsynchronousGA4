/*
 *  Mating.cpp
 *  AsynchronousGA
 *
 *  Created by Bill Sellers on 19/10/2010.
 *  Copyright 2010 Bill Sellers. All rights reserved.
 *
 */

#include "Mating.h"
#include "Random.h"
#include "Genome.h"

#include <iostream>
#include <cmath>
#include <memory>

Mating::Mating(Random *random)
{
    m_random = random;
}


// mate two parents producing an offspring
// some crossover *ALWAYS* occurs
int Mating::Mate(Genome *parent1, Genome *parent2, Genome *offspring, Mating::CrossoverType type)
{

    int i;
    int genomeLength = offspring->GetGenomeLength();

    switch (type)
    {
    case OnePoint:
    {
        int crossoverPoint = m_random->RandomInt(1, genomeLength - 1);

        for (i = 0; i < crossoverPoint; i++)
            offspring->SetGene(i, parent1->GetGene(i));

        for (i = crossoverPoint; i < genomeLength; i++)
            offspring->SetGene(i, parent2->GetGene(i));

        break;
    }

    case Average:
    {
        for (i = 0; i < genomeLength; i++)
            offspring->SetGene(i, (parent1->GetGene(i) + parent2->GetGene(i)) / 2.0);
        break;
    }
    }
    return 1;
}

// mutate an individual by adding a gaussian distributed double
// if gaussianSD <= 0 then don't mutate
int Mating::GaussianMutate(Genome *genome, double mutationChance, bool bounceMutation)
{
    double v = 0;
    int genomeLength = genome->GetGenomeLength();
    int location;
    double r, w;

    if (mutationChance == 0) return 0;
    if (mutationChance < 1.0)
    {
        if (!m_random->CoinFlip(mutationChance)) return 0;
    }

    // gaussian mutator
    r = m_random->RandomUnitGaussian();
    location = m_random->RandomInt(0, genomeLength - 1);
    while (genome->GetGaussianSD(location) > 0)
    {
        if (genome->GetLowBound(location) >= genome->GetHighBound(location))
        {
            genome->SetGene(location, genome->GetLowBound(location));
            break;
        }
        v = genome->GetGene(location) + r * genome->GetGaussianSD(location);
        if (v < genome->GetLowBound(location))
        {
            if (genome->GetCircularMutation(location) == false)
            {
                if (bounceMutation == false)
                {
                    v = genome->GetLowBound(location);
                }
                else
                {
                    w = genome->GetLowBound(location) - v; // must be positive
                    w = fmod(w, genome->GetHighBound(location) - genome->GetLowBound(location));
                    v = genome->GetLowBound(location) + w; // this must be less than genome->GetHighBound(location)
                }
            }
            else
            {
                w = v - genome->GetLowBound(location);
                w = fmod(w, genome->GetHighBound(location) - genome->GetLowBound(location));
                v = genome->GetHighBound(location) + w; // this is right because w must be negative and magnitude <= genome->GetHighBound(location) - genome->GetLowBound(location)
            }
            genome->SetGene(location, v);
            break;
        }
        if (v > genome->GetHighBound(location))
        {
            if (genome->GetCircularMutation(location) == false)
            {
                if (bounceMutation == false)
                {
                    v = genome->GetHighBound(location);
                }
                else
                {
                    w = v - genome->GetHighBound(location); // must be positive
                    w = fmod(w, genome->GetHighBound(location) - genome->GetLowBound(location));
                    v = genome->GetHighBound(location) - w; // this must be higher than genome->GetLowBound(location)
                }
            }
            else
            {
                w = v - genome->GetHighBound(location);
                w = fmod(w, genome->GetHighBound(location) - genome->GetLowBound(location));
                v = genome->GetLowBound(location) + w; // this is right because w must be positive and magnitude <= genome->GetHighBound(location) - genome->GetLowBound(location)
            }
            genome->SetGene(location, v);
            break;
        }
        genome->SetGene(location, v);
    }
    return 1;
}

// mutate an individual by adding a gaussian distributed double
// with a finite chance per gene
// if gaussianSD <= 0 then don't mutate
int Mating::MultipleGaussianMutate(Genome *genome, double mutationChance, bool bounceMutation)
{
    // use the mutation chance on each gene

    int i;
    double v = 0;
    int genomeLength = genome->GetGenomeLength();
    double r, w;

    if (mutationChance == 0) return 0;

    // gaussian mutator
    int mutated = 0;
    for (i = 0; i < genomeLength; i++)
    {
        if (genome->GetLowBound(i) >= genome->GetHighBound(i))
        {
            genome->SetGene(i, genome->GetLowBound(i));
            continue;
        }
        if (m_random->CoinFlip(mutationChance))
        {
            mutated++;
            r = m_random->RandomUnitGaussian();
            if (genome->GetGaussianSD(i) > 0)
            {
                v = genome->GetGene(i) + r * genome->GetGaussianSD(i);
                if (v < genome->GetLowBound(i))
                {
                    if (genome->GetCircularMutation(i) == false)
                    {
                        if (bounceMutation == false)
                        {
                            v = genome->GetLowBound(i);
                        }
                        else
                        {
                            w = genome->GetLowBound(i) - v; // must be positive
                            w = fmod(w, genome->GetHighBound(i) - genome->GetLowBound(i));
                            v = genome->GetLowBound(i) + w; // this must be less than genome->GetHighBound(location)
                        }
                    }
                    else
                    {
                        w = v - genome->GetLowBound(i);
                        w = fmod(w, genome->GetHighBound(i) - genome->GetLowBound(i));
                        v = genome->GetHighBound(i) + w; // this is right because w must be negative and magnitude <= genome->GetHighBound(location) - genome->GetLowBound(location)
                    }
                }
                else
                {
                    if (v > genome->GetHighBound(i))
                    {
                        if (genome->GetCircularMutation(i) == false)
                        {
                            if (bounceMutation == false)
                            {
                                v = genome->GetHighBound(i);
                            }
                            else
                            {
                                w = v - genome->GetHighBound(i); // must be positive
                                w = fmod(w, genome->GetHighBound(i) - genome->GetLowBound(i));
                                v = genome->GetHighBound(i) - w; // this must be higher than genome->GetLowBound(location)
                            }
                        }
                        else
                        {
                            w = v - genome->GetHighBound(i);
                            w = fmod(w, genome->GetHighBound(i) - genome->GetLowBound(i));
                            v = genome->GetLowBound(i) + w; // this is right because w must be positive and magnitude <= genome->GetHighBound(location) - genome->GetLowBound(location)
                        }
                    }
                }
                genome->SetGene(i, v);
            }
        }
    }
    return mutated;
}

// mutate an individual by inserting or deleting a gene
int Mating::FrameShiftMutate(Genome *genome, double mutationChance)
{
    int i;
    int location;
    int genomeLength = genome->GetGenomeLength();

    if (mutationChance == 0) return 0;
    if (mutationChance < 1.0)
    {
        if (!m_random->CoinFlip(mutationChance)) return 0;
    }

    // insertion/deletion
    location = m_random->RandomInt(0, genomeLength - 1);
    if (m_random->CoinFlip(0.5))
    {
        // deletion
        for (i = location; i < genomeLength - 1; i++)
            genome->SetGene(i, genome->GetGene(i + 1));
    }
    else
    {
        // insertion
        for (i = genomeLength - 2; i >= location; i--)
            genome->SetGene(i + 1, genome->GetGene(i));
    }
    return 1;
}

// mutate an individual by duplicating a block and inserting it in a
// random location
int Mating::DuplicationMutate(Genome *genome, double mutationChance)
{
    int i;
    int genomeLength = genome->GetGenomeLength();
    int origin;
    int length;
    int insertion;

    if (mutationChance == 0) return 0;
    if (mutationChance < 1.0)
    {
        if (!m_random->CoinFlip(mutationChance)) return 0;
    }

    // calculate segment to copy
    origin = m_random->RandomInt(0, genomeLength - 1);
    if (genomeLength - origin == 1) length = 1;
    else length = m_random->RandomInt(1, genomeLength - origin);

    // make a copy
    std::unique_ptr<double[]> store = std::make_unique<double[]>(length);
    for (i = 0; i < length; i++)
        store[i] = genome->GetGene(origin + i);

    // and write the copy into the genome
    insertion = m_random->RandomInt(0, genomeLength - 1);
    for (i = 0; i < length; i++)
    {
        genome->SetGene(insertion, store[i]);
        insertion++;
        if (insertion >= genomeLength) break;
    }

    return 1;
}

void Mating::SetRandom(Random *random)
{
    m_random = random;
}


