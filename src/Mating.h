/*
 *  Mating.h
 *  AsynchronousGA
 *
 *  Created by Bill Sellers on 19/10/2010.
 *  Copyright 2010 Bill Sellers. All rights reserved.
 *
 */

#ifndef MATING_H
#define MATING_H

class Genome;
class Population;
class Random;

class Mating
{
public:
    Mating(Random *random);

    enum CrossoverType
    {
        OnePoint = 0,
        Average = 1
    };

    void SetRandom(Random *random);
    int Mate(Genome *parent1, Genome *parent2, Genome *offspring, CrossoverType type);
    int GaussianMutate(Genome *genome, double mutationChance, bool bounceMutation);
    int MultipleGaussianMutate(Genome *genome, double mutationChance, bool bounceMutation);
    int FrameShiftMutate(Genome *genome, double mutationChance);
    int DuplicationMutate(Genome *genome, double mutationChance);

private:
    Random *m_random = nullptr;
};


#endif // MATING_H
