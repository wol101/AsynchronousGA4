#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import pandas
import matplotlib.pyplot

# Step 1: Read the CSV file into a Pandas DataFrame
csv_file_path = 'D:/wis/Scratch/random_text.tab'
df = pandas.read_csv(csv_file_path, delimiter = '\t')

# double randomDouble = random.RandomDouble(-10, 10);
# int randomInt = random.RandomInt(1, 6);
# bool coinFlip = random.CoinFlip(0.25);
# int sqrtBiasedRandomInt = random.SqrtBiasedRandomInt(0, 1000);
# double randomUnitGaussian = random.RandomUnitGaussian();
# int gammaBiasedRandomInt = random.GammaBiasedRandomInt(0, 500, 0.2);
# int rankBiasedRandomInt = random.RankBiasedRandomInt(0, 100);

# Step 2: Plot a histogram
column_to_plot = 'randomDouble'
matplotlib.pyplot.hist(df[column_to_plot], edgecolor='black', bins=[x*2-10.0 for x in range(0,11)])
matplotlib.pyplot.xlabel('Values')
matplotlib.pyplot.ylabel('Frequency')
matplotlib.pyplot.title(f'Histogram of {column_to_plot}')
matplotlib.pyplot.grid(True)
matplotlib.pyplot.show()

column_to_plot = 'randomInt'
matplotlib.pyplot.hist(df[column_to_plot], edgecolor='black', bins=[x+0.5 for x in range(0,7)])
matplotlib.pyplot.xlabel('Values')
matplotlib.pyplot.ylabel('Frequency')
matplotlib.pyplot.title(f'Histogram of {column_to_plot}')
matplotlib.pyplot.grid(True)
matplotlib.pyplot.show()

column_to_plot = 'coinFlip'
matplotlib.pyplot.hist(df[column_to_plot], edgecolor='black', bins=[x-0.5 for x in range(0,3)])
matplotlib.pyplot.xlabel('Values')
matplotlib.pyplot.ylabel('Frequency')
matplotlib.pyplot.title(f'Histogram of {column_to_plot}')
matplotlib.pyplot.grid(True)
matplotlib.pyplot.show()

column_to_plot = 'sqrtBiasedRandomInt'
matplotlib.pyplot.hist(df[column_to_plot], edgecolor='black', bins=[x*10-0.5 for x in range(0,101)])
matplotlib.pyplot.xlabel('Values')
matplotlib.pyplot.ylabel('Frequency')
matplotlib.pyplot.title(f'Histogram of {column_to_plot}')
matplotlib.pyplot.grid(True)
matplotlib.pyplot.show()

column_to_plot = 'randomUnitGaussian'
matplotlib.pyplot.hist(df[column_to_plot], edgecolor='black', bins=[x*0.1-5.05 for x in range(0,101)])
matplotlib.pyplot.xlabel('Values')
matplotlib.pyplot.ylabel('Frequency')
matplotlib.pyplot.title(f'Histogram of {column_to_plot}')
matplotlib.pyplot.grid(True)
matplotlib.pyplot.show()

column_to_plot = 'gammaBiasedRandomInt'
matplotlib.pyplot.hist(df[column_to_plot], edgecolor='black', bins=[x*5-0.5 for x in range(0,101)])
matplotlib.pyplot.xlabel('Values')
matplotlib.pyplot.ylabel('Frequency')
matplotlib.pyplot.title(f'Histogram of {column_to_plot}')
matplotlib.pyplot.grid(True)
matplotlib.pyplot.show()





