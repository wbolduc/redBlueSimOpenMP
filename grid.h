/*
william bolduc 0851313
grid.h
contains the all the grid making, loading and printing function definitions
also holds some useful 64 bit macros, in theory you could port this to 32 bit by changing these
*/
#ifndef GRID_H
#define GRID_H
#define BITCOUNT 64
#define BITCOUNTMINUS1 63
#define RIGHTMOSTBIT 0x0000000000000001
#define LEFTMOSTBIT 0x8000000000000000
#define INVRIGHTMOSTBIT 0xFFFFFFFFFFFFFFFE
#define ALL1S 0xFFFFFFFFFFFFFFFF
#include <stdio.h>

typedef struct _sector_t{
	unsigned long long R;
	unsigned long long B;
}sector_t;

void randomInitGrid(sector_t **grid, int size, int fullSectors, int partialSectorSize);

void printGrid(sector_t **grid, int size, int fullSectors, int partialSectorSize, FILE *out);

void printBits(unsigned long long x);

void printCellBits(sector_t sector);

void printCells(sector_t sector, int partialSectorSize, FILE *out);

sector_t loadColours(char line[]);

#endif /*GRID_H*/