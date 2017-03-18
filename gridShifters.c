/*
william bolduc 0851313
gridShifters.c
contains the all the grid shifting and checking code
*/

#include "gridShifters.h"

void redShift(sector_t *sectors, int fullSectors, int partialSectorSize)
{	//ONLY WORKS WITH BLUE PADDING ON END OF PARTIAL SECTOR
	int i;

	register unsigned long long sector0Coverage;
	register unsigned long long currCopyR;
	register unsigned long long currCoverage;

	register unsigned long long nextCoverage;
	
	register unsigned long long lineR;

	register unsigned long long currB;
	register unsigned long long currR;

	register unsigned long long nextB;
	register unsigned long long nextR;

	register unsigned long long canMove;

	register unsigned long long outR;
	register unsigned long long inR = 0;

	currB = (sectors[0]).B;		//I wonder if calling global sector twice is bad, since they are so close anyways
	currR = (sectors[0]).R;
	
	sector0Coverage = currB | currR;
	currCoverage = sector0Coverage;

	//do full shifts
	i = 0;
	for (i; i < fullSectors; i++)
	{
		//copy this line
		currCopyR = currR;

		//shift above line to line up with it's proceeding chars
		lineR = currR >> 1;
		//shift in bits
		lineR |= inR;

		//find which cells can Move
		canMove = lineR & ~(currCoverage);
		//add red cells to their moved locations
		currR = currR | canMove;
		//remove red cells from line
		currR &= ~(canMove << 1);

		//remove bit from this sector if the next one has space
		nextR = sectors[i+1].R;
		nextB = sectors[i+1].B;
		nextCoverage = nextR | nextB;

		currR &= ~(RIGHTMOSTBIT & currCopyR & ~(nextCoverage >> BITCOUNTMINUS1));

		//store bit to move if necessary
		inR = ((currCopyR << BITCOUNTMINUS1) & ~nextCoverage) & LEFTMOSTBIT;

		//update lines
		(sectors[i]).R = currR;

		currB = nextB;
		currR = nextR;
		currCoverage = nextCoverage;
	}
	
	// shift on partial sector
	currB = (sectors[i]).B;
	currR = (sectors[i]).R;
	currCopyR = currR; //stores the last bit, useful later

	//align lineR
	lineR = currR >> 1;
	//shift in bits
	lineR |= inR;

	//find which cells can Move
	canMove = lineR & ~(currB | currR);
	//add red cells to their moved locations
	currR = currR | canMove;

	//remove red cells from line
	currR &= ~(canMove << 1);

	//wrap around to 0th sector
	//remove last bit if needed
	sectors[i].R = currR & ~(((~sector0Coverage) >> (partialSectorSize - 1)) & currCopyR);
	//add first bit if needed
	sectors[0].R |= ((currCopyR << (partialSectorSize - 1)) & ~sector0Coverage)& LEFTMOSTBIT;
}

void doRedShifts(sector_t ** grid, int fullSectors, int partialSectorSize, int start, int end)
{
	int i;
	for (i = start; i < end; i++)
	{
		redShift(grid[i], fullSectors, partialSectorSize);
	}
}

void blueShifts(sector_t ** grid, int totalSectors, int start, int end, unsigned long long *blueTransfer, unsigned long long *lastMove)	//movelist is only passed to avoid superfluous mallocs
{
	int i, j;
	
	unsigned long long l1B;
	unsigned long long l1R;
	unsigned long long l2B;
	unsigned long long l2R;
	unsigned long long canMove;

	//move each row at a time
	//do first row, store it's cover in blueTransfer
	
	for(j = 0; j < totalSectors; j++)
	{
		l1B = grid[start][j].B;
		l1R = grid[start][j].R;

		l2B = grid[start+1][j].B;
		l2R = grid[start+1][j].R;	

		//store first row in blue transfer
		blueTransfer[j] = l1B | l1R;

		//find which cells can Move
		canMove = l1B & ~(l2B | l2R);

		//remove blue cells from line 1
		l1B = l1B & ~canMove;
		//add blue cells to line 2
		l2B = l2B | canMove;

		lastMove[j] = canMove;
		grid[start][j].B = l1B;
		grid[start+1][j].B = l2B;
	}

	end--;
	for(i = start + 1; i < end; i++)
	{
		for(j = 0; j < totalSectors; j++)
		{
			l1B = grid[i][j].B;

			l2B = grid[i+1][j].B;
			l2R = grid[i+1][j].R;

			//find which cells can move
			canMove = (l1B & ~lastMove[j]) & ~(l2B | l2R);

			//remove blue cells from line 1
			l1B = l1B & ~canMove;

			//add blue cells to line 2
			l2B = l2B | canMove;

			lastMove[j] = canMove;
			grid[i][j].B = l1B;
			grid[i+1][j].B = l2B;
		}
	}
}

