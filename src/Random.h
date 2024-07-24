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
#include <array>

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
    std::vector<int> m_cumulativeRank;
    std::array<int, 2> m_cumulativeRankBounds = {std::numeric_limits<int>::max(), std::numeric_limits<int>::min()};
};

#endif // RANDOM_H
