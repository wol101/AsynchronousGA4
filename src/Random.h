/*
 *  Random.cpp
 *  GeneralGA
 *
 *  Created by Bill Sellers on Mon Jan 31 2005.
 *  Copyright (c) 2005 Bill Sellers. All rights reserved.
 *
 */

#ifndef RANDOM_H
#define RANDOM_H

#include <random>

class Random
{
public:
    Random();

    void RandomSeed(uint64_t randomSeed);
    double RandomDouble(double lowBound, double highBound);
    int RandomInt(int lowBound, int highBound);
    bool CoinFlip(double chanceOfReturningTrue = 0.5);
    int SqrtBiasedRandomInt(int lowBound, int highBound);
    double RandomUnitGaussian();
    int RankBiasedRandomInt(int lowBound, int highBound);
    int GammaBiasedRandomInt(int lowBound, int highBound, double gamma);

private:
    std::mt19937_64 m_randomNumberGenerator;
};

#endif // RANDOM_H