void stitchBlueShifts(sector_t ** grid, int totalSectors, int lastLine, int nextLine, unsigned long long *blueTransfer, unsigned long long *lastMove)
{
	int j;
	register unsigned long long canMove;

	for(j = 0; j < totalSectors; j++)
	{
		canMove = grid[lastLine][j].B & ~lastMove[j] & ~blueTransfer[j];
		grid[lastLine][j].B &= ~canMove;
		grid[nextLine][j].B |= canMove;
	}
}

int totalSubSums(subSum_t **subSums, int id, int threadCount, int overlaySize, int overlayCount, int overlays)
{
	int i,j,k;
	int sumB;
	int sumR;
	int overlayX;
	int overlayY;
	int overlayYEnd;

	int biggestDensity = 0;

	for (i = id; i < overlayCount; i += threadCount)
	{
		overlayX = i % overlays;
		overlayY = (i / overlays) * overlaySize;
		overlayYEnd = overlayY + overlaySize;
		sumB = 0;
		sumR = 0;
		
		for(overlayY; overlayY < overlayYEnd; overlayY++)
		{
			sumB += subSums[overlayY][overlayX].subSumB;
			sumR += subSums[overlayY][overlayX].subSumR;
		}

		if(biggestDensity < sumR)
		{
			biggestDensity = sumR;	
		}
		if (biggestDensity < sumB)
		{
			biggestDensity = sumB;
		}
	}
	return biggestDensity;
}

void countSubSums(sector_t **grid, int size, int overlaySize, int start, int end, subSum_t **subSums)
{	//check this for muliples and divisors of 64
	int i,j;
	
	int midSector;

	int oLStart;
	
	int startSector;
	int displacement;

	int endSector;
	int remaining;

	int overlayCount = size / overlaySize;

	unsigned long long frontMask;
	unsigned long long rearMask;

	if (overlaySize == BITCOUNT)
	{
		for ( i = start; i < end; i++)
		{
			for (j = 0; j < overlayCount; j++)
			{
				subSums[i][j].subSumB = __popcnt64(grid[i][j].B);
				subSums[i][j].subSumR = __popcnt64(grid[i][j].R);
			}
		}
	}
	else if (overlaySize > BITCOUNT)
	{
		for ( i = start; i < end; i++)
		{
			startSector = 0;
			displacement = 0;
			for (j = 0, oLStart = overlaySize; oLStart <= size; j++, oLStart +=overlaySize)
			{
				remaining = oLStart % BITCOUNT;
				endSector = (oLStart / BITCOUNT) - (remaining == 0); //The condition additions are big ass patches

				frontMask = 0xFFFFFFFFFFFFFFFF >> displacement;
				rearMask = 0xFFFFFFFFFFFFFFFF << (BITCOUNT - remaining);	//could be fucky
				
				subSums[i][j].subSumB = __popcnt64(frontMask & grid[i][startSector].B);
				subSums[i][j].subSumR = __popcnt64(frontMask & grid[i][startSector].R);

				//loop though fullSectors
				for(midSector = startSector + 1; midSector < endSector; midSector++)
				{
					subSums[i][j].subSumB += __popcnt64(grid[i][midSector].B);
					subSums[i][j].subSumR += __popcnt64(grid[i][midSector].R);
				}
				
				//look at last sector
				subSums[i][j].subSumB += __popcnt64(rearMask & grid[i][endSector].B);
				subSums[i][j].subSumR += __popcnt64(rearMask & grid[i][endSector].R);

				startSector = endSector + (remaining == 0);
				displacement = remaining;
			}
		}
	}
	else
	{
		//overlay is small
		for ( i = start; i < end; i++)
		{
			startSector = 0;
			displacement = 0;
			for (j = 0, oLStart = overlaySize; oLStart <= size; j++, oLStart +=overlaySize)
			{
				endSector = oLStart / BITCOUNT;
				remaining = oLStart % BITCOUNT;

				frontMask = ALL1S >> displacement;
				rearMask = ALL1S << (BITCOUNT - remaining);	//could be fucky

				if(startSector == endSector)
				{
					subSums[i][j].subSumB = __popcnt64(frontMask & rearMask & grid[i][startSector].B);
					subSums[i][j].subSumR = __popcnt64(frontMask & rearMask & grid[i][startSector].R);
				}
				else
				{
					subSums[i][j].subSumB = __popcnt64(frontMask & grid[i][startSector].B);
					subSums[i][j].subSumR = __popcnt64(frontMask & grid[i][startSector].R);
					subSums[i][j].subSumB += __popcnt64(rearMask & grid[i][endSector].B);
					subSums[i][j].subSumR += __popcnt64(rearMask & grid[i][endSector].R);
				}
			
				startSector = endSector;
				displacement = remaining;
			}
		}
	}
}

