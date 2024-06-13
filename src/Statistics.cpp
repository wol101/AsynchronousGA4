/*
 *  Statistics.cpp
 *  AsynchronousGA
 *
 *  Created by Bill Sellers on 19/10/2010.
 *  Copyright 2010 Bill Sellers. All rights reserved.
 *
 */

#include <stdio.h>
#include <assert.h>
#include <iostream>
#include <math.h>
#include <float.h>
#include <iomanip>

#include "Statistics.h"
#include "Genome.h"
#include "Population.h"

// calculate standard statistics
void CalculateStatistics(Population *thePopulation, Statistics *stats)
{
    int i;
    double min = DBL_MAX;
    double max = -DBL_MAX;
    double sum = 0;
    double sumSquareDiff = 0;
    double variance;
    double mean;
    double fitness;
    double diff;
    double sd;
    int populationSize = thePopulation->GetPopulationSize();

    for (i = 0; i < populationSize; i++)
    {
        fitness = thePopulation->GetGenome(i)->GetFitness();
        if (fitness > max) max = fitness;
        if (fitness < min) min = fitness;
        sum += fitness;
    }

    mean = sum / populationSize;
    for (i = 0; i < populationSize; i++)
    {
        fitness = thePopulation->GetGenome(i)->GetFitness();
        diff = fitness - mean;
        sumSquareDiff += diff * diff;
    }
    variance = sumSquareDiff / populationSize;
    sd = sqrt(variance);

    stats->min = min;
    stats->max = max;
    stats->mean = mean;
    stats->sd = sd;
}

// output to a stream
std::ostream& operator<<(std::ostream &out, Statistics &s)
{
    out << std::fixed << std::setw(10) << std::setprecision(5) << s.max << " " << s.mean << " " << s.min << " " << s.sd;
    return out;
}

// calculate percentiles at 10% intervals
// ASSUMES POPULATION IS SORTED!
void CalculateTenPercentiles(Population *thePopulation, TenPercentiles *perc)
{
    double delta = double(thePopulation->GetPopulationSize()) / 10.0;
    int i, j;
    double index = 0;

    for (i = 0; i < 11; i ++)
    {
        j = int(index + 0.5);
        if (j >= thePopulation->GetPopulationSize()) j = thePopulation->GetPopulationSize() - 1;
        perc->values[i] = thePopulation->GetGenome(j)->GetFitness();
        index += delta;
    }
}

// output to a stream
std::ostream& operator<<(std::ostream &out, TenPercentiles &s)
{
    int i;
    out << std::fixed << std::setw(10) << std::setprecision(5);
    for (i = 0; i < 11; i++)
    {
        if (i == 10) out << s.values[i];
        else out << s.values[i] << " ";
    }
    return out;
}

