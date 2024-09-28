/*
 *  Genome.cpp
 *  AsynchronousGA
 *
 *  Created by Bill Sellers on 19/10/2010.
 *  Copyright 2010 Bill Sellers. All rights reserved.
 *
 */

#include <iostream>
#include <limits>

#include "Genome.h"
#include "Random.h"

// constructor
Genome::Genome()
{
}

Genome::Genome(const Genome &in)
{
    m_genes = in.m_genes;
    m_lowBounds = in.m_lowBounds;
    m_highBounds = in.m_highBounds;
    m_gaussianSDs = in.m_gaussianSDs;
    m_circularMutationFlags = in.m_circularMutationFlags;
    m_genomeType = in.m_genomeType;
    m_globalCircularMutationFlag = in.m_globalCircularMutationFlag;
    m_fitness = in.m_fitness;
}

Genome::Genome(Genome &&in)
{
    m_genes = in.m_genes;
    m_lowBounds = in.m_lowBounds;
    m_highBounds = in.m_highBounds;
    m_gaussianSDs = in.m_gaussianSDs;
    m_circularMutationFlags = in.m_circularMutationFlags;
    m_genomeType = in.m_genomeType;
    m_globalCircularMutationFlag = in.m_globalCircularMutationFlag;
    m_fitness = in.m_fitness;
}

// define = operators
Genome &Genome::operator=(const Genome &in)
{
    if (&in != this)
    {
        m_genes = in.m_genes;
        m_lowBounds = in.m_lowBounds;
        m_highBounds = in.m_highBounds;
        m_gaussianSDs = in.m_gaussianSDs;
        m_circularMutationFlags = in.m_circularMutationFlags;
        m_genomeType = in.m_genomeType;
        m_globalCircularMutationFlag = in.m_globalCircularMutationFlag;
        m_fitness = in.m_fitness;
    }
    return *this;
}

Genome &Genome::operator=(Genome &&in)
{
    if (&in != this)
    {
        m_genes = in.m_genes;
        m_lowBounds = in.m_lowBounds;
        m_highBounds = in.m_highBounds;
        m_gaussianSDs = in.m_gaussianSDs;
        m_circularMutationFlags = in.m_circularMutationFlags;
        m_genomeType = in.m_genomeType;
        m_globalCircularMutationFlag = in.m_globalCircularMutationFlag;
        m_fitness = in.m_fitness;
    }
    return *this;
}

void Genome::Clear()
{
    m_genes.clear();
    m_lowBounds.clear();
    m_highBounds.clear();
    m_gaussianSDs.clear();
    m_circularMutationFlags.clear();
    m_genomeType = IndividualRanges;
    m_globalCircularMutationFlag = false;
    m_fitness = -std::numeric_limits<double>::max();
}

// randomise the genome
void Genome::Randomise(Random *random)
{
    switch (m_genomeType)
    {
    case IndividualRanges:
    case IndividualCircularMutation:
        for (size_t i = 0; i < m_genes.size(); i++)
        {
            if (m_gaussianSDs[i] != 0)
                m_genes[i] = random->RandomDouble(m_lowBounds[i], m_highBounds[i]);
        }
        break;
    }
}

// get the value of the circular mutation flag for a gene
bool Genome::GetCircularMutation(int i)
{
    bool v = 0;

    switch (m_genomeType)
    {
    case IndividualRanges:
        v = m_globalCircularMutationFlag;
        break;

    case IndividualCircularMutation:
        v = m_circularMutationFlags[i];
        break;
    }

    return v;
}

// output to a stream
std::ostream& operator<<(std::ostream &out, const Genome &g)
{
    char buf[256];
    switch (g.m_genomeType)
    {
    case Genome::IndividualRanges:
        std::sprintf(buf, "%d\n", g.m_genomeType);
        out << buf;
        std::sprintf(buf, "%zu\n", g.m_genes.size());
        out << buf;
        for (size_t i = 0; i < g.m_genes.size(); i++)
        {
            std::sprintf(buf, "%.17g\t%.17g\t%.17g\t%.17g\n", g.m_genes[i], g.m_lowBounds[i], g.m_highBounds[i],  g.m_gaussianSDs[i]);
            out << buf;
        }
        std::sprintf(buf, "%.17g\t0\t0\t0\t0\n",  g.m_fitness);
        out << buf;
        break;

    case Genome::IndividualCircularMutation:
        std::sprintf(buf, "%d\n", g.m_genomeType);
        out << buf;
        std::sprintf(buf, "%zu\n", g.m_genes.size());
        out << buf;
        for (size_t i = 0; i < g.m_genes.size(); i++)
        {
            std::sprintf(buf, "%.17g\t%.17g\t%.17g\t%.17g\t%d\n", g.m_genes[i], g.m_lowBounds[i], g.m_highBounds[i],  g.m_gaussianSDs[i], g.m_circularMutationFlags[i]);
            out << buf;
        }
        std::sprintf(buf, "%.17g\t0\t0\t0\t0\n",  g.m_fitness);
        out << buf;
        break;
    }
    return out;
}

// input from a stream
std::istream& operator>>(std::istream &in, Genome &g)
{
    int genomeType;
    size_t genomeLength;
    int dummy = 0;

    in >> genomeType;
    in >> genomeLength;

    g.Clear();
    g.m_genes.resize(genomeLength);
    g.m_lowBounds.resize(genomeLength);
    g.m_highBounds.resize(genomeLength);
    g.m_gaussianSDs.resize(genomeLength);
    g.m_circularMutationFlags.resize(genomeLength);

    g.m_genomeType = (Genome::GenomeType)genomeType;
    switch (g.m_genomeType)
    {
    case Genome::IndividualRanges:
        for (size_t i = 0; i < genomeLength; i++) { in >> g.m_genes[i] >> g.m_lowBounds[i] >> g.m_highBounds[i] >> g.m_gaussianSDs[i]; }
        in >> g.m_fitness >> dummy >> dummy >> dummy >> dummy;
        break;

    case Genome::IndividualCircularMutation:
        for (size_t i = 0; i < genomeLength; i++) { in >> g.m_genes[i] >> g.m_lowBounds[i] >> g.m_highBounds[i] >> g.m_gaussianSDs[i] >> g.m_circularMutationFlags[i]; }
        in >> g.m_fitness >> dummy >> dummy >> dummy >> dummy;
        break;
    }

    return in;
}