void shiftThread(threadArgs_t *args)
{
	int id = args->id;
	int threadCount = args->threadCount;
	int maxSteps = args->maxSteps;

	int maxColour = args->maxColour;

	int sum;
	int* exitCond = args->exitCond;

	sector_t **grid = args->grid;
	int fullSectors = args->fullSectors;
	int partialSectorSize = args->partialSectorSize;
	int totalSectors = fullSectors + 1;
	
	int size = args->size;
	int overlaySize = args->overlaySize;
	int overlayDimension = size / overlaySize;
	int overlayCount = overlayDimension * overlayDimension;
	
	unsigned long long **blueTransfers = args->blueTransfer;
	subSum_t **subSums = args->subSums;

	int start = args->workStart;
	int end = args->workEnd;
	int density;
	int highestDensity = 0;

	int j,i,step;

	//for blue shifts
	int nextRow;
	int lastRow;
	int blueTransferRow;

	unsigned long long *lastMove;
	unsigned long long canMove;
	//pthread_barrier_t *barrier = args->barrier;

	lastMove = (unsigned long long *)malloc(sizeof(unsigned long long) * totalSectors);

	//wrap around
	lastRow = end - 1;		
	if (threadCount - 1 == id)
	{
		blueTransferRow = 0;
		nextRow = 0;
	}
	else
	{
		blueTransferRow = id + 1;
		nextRow = end;
	}
	
	for (step = 0; step < maxSteps && !*exitCond; step++)
	{
		//Do red shifts
		doRedShifts(grid, fullSectors, partialSectorSize, start, end);
		#pragma omp barrier
		
		//do blue shifts
		blueShifts(grid, totalSectors, start, end, blueTransfers[id], lastMove);
		#pragma omp barrier
		
		//stitch blue transfers together
		stitchBlueShifts(grid,totalSectors,lastRow, nextRow, blueTransfers[blueTransferRow], lastMove);		
		#pragma omp barrier
		
		//find sub sums
		countSubSums(grid, size, overlaySize, start, end, subSums);
		#pragma omp barrier

		if(maxColour < (density = totalSubSums(subSums, id, threadCount, overlaySize, overlayCount, overlayDimension)))
		{
			(*exitCond)++;
		}
		else if (density > highestDensity)
		{
			highestDensity = density;
		}
		#pragma omp barrier
	}

	args->highestDensity = highestDensity;
	args->maxStepsTaken = step;

	free(lastMove);
}