/*
 *  Preferences.cpp
 *  AsynchronousGA
 *
 *  Created by Bill Sellers on 10/11/2010.
 *  Copyright 2010 Bill Sellers. All rights reserved.
 *
 */

#include "Preferences.h"

#include <sstream>

using namespace std::string_literals;


Preferences::Preferences()
{
}

int Preferences::ReadPreferences(const std::string &filename)
{

    std::string paramsBuffer;

    try
    {
        if (params.ReadFile(filename)) throw __LINE__;

        // essential parameters
        if (params.RetrieveAttribute("genomeLength", &genomeLength)) throw __LINE__;
        if (params.RetrieveAttribute("populationSize", &populationSize)) throw __LINE__;
        if (params.RetrieveAttribute("maxReproductions", &maxReproductions)) throw __LINE__;
        if (params.RetrieveAttribute("gaussianMutationChance", &gaussianMutationChance)) throw __LINE__;
        if (params.RetrieveAttribute("frameShiftMutationChance", &frameShiftMutationChance)) throw __LINE__;
        if (params.RetrieveAttribute("duplicationMutationChance", &duplicationMutationChance)) throw __LINE__;
        if (params.RetrieveAttribute("crossoverChance", &crossoverChance)) throw __LINE__;
        if (params.RetrieveAttribute("parentsToKeep", &parentsToKeep)) throw __LINE__;
        if (params.RetrieveAttribute("saveBestEvery", &saveBestEvery)) throw __LINE__;
        if (params.RetrieveAttribute("savePopEvery", &savePopEvery)) throw __LINE__;
        if (params.RetrieveAttribute("outputStatsEvery", &outputStatsEvery)) throw __LINE__;
        if (params.RetrieveAttribute("onlyKeepBestGenome", &onlyKeepBestGenome)) throw __LINE__;
        if (params.RetrieveAttribute("onlyKeepBestPopulation", &onlyKeepBestPopulation)) throw __LINE__;
        if (params.RetrieveAttribute("improvementReproductions", &improvementReproductions)) throw __LINE__;
        if (params.RetrieveAttribute("improvementThreshold", &improvementThreshold)) throw __LINE__;
        if (params.RetrieveAttribute("multipleGaussian", &multipleGaussian)) throw __LINE__;
        if (params.RetrieveAttribute("randomiseModel", &randomiseModel)) throw __LINE__;
        if (params.RetrieveAttribute("outputPopulationSize", &outputPopulationSize)) throw __LINE__;
        if (params.RetrieveAttribute("watchDogTimerLimit", &watchDogTimerLimit)) throw __LINE__;

        if (params.RetrieveAttribute("parentSelection", &paramsBuffer)) throw __LINE__;
        if (paramsBuffer == "UniformSelection"s) parentSelection = UniformSelection;
        else if (paramsBuffer == "SqrtBasedSelection"s) parentSelection = SqrtBasedSelection;
        else if (paramsBuffer == "GammaBasedSelection"s) parentSelection = GammaBasedSelection;
        else throw __LINE__;

        if (params.RetrieveAttribute("gamma", &gamma)) throw __LINE__;

        if (params.RetrieveAttribute("crossoverType", &paramsBuffer)) throw __LINE__;
        if (paramsBuffer == "OnePoint"s) crossoverType = Mating::OnePoint;
        else if (paramsBuffer == "Average"s) crossoverType = Mating::Average;
        else throw __LINE__;

        if (params.RetrieveAttribute("circularMutation", &circularMutation)) throw __LINE__;
        if (params.RetrieveAttribute("bounceMutation", &bounceMutation)) throw __LINE__;
        if (params.RetrieveAttribute("minimizeScore", &minimizeScore)) throw __LINE__;

        if (params.RetrieveAttribute("resizeControl", &paramsBuffer)) throw __LINE__;
        if (paramsBuffer == "RandomiseResize"s) resizeControl = RandomiseResize;
        else if (paramsBuffer == "MutateResize"s) resizeControl = MutateResize;
        else throw __LINE__;

        // optional parameters
        params.RetrieveAttribute("startingPopulation", &startingPopulation);

    }

    catch (int e)
    {
        std::cerr << "Preferences::ReadPreferences Parameter file error on line " << e << "\n";
        return e;
    }

    return 0;
}

// output to a string
std::string Preferences::GetPreferencesString()
{
    std::stringstream out;
    out << "genomeLength " << genomeLength << "\n";
    out << "populationSize " << populationSize << "\n";
    out << "maxReproductions " << maxReproductions << "\n";
    out << "gaussianMutationChance " << gaussianMutationChance << "\n";
    out << "frameShiftMutationChance " << frameShiftMutationChance << "\n";
    out << "duplicationMutationChance " << duplicationMutationChance << "\n";
    out << "crossoverChance " << crossoverChance << "\n";
    out << "parentsToKeep " << parentsToKeep << "\n";
    out << "saveBestEvery " << saveBestEvery << "\n";
    out << "savePopEvery " << savePopEvery << "\n";
    out << "outputStatsEvery " << outputStatsEvery << "\n";
    out << "onlyKeepBestGenome " << onlyKeepBestGenome << "\n";
    out << "onlyKeepBestPopulation " << onlyKeepBestPopulation << "\n";
    out << "improvementReproductions " << improvementReproductions << "\n";
    out << "improvementThreshold " << improvementThreshold << "\n";
    out << "multipleGaussian " << multipleGaussian << "\n";
    out << "randomiseModel " << randomiseModel << "\n";
    out << "startingPopulation \"" << startingPopulation << "\"\n";
    out << "outputPopulationSize " << outputPopulationSize << "\n";
    out << "watchDogTimerLimit " << watchDogTimerLimit << "\n";
    out << "circularMutation " << circularMutation << "\n";
    out << "bounceMutation " << bounceMutation << "\n";
    out << "minimizeScore " << minimizeScore << "\n";

    switch (parentSelection)
    {
    case UniformSelection:
        out << "parentSelection " << "UniformSelection\n";
        break;
    case SqrtBasedSelection:
        out << "parentSelection " << "SqrtBasedSelection\n";
        break;
    case GammaBasedSelection:
        out << "parentSelection " << "GammaBasedSelection\n";
        out << "gamma " << gamma << "\n";
        break;
    }

    switch (crossoverType)
    {
    case Mating::OnePoint:
        out << "crossoverType " << "OnePoint\n";
        break;
    case Mating::Average:
        out << "crossoverType " << "Average\n";
        break;
    }

    switch (resizeControl)
    {
    case RandomiseResize:
        out << "resizeControl " << "RandomiseResize\n";
        break;
    case MutateResize:
        out << "resizeControl " << "MutateResize\n";
        break;
    }

    return out.str();
}


