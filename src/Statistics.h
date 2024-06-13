/*
 *  Statistics.h
 *  AsynchronousGA
 *
 *  Created by Bill Sellers on 19/10/2010.
 *  Copyright 2010 Bill Sellers. All rights reserved.
 *
 */

#ifndef STATISTICS_H
#define STATISTICS_H

#include <iostream>

class Genome;
class Population;

struct Statistics
{
    double min;
    double max;
    double mean;
    double sd;
};

struct TenPercentiles
{
    double values[11];
};

void CalculateStatistics(Population *thePopulation, Statistics *stats);
void CalculateTenPercentiles(Population *thePopulation, TenPercentiles *perc);
std::ostream& operator<<(std::ostream &out, Statistics &s);
std::ostream& operator<<(std::ostream &out, TenPercentiles &s);


#endif // STATISTICS_H
