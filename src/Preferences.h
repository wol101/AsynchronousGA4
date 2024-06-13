/*
 *  Preferences.h
 *  AsynchronousGA
 *
 *  Created by Bill Sellers on 10/11/2010.
 *  Copyright 2010 Bill Sellers. All rights reserved.
 *
 */

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "DataFile.h"
#include "Population.h"
#include "Mating.h"
#include "Random.h"

class Preferences
{
public:
    Preferences();

    int ReadPreferences(const std::string &filename);
    std::string GetPreferencesString();

    DataFile params;
    int genomeLength = 0;
    int populationSize = 0;
    int maxReproductions = 0;
    double gaussianMutationChance = 0;
    double frameShiftMutationChance = 0;
    double duplicationMutationChance = 0;
    double crossoverChance = 0;
    int parentsToKeep = 0;
    int saveBestEvery = 0;
    int savePopEvery = 0;
    int outputStatsEvery = 0;
    bool onlyKeepBestGenome = false;
    bool onlyKeepBestPopulation = false;
    int improvementReproductions = 0;
    double improvementThreshold = 0;
    bool multipleGaussian = false;
    SelectionType parentSelection = RankBasedSelection;
    Mating::CrossoverType crossoverType = Mating::Average;
    std::string startingPopulation;
    bool randomiseModel = false;
    double watchDogTimerLimit = 60 * 5;
    int outputPopulationSize;
    double gamma = 0.5;
    bool circularMutation = false;
    bool bounceMutation = true;
    bool minimizeScore = false;
    ResizeControl resizeControl = MutateResize;
};

#endif // PREFERENCES_H
