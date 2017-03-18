/*
william bolduc 0851313
orbs.c
contains all the main code, initializing structs and threading
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <smmintrin.h>
#include <omp.h>

#include "wallclock.h"
#include "grid.h"
#include "gridShifters.h"

void testRedShift()
{
	int i,j;
	int fullSectors = 1;
	int partialSectorSize = 64;
	sector_t sectors[3];

	sectors[0] = loadColours("BRR   RRRRRRRRR    BRR                    RRRRRRRRRRRRRRRRRRRRRR");
	sectors[1] = loadColours("BRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR");
	sectors[2] = loadColours("RRRRRRRRRRBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");
	printCells(sectors[0], 64,stdout);
	printCells(sectors[1], 64,stdout);
	printCells(sectors[2], partialSectorSize, stdout);
	printf("\n");

	for (i = 0; i < 120; i++)
	{
		for(j=0; j<fullSectors * BITCOUNT + partialSectorSize; j++) printf("_");
		printf("\n");
		redShift(sectors, fullSectors, partialSectorSize);
		printCells(sectors[0], 64, stdout);
		printCells(sectors[1], 64, stdout);
		//printCells(sectors[2], partialSectorSize,stdout);
		getchar();
	}


	//size 10: fullSector = 0, partial = 10
	//size 64: fullSector = 0, partial = 64
	//size 74: fullSector = 1, partial = 10

}

int main(int argc, char *argv[])
{
	//input arguments
	int threadCount = -1;
	int size = -1;
	int overlaySize = -1;
	int cDensity = -1;
	int maxSteps = -1;
	int seed;
	char seedGiven = 0;
	char interactive = 0;

	char notAllArgumentsGiven = 0;	//used to check for all the required command line arguments

	
	int stepsRun;
	int endDensity = 0;
	FILE *outputFile;

	//thread
	threadArgs_t *threadArgs;
	
	int fullSectors;
	int partialSectorSize;
	sector_t **grid;
	unsigned long long **blueTransfer;
	subSum_t **subSums;
	
	int exitCond = 0;
	threadReturn_t returnVal;

	int maxColour;
	int workStart, workEnd;
	int i,j,step;

	float simTime;

	simTime = 0;

	//load in command line arguments
	for(i = 1; argv[i]; i++)
	{
		switch (argv[i][0])
		{
		case 'p':
			threadCount = atoi(&(argv[i][1]));
			break;
		case 'b':
			size = atoi(&(argv[i][1]));
			break;
		case 't':
			overlaySize = atoi(&(argv[i][1]));
			break;
		case 'c':
			cDensity = atoi(&(argv[i][1]));
			break;
		case 'm':
			maxSteps = atoi(&(argv[i][1]));
			break;
		case 's':
			seedGiven = 1;
			seed = atoi(&(argv[i][1]));
			break;
		case 'i':
			interactive = 1;
			break;
		default:
			printf("\"%s\" - is not a valid option", argv[i]);
			break;
		}
	}

	//check required args
	if (threadCount == -1)
	{
		printf("missing processor count\n");
		notAllArgumentsGiven = 1;
	}
	if (size == -1)
	{
		printf("missing board width\n");
		notAllArgumentsGiven = 1;
	}
	if (overlaySize == -1)
	{
		printf("missing overlay tile width\n");
		notAllArgumentsGiven = 1;
	}
	if (cDensity == -1)
	{
		printf("missing colour density\n");
		notAllArgumentsGiven = 1;
	}
	if (maxSteps == -1)
	{
		printf("missing max step count\n");
		notAllArgumentsGiven = 1;
	}
	if (notAllArgumentsGiven)
	{
		getchar();
		return 0;
	}

	//check to make sure overlay fits
	if (size % overlaySize)
	{
		printf("overlay does not fit\n");
		getchar();
		return 0;
	}

	//check for seed then seed
	if (!seedGiven)
	{
		seed = time(NULL);
	}
	srand(seed);
	
	//BUILD DATA STRUCTS
	fullSectors = (size / BITCOUNT);
	partialSectorSize = size % BITCOUNT;

	if (!partialSectorSize)
	{
		partialSectorSize = 64;
		fullSectors--;
	}
	
	grid = (sector_t **)malloc(sizeof(sector_t *) * size); 
	subSums = (subSum_t **)malloc(sizeof(subSum_t *) * size);
	for (i = 0; i < size; i++)
	{
		grid[i] = (sector_t*)malloc(sizeof(sector_t) * (fullSectors + 1));
		subSums[i] = (subSum_t *)malloc(sizeof(subSum_t) * (size / overlaySize));
	}

	blueTransfer = (unsigned long long **)malloc(sizeof(unsigned long long *) * threadCount);
	for (i = 0; i < threadCount; i++)
	{
		blueTransfer[i] = (unsigned long long *)malloc(sizeof(unsigned long long *) * (fullSectors + 1));
	}

	//initialize return
	returnVal.foundSomething = 0;
	returnVal.density = 0;

	//initialize the board randomly
	randomInitGrid(grid, size, fullSectors, partialSectorSize);
		

	//make threads
	threadArgs = (threadArgs_t *)malloc(sizeof(threadArgs_t) * threadCount);
	
	maxColour = overlaySize * overlaySize * cDensity / 100;
	workStart = 0;

	for (i = 0; i < threadCount; i++) {
		//set data for threads
		threadArgs[i].id = i;
		threadArgs[i].threadCount = threadCount;		
		threadArgs[i].maxSteps = maxSteps;
		threadArgs[i].maxColour = maxColour;

		threadArgs[i].exitCond = &exitCond;

		threadArgs[i].grid = grid;
		threadArgs[i].size = size;
		threadArgs[i].overlaySize = overlaySize;
		threadArgs[i].fullSectors = fullSectors;
		threadArgs[i].partialSectorSize = partialSectorSize;
		
		threadArgs[i].blueTransfer = blueTransfer;
		
		threadArgs[i].subSums = subSums;

		//assign work
		workEnd = workStart + size/threadCount;
		if (size % threadCount > i)
		{
			workEnd++;
		}
		threadArgs[i].workStart = workStart;
		threadArgs[i].workEnd = workEnd;
		workStart = workEnd;
    }

	omp_set_num_threads(threadCount);
	StartTime();
	#pragma omp parallel 
	{
		//printf("I am thread %d\n",omp_get_thread_num());
		shiftThread(&threadArgs[omp_get_thread_num()]);
	}
	simTime = EndTime();

	//find highest density
	endDensity = 0;
	for (i = 0; i < threadCount; i++)
	{
		if (endDensity < threadArgs[i].highestDensity)
		{
			endDensity = threadArgs[i].highestDensity;
		}
	}
	//convert to percent
	endDensity = endDensity * 100 / (overlaySize * overlaySize);
	
	//output
	outputFile = fopen("redblue.txt","w");
	printGrid(grid, size, fullSectors, partialSectorSize, outputFile);
	fprintf(outputFile, "p%d b%d t%d c%d m%d s%d Iterations: %d density %d Timer: %lf", threadCount, size, overlaySize, cDensity, maxSteps, seed, threadArgs[0].maxStepsTaken, endDensity, simTime);
	fclose(outputFile);

	printf("p%d b%d t%d c%d m%d s%d Iterations: %d density %d Timer: %lf", threadCount, size, overlaySize, cDensity, maxSteps, seed, threadArgs[0].maxStepsTaken, endDensity, simTime);

	//free data
	for (i = 0; i < size; i++)
	{
		free(subSums[i]);
		free(grid[i]);
	}
	free(grid);
	free(subSums);


	for (i = 0; i < threadCount; i++)
	{
		free(blueTransfer[i]);
	}
	free(blueTransfer);
	free(threadArgs);
	
	getchar();
	return 0;
}