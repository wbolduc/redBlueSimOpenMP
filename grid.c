/*
william bolduc 0851313
grid.c
contains the all the grid making, loading and printing functions
*/
#include <stdlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "grid.h"

void randomInitGrid(sector_t **grid, int size, int fullSectors, int partialSectorSize)
{
	int i, j, k, colour;

	//loop through rows
	for (i = 0; i < size; i++)
	{
		//load full sectors
		for (j = 0; j < fullSectors; j++)
		{
			grid[i][j].R = 0;
			grid[i][j].B = 0;
			for(k = 0; k < BITCOUNT - 1; k++)
			{
				colour = rand() % 3;
				switch(colour)
				{
				case 1:
					grid[i][j].R |= 1;
					break;
				case 2:
					grid[i][j].B |= 1;
					break;
				case 0:
					break;
				}
				grid[i][j].R <<= 1;
				grid[i][j].B <<= 1;
			}

			//load last colour in this sector
			colour = rand() % 3;
			switch(colour)
			{
			case 1:
				grid[i][j].R |= 1;
				break;
			case 2:
				grid[i][j].B |= 1;
				break;
			case 0:
				break;
			}
		}
		
		//load partial sector
		grid[i][j].R = 0;
		grid[i][j].B = 0;
		for(k = 0; k < partialSectorSize - 1; k++)
		{
			colour = rand() % 3;
			switch(colour)
			{
			case 1:
				grid[i][j].R |= 1;
				break;
			case 2:
				grid[i][j].B |= 1;
				break;
			case 0:
				break;
			}
			grid[i][j].R <<= 1;
			grid[i][j].B <<= 1;
		}
		//load last colour in partial sector
		colour = rand() % 3;
		switch(colour)
		{
		case 1:
			grid[i][j].R |= 1;
			break;
		case 2:
			grid[i][j].B |= 1;
			break;
		case 0:
			break;
		}
		grid[i][j].R <<= (BITCOUNT - partialSectorSize);
		//Pad a B to stop reds shifting in partial sector
		grid[i][j].B <<= 1;
		grid[i][j].B |= 1;		
		grid[i][j].B <<= (BITCOUNT - partialSectorSize - 1);
	}
}
/*
void initBlueTest(cellLine_t ** grid, int fullSectors, int size, int partialSectorSize)
{
	int i, j;
	for (i = 0; i < 3; i++)
	{
		//load full sectors
		for (j = 0; j < fullSectors; j++)
		{
			grid[i][j].R = 0x0000001000000000;
			grid[i][j].B = 0xFFFFFFEFFFFFFFFF;
		}
		
		//load partial sector
		grid[i][j].R = 0x0000000000000010;
		grid[i][j].B = 0xFFFFFFFFFFFFFFEF;
		grid[i][j].R <<= (BITCOUNT - partialSectorSize);
		grid[i][j].B <<= (BITCOUNT - partialSectorSize);
	}
	for(i; i < size; i++)
	{
		for (j = 0; j < fullSectors + 1; j++)
		{
			grid[i][j].R = 0;
			grid[i][j].B = 0;
		}
	}
}
*/
void printGrid(sector_t **grid, int size, int fullSectors, int partialSectorSize, FILE *out)
{
	int i,j;

	for (i = 0; i < size; i++)
	{
		//print full sectors
		for(j = 0; j < fullSectors; j++)
		{
			printCells(grid[i][j], BITCOUNT, out);
		}
		//print partial sectors
		printCells(grid[i][j], partialSectorSize, out);
		fprintf(out, "\n");
	}
}

void printBits(unsigned long long x)
{
	int bytes;
	unsigned char *b = (unsigned char *)&x;
	int i, j;

	bytes = sizeof(x);
	for (i = bytes - 1; i >= 0; i--)
	{
		for (j = 7; j >= 0; j--)
		{
			printf("%u", b[i] >> j & 0x1);
		}
	}
	printf("|\n");
}

void printCellBits(sector_t sector)
{
	printBits(sector.R);
	printBits(sector.B);
}

void printCells(sector_t sector, int partialSectorSize, FILE * out)
{
	int bytes;
	int i, j;
	unsigned char *b = (unsigned char *)&sector.B;
	unsigned char *r = (unsigned char *)&sector.R;
	
	if (!partialSectorSize)
	{
		return;
	}

	bytes = sizeof(sector.B);

	for (i = bytes - 1; i >= 0; i--)
	{
		for (j = 7; j >= 0; j--)
		{
			if (b[i] >> j & 0x1)
			{
				fprintf(out, "V");
			}
			else if (r[i] >> j & 0x1)
			{
				fprintf(out, ">");
			}
			else
			{
				fprintf(out, " ");
			}

			if(!--partialSectorSize)
			{
				return;
			}
		}
	}
}

sector_t loadColours(char line[])
{
	int i, max;
	sector_t bitLine;
	
	bitLine.R = 0;
	bitLine.B = 0;

	max = strlen(line);

	if (max > BITCOUNT)
	{
		max = BITCOUNT;
	}
	
	for(i = 0; i < max; i++)
	{
		if (line[i] == 'B')
		{
			bitLine.B |= 0X1;
		}
		else if (line[i] == 'R')
		{
			bitLine.R |= 0X1;
		}
		
		if (i < max - 1)
		{
			bitLine.B <<= 1;
			bitLine.R <<= 1;
		}
	}
	bitLine.B <<= BITCOUNT - max;
	bitLine.R <<= BITCOUNT - max;
	return bitLine;
}