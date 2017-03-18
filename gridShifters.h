/*
william bolduc 0851313
gridShifters.h
struct and function definitions for grid shifting and checking code
*/

#include <stdio.h>
#include "grid.h"
#include <nmmintrin.h>


typedef struct _subSum_t{
	int subSumB;
	int subSumR;
}subSum_t;

typedef struct _threadReturn_t{
	int foundSomething;
	
	int overlayX;
	int overlayY;
	int density;
	int stepsRun;
}threadReturn_t;

typedef struct _threadArgs_t{
	int id;
	int threadCount;
	int maxSteps;
	int maxColour;

	int workStart;
	int workEnd;
	int *exitCond;

	sector_t **grid;

	//sectors are the longs which store the colours, the last one may not be full 
	int fullSectors;
	int partialSectorSize;

	//basically ySize
	int size;
	int overlaySize;

	subSum_t **subSums;
	unsigned long long **blueTransfer;

	int highestDensity;
	int maxStepsTaken;
}threadArgs_t;

void redShift(sector_t *sectors, int fullSectors, int partialSectorSize);

void doRedShifts(sector_t ** grid, int fullSectors, int partialSectorSize, int start, int end);

void blueShifts(sector_t ** grid, int totalSectors, int start, int end, unsigned long long *blueTransfer, unsigned long long *lastMove);

void stitchBlueShifts(sector_t ** grid, int totalSectors, int lastLine, int nextLine, unsigned long long *blueTransfer, unsigned long long *lastMove);

int totalSubSums(subSum_t **subSums, int id, int threadCount, int overlaySize, int overlayCount, int overlays);

void countSubSums(sector_t **grid, int size, int overlaySize, int start, int end, subSum_t **subSums);

void shiftThread(threadArgs_t *args);
