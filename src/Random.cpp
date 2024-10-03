/*
 *  Random.cpp
 *  GeneralGA
 *
 *  Created by Bill Sellers on Mon Jan 31 2005.
 *  Copyright (c) 2005 Bill Sellers. All rights reserved.
 *
 */

#include "Random.h"

#include <cmath>

Random::Random()
{
    std::random_device randomDevice;
    m_randomNumberGenerator.seed(randomDevice());
}

// set the random number generator seed
void Random::RandomSeed(uint64_t randomSeed)
{
    m_randomNumberGenerator.seed(randomSeed);
}

// random double between limits
// random generators vary a bit but generally the value
// will never equal the limits
double Random::RandomDouble(double lowBound, double highBound)
{
    if (lowBound >= highBound) return lowBound;
    std::uniform_real_distribution<double> doubleDistribution(lowBound, highBound);
    return doubleDistribution(m_randomNumberGenerator);
}

// random int between limits
// the value can definitely equal both boundaries
int Random::RandomInt(int lowBound, int highBound)
{
    if (lowBound >= highBound) return lowBound;
    std::uniform_int_distribution<int> intDistribution(lowBound, highBound);
    return intDistribution(m_randomNumberGenerator);
}

// square root biased random int between limits
// higher numbers more likely
// note - the distribution is the same as the old RankBasedSelection
int Random::SqrtBiasedRandomInt(int lowBound, int highBound)
{
    if (lowBound >= highBound) return lowBound;
    double span = (static_cast<double>(highBound) + 0.5) - (static_cast<double>(lowBound) - 0.5);
    double v = std::sqrt(RandomDouble(0.0, 1.0)) * span + static_cast<double>(lowBound) - 0.5;

    int i = static_cast<int>(v + 0.5);

    // should never happen - but rounding errors...
    if (i < lowBound) i = lowBound;
    else if (i > highBound) i = highBound;

    return i;
}

// gamma biased random number
// if gamma is less than 1 then higher rank numbers are more likely
// if gamme is 1 produces a uniform distribution
// if gamma is 0.5 matches the square root dirstibution
// y = pow(x, gamma) with x in range 0 to 1
int Random::GammaBiasedRandomInt(int lowBound, int highBound, double gamma)
{
    if (lowBound >= highBound) return lowBound;
    // calculate a gamma biased random value from 0 to 1

    double v = std::pow(RandomDouble(0.0, 1.0), gamma);

    // now select a suitable integer from the range

    double range = static_cast<double>(1 + highBound - lowBound);

    int i = static_cast<int>(static_cast<double>(lowBound) + v * range);

    // that should be enough but if v ever equalled 1 then it would give
    // highBound + 1 as an answer

    if (i < lowBound) i = lowBound;
    else if (i > highBound) i = highBound;

    return i;
}

// random coin flip - returns true a proportion of the time that
// depends on 'chanceOfReturningTrue'
bool Random::CoinFlip(double chanceOfReturningTrue)
{
    if (chanceOfReturningTrue == 0) return false;
    if (RandomDouble(0.0, 1.0) < chanceOfReturningTrue) return true;
    else return false;
}

// unit gaussian routine from galib
// Return a number from a unit Gaussian distribution.  The mean is 0 and the
// standard deviation is 1.0.
//   First we generate two uniformly random variables inside the complex unit
// circle.  Then we transform these into Gaussians using the Box-Muller
// transformation.  This method is described in Numerical Recipes in C
// ISBN 0-521-43108-5 at http://world.std.com/~nr
//   When we find a number, we also find its twin, so we cache that here so
// that every other call is a lookup rather than a calculation.  (I think GNU
// does this in their implementations as well, but I don't remember for
// certain.)
double Random::RandomUnitGaussian()
{
    static bool cached = false;
    static double cachevalue;
    if (cached == true)
    {
        cached = false;
        return cachevalue;
    }

    double rsquare, factor, var1, var2;
    do
    {
        var1 = 2.0 * RandomDouble(0.0, 1.0) - 1.0;
        var2 = 2.0 * RandomDouble(0.0, 1.0) - 1.0;
        rsquare = var1 * var1 + var2 * var2;
    }
    while (rsquare >= 1.0 || rsquare == 0.0);

    double val = -2.0 * log(rsquare) / rsquare;
    if (val > 0.0)
        factor = sqrt(val);
    else
        factor = 0.0;  // should not happen, but might due to roundoff

    cachevalue = var1 * factor;
    cached = true;

    return (var2 * factor);
}

