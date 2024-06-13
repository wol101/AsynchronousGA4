/*
 *  Genome.h
 *  AsynchronousGA
 *
 *  Created by Bill Sellers on 19/10/2010.
 *  Copyright 2010 Bill Sellers. All rights reserved.
 *
 */

#ifndef GENOME_H
#define GENOME_H

#include <iostream>
#include <vector>
#include <limits>

class Random;

class Genome
{
public:

    Genome();
    Genome(const Genome &g);
    Genome(Genome &&g);
    Genome& operator=(const Genome &g);
    Genome& operator=(Genome &&g);

    enum GenomeType
    {
        IndividualRanges = -1,
        IndividualCircularMutation = -2
    };

    double GetGene(size_t i) const { return m_genes[i]; }
    size_t GetGenomeLength() const { return m_genes.size(); }
    double GetHighBound(size_t i) const { return m_highBounds[i]; }
    double GetLowBound(size_t i) const { return m_lowBounds[i]; }
    double GetGaussianSD(size_t i) const { return m_gaussianSDs[i]; }
    double GetFitness() const { return m_fitness; }
    GenomeType GetGenomeType() const { return m_genomeType; }
    std::vector<double> *GetGenes() { return &m_genes; }
    bool GetCircularMutation(int i);
    bool GetGlobalCircularMutationFlag() { return m_globalCircularMutationFlag; }

    void Randomise(Random *random);
    void SetGene(size_t i, double value) { m_genes[i] = value; }
    void SetFitness(double fitness) { m_fitness = fitness; }
    void SetCircularMutation(size_t i, bool circularFlag);
    void SetGlobalCircularMutationFlag(bool circularFlag) { m_globalCircularMutationFlag = circularFlag; }
    void Clear();

    bool operator<(const Genome &in);

    friend std::ostream& operator<<(std::ostream &out, const Genome &g);
    friend std::istream& operator>>(std::istream &in, Genome &g);

private:

    std::vector<double> m_genes;
    std::vector<double> m_lowBounds;
    std::vector<double> m_highBounds;
    std::vector<double> m_gaussianSDs;
    std::vector<char> m_circularMutationFlags;
    GenomeType m_genomeType = IndividualRanges;
    bool m_globalCircularMutationFlag = false;
    double m_fitness = -std::numeric_limits<double>::max();
};

#endif // GENOME_H
