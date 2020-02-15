/* Copyright 2011 Matias Bjørling */

/* dftp_ftl.cpp  */

/* FlashSim is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version. */

/* FlashSim is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details. */

/* You should have received a copy of the GNU General Public License
 * along with FlashSim.  If not, see <http://www.gnu.org/licenses/>. */

/****************************************************************************/

/* Runtime information for the SSD Model
 */

#include <new>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <iostream> 
#include "ssd.h"

using namespace ssd;

MT_Stats::MT_Stats()
{
	MT_reset();
}  
void MT_Stats::MT_reset()
{
	
	numMTRead = 0;
	numMTWrite = 0;
	numMTErase = 0;
	PPN_USED = 0;
}

void MT_Stats::MT_reset_statistics()
{
	MT_reset();
}

void MT_Stats::MT_print_statistics()
{
	printf("MT_Statistics:\n");
	printf("-----------\n");
	printf("MT Reads: %li\t Writes: %li\t Erases: %li\n", numMTRead, numMTWrite, numMTErase);
	printf("MT PPN_USED: %li\n", PPN_USED);
	// printf("GC  Reads: %li\t Writes: %li\t Erases: %li\n", numGCRead, numGCWrite, numGCErase);
	// printf("WL  Reads: %li\t Writes: %li\t Erases: %li\n", numWLRead, numWLWrite, numWLErase);
	// printf("Log FTL Switch: %li Partial: %li Full: %li\n", numLogMergeSwitch, numLogMergePartial, numLogMergeFull);
	// printf("Page FTL Convertions: %li\n", numPageBlockToPageConversion);
	// printf("Cache Hits: %li Faults: %li Hit Ratio: %f\n", numCacheHits, numCacheFaults, (double)numCacheHits/(double)(numCacheHits+numCacheFaults));
	// printf("Memory Consumption:\n");
	// printf("Tranlation: %li Cache: %li\n", numMemoryTranslation, numMemoryCache);
	// printf("Reads: %li \tWrites: %li\n", numMemoryRead, numMemoryWrite);
	printf("-----------\n");
}
 