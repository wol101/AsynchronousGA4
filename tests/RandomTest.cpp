#include "../src/Random.h"

#include <fstream>

int main(int argc, const char **argv)
{
    Random random;
    std::ofstream outputFile("D:/wis/Scratch/random_text.tab");
    outputFile << "i\trandomDouble\trandomInt\tcoinFlip\tsqrtBiasedRandomInt\trandomUnitGaussian\tgammaBiasedRandomInt\trankBiasedRandomInt\n";

    for (int i = 0; i < 1000; i++)
    {
        double randomDouble = random.RandomDouble(-10, 10);
        int randomInt = random.RandomInt(1, 6);
        bool coinFlip = random.CoinFlip(0.25);
        int sqrtBiasedRandomInt = random.SqrtBiasedRandomInt(0, 1000);
        double randomUnitGaussian = random.RandomUnitGaussian();
        int gammaBiasedRandomInt = random.GammaBiasedRandomInt(0, 500, 0.2);
        int rankBiasedRandomInt = random.RankBiasedRandomInt(0, 100);
        outputFile << i << "\t" << randomDouble << "\t" << randomInt << "\t" << coinFlip << "\t" << sqrtBiasedRandomInt << "\t" << randomUnitGaussian << "\t" << gammaBiasedRandomInt << "\t" << rankBiasedRandomInt << "\n";
    }
}


