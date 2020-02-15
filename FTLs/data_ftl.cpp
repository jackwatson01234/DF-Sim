/* Copyright 2011 Matias Bjørling */

/* page_ftl.cpp  */

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

/* Implements a very simple page-level FTL without merge */

#include <new>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "../ssd.h"

using namespace ssd;
//////////////////////////////////////////////////////////////////////////

struct Pages
{
	enum page_state state;
	char data;
};
struct Blocks
{
	int empty;
	int valid;
	int invalid;
	int erase;
	bool is_rs;
};

struct Planes
{	
	int erase;
	bool bit_map;
	int full;
};
struct Dies
{	
	int erase;
};
struct Packages
{	
	int erase;
};

Packages *package;
Dies *die;
Planes *plane;
Blocks *block;
Pages *page;


int *MT;
int free_pages;
int *rs_blocks;
int rs_number;

int page_number; 
int block_number; 
int plane_number; 
int die_number; 
int package_number;

float GC_threshold = 0.25;
int last_block;

/////////////////////////////////////////////////////////////////////////

FtlImpl_DATAftl::FtlImpl_DATAftl(Controller &controller):
	FtlParent(controller)
{
	trim_map = new bool[NUMBER_OF_ADDRESSABLE_BLOCKS * BLOCK_SIZE];
	/////////////////////////////////////////////////////////////////////
	page_number = (SSD_SIZE * PACKAGE_SIZE * DIE_SIZE * PLANE_SIZE * BLOCK_SIZE) / VIRTUAL_PAGE_SIZE;
	block_number = (SSD_SIZE * PACKAGE_SIZE * DIE_SIZE * PLANE_SIZE) / VIRTUAL_PAGE_SIZE;
	plane_number = (SSD_SIZE * PACKAGE_SIZE * DIE_SIZE) / VIRTUAL_PAGE_SIZE;
	die_number = (SSD_SIZE * PACKAGE_SIZE) / VIRTUAL_PAGE_SIZE;
	package_number = SSD_SIZE / VIRTUAL_PAGE_SIZE;
	/////////////////////////////////////////////////////////////////////
	page = new Pages[page_number];	
	for(int i = 0; i < page_number; i++)
	{
		page[i].state = EMPTY;
		page[i].data = '-';
		printf("%c | \n",page[i].data);
		printf("\n");
	}

	int counter = 0;	
	for(int k = 0; k < NUMBER_OF_ADDRESSABLE_BLOCKS * BLOCK_SIZE; k++)
	{
		if (k > 9) {
		printf("%d[%c][%d]  " , k, page[k].data, page[k].state);
		}
		else
		{
			printf("%d [%c][%d]  " , k, page[k].data, page[k].state);
		}
		counter ++;
		if (counter == 4) {
			printf(" \n");
			counter = 0;
		}
	}
	printf("---------------------------------------------\n");
	/////////////////////////////////////////////////////////////////////
	block = new Blocks[block_number];
	for(int i = 0; i < block_number; i++)
	{
		block[i].empty = BLOCK_SIZE;
		block[i].valid = 0;
		block[i].invalid = 0;
		block[i].erase = 0;
		block[i].is_rs = false;
	}
	/////////////////////////////////////////////////////////////////////
	plane = new Planes[plane_number];
	for(int i = 0; i < plane_number; i++)
	{
		plane[i].bit_map = 0;
		plane[i].erase = 0;
		plane[i].full = PLANE_SIZE;
	}
	/////////////////////////////////////////////////////////////////////
	die = new Dies[die_number];
	for(int i = 0; i < die_number; i++)
	{
		die[i].erase = 0;
	}
	/////////////////////////////////////////////////////////////////////
	package = new Packages[package_number];
	for(int i = 0; i < package_number; i++)
	{
		package[i].erase = 0;
	}
	/////////////////////////////////////////////////////////////////////
	int x = BLOCK_SIZE * plane_number * (PLANE_SIZE - (PLANE_SIZE * GC_threshold) - 1);
	MT = new int[x];
	printf("====== %d\n", x);
	for(int i = 0; i < x; i++)
	{
		MT[i] = -1;
	}
	/////////////////////////////////////////////////////////////////////
	int y = PLANE_SIZE * GC_threshold;
	for(int i = 0; i < plane_number; i++)
	{
		int m = PLANE_SIZE - 1;
		for(int j = 0; j < y; j++)
		{
			block[i * PLANE_SIZE + m].is_rs = true;
			m --;
			plane[i].full --;
		}
	}
	/////////////////////////////////////////////////////////////////////
	last_block = 0;

	printf("------ start -----\n");
	/////////////////////////////////////////////////////////////////////
	return;
}

FtlImpl_DATAftl::~FtlImpl_DATAftl(void)
{
	return;
}

enum status FtlImpl_DATAftl::read(Event &event)
{
	event.set_address(Address(0, PAGE));
	event.set_noop(true);

	if (MT[event.get_logical_address()] != -1) {
		int ppn = MT[(int)event.get_logical_address()] ;
		printf("data is : %c\n", page[ppn].data);
	}
	else
	{
		printf("--------------there is no data in this lpn!\n");
	}

	controller.stats.numFTLRead++;
	return controller.issue(event);
}

enum status FtlImpl_DATAftl::write(Event &event)
{
	event.set_address(Address(1, PAGE));
	event.set_noop(true);

	int ppn;
	
	assert(event.get_logical_address() < (NUMBER_OF_ADDRESSABLE_BLOCKS - rs_number - 1) * BLOCK_SIZE);
	if (MT[event.get_logical_address()] != -1)
	{	
		printf("---------------------------------------------if\n");
		ppn = find_free_page(event);
		printf("--- ppn : %d | MT:%d\n", ppn, MT[event.get_logical_address()]);
		assert(MT[event.get_logical_address()] >= 0);
		page[MT[event.get_logical_address()]].state = INVALID;
		printf("--- block[MT[event.get_logical_address()] / BLOCK_SIZE]  : %d\n", MT[event.get_logical_address()] / BLOCK_SIZE );
		block[MT[event.get_logical_address()] / BLOCK_SIZE].valid --;


		assert(block[MT[event.get_logical_address()] / BLOCK_SIZE].valid >= 0);


		block[MT[event.get_logical_address()] / BLOCK_SIZE].invalid ++;
		////////////////
		increase_bitmap(MT[event.get_logical_address()] / BLOCK_SIZE);
		////////////////
		page[ppn].data = event.get_data();
		page[ppn].state = VALID;		
		block[ppn / BLOCK_SIZE].empty --;
		printf("------------test ---------------- %d\n", ppn / BLOCK_SIZE);
		block[ppn / BLOCK_SIZE].valid ++;
		MT[event.get_logical_address()] = ppn;
	}
	else
	{
		printf("---------------------------------------------else\n");
		ppn = find_free_page(event);
		printf("--- ppn : %d \n", ppn);
		MT[event.get_logical_address()] = ppn;
		page[ppn].data = event.get_data();
		page[ppn].state = VALID;
		block[ppn / BLOCK_SIZE].empty --;
		block[ppn / BLOCK_SIZE].valid ++;
	}
	// increase_bitmap(ppn / BLOCK_SIZE);

	int counter = 0;	
	for(int k = 0; k < NUMBER_OF_ADDRESSABLE_BLOCKS * BLOCK_SIZE; k++)
	{
		if (k > 9) { printf("%d[%c][%d]  " , k, page[k].data, page[k].state); }
		else { printf("%d [%c][%d]  " , k, page[k].data, page[k].state); }
		counter ++;
		if (counter == 4) { 
			printf(" ---%d|empty:%d|valid:%d|invalid:%d|erase:%d|rs:%d", k/BLOCK_SIZE, block[k/BLOCK_SIZE].empty, block[k/BLOCK_SIZE].valid, block[k/BLOCK_SIZE].invalid, block[k/BLOCK_SIZE].erase, block[k/BLOCK_SIZE].is_rs);
			printf(" \n"); 
			counter = 0; 
			}
	}
	printf("---------------------------------------------\n");
	int x = BLOCK_SIZE * plane_number * (PLANE_SIZE - (PLANE_SIZE * GC_threshold) - 1);
	printf("------------------------------------------------------------------------------\n");
	for(int i = 0; i < x; i++)
	{
		printf("MT[%d]>%d | ", i , MT[i]);
	}
	printf("\n");
	printf("------------------------------------------------------------------------------\n");

	for (int i = 0; i < plane_number; i++)
	{
		printf("- %d[%d] | ", i , plane[i].bit_map);
	}

	printf("\n");
	

	controller.stats.numFTLWrite++;
	return controller.issue(event);
}
 
enum status FtlImpl_DATAftl::trim(Event &event)
{
	controller.stats.numFTLTrim++;
	return SUCCESS;
}

int FtlImpl_DATAftl::find_free_page(Event &event)
{
	int pbn = find_free_block(event);
	printf("---pbn:%d\n",pbn);
	for(int i = 0; i < BLOCK_SIZE; i++)
	{
		if (page[pbn * BLOCK_SIZE + i].state == EMPTY) {
			printf("--- page:%d\n", pbn * BLOCK_SIZE + i);
			return pbn * BLOCK_SIZE + i;
		}
	}
	return 0;
}

int FtlImpl_DATAftl::find_free_block(Event &event)
{
	printf("--- last_block:%d\n",last_block);
	//-------------------------------------------------------
	if (block[last_block].empty != 0 && block[last_block].is_rs == false) 
	{
		if (last_block == block_number) { last_block = 0; }		
		printf("insssssss | %d | %d\n", last_block, block[last_block].empty);
		return last_block;
	}
	else
	{
		printf("uuuuuuuuuuuuuu | %d\n", last_block);
		int tmp = last_block;
		int en = 0;
		while(block[tmp].empty == 0 || block[tmp].is_rs == true){ 
			printf("---tmp:%d | tmp_empty:%d | tmp_rs:%d | %d\n", tmp, block[tmp].empty,block[tmp].is_rs, plane[tmp/PLANE_SIZE].bit_map);
			
			/////////////////////////////////////////////
			
			// printf("--- block_number:%d\n",block_number);
			// printf("--- tmp:%d\n",tmp);
			//////////////////////////////////////////
			tmp ++;
			printf("--- block_number:%d\n",block_number);
			if (tmp == block_number) {
				tmp = 0;
			}
			
			printf("---tmp:%d | tmp_empty:%d | tmp_rs:%d | %d\n", tmp, block[tmp].empty,block[tmp].is_rs, plane[tmp/PLANE_SIZE].bit_map);
			if (tmp == last_block) { 
				select_planes(event, last_block / (PLANE_SIZE * DIE_SIZE));
			}
			assert(en < 300);
			en ++;
		}
		printf("--- %d \n", tmp);
		last_block = tmp;
		return last_block;
	}
}

void FtlImpl_DATAftl::select_planes(Event &event, int die_num)
{
	printf("------------------------------------select\n");
	// for(int i = 0; i < DIE_SIZE; i++)
	for(int i = 0; i < PACKAGE_SIZE * DIE_SIZE; i++)
	{
		printf("========= %d , bit_map : %d \n", i , plane[i].bit_map);
		// if (plane[DIE_SIZE * die_num + i].bit_map == 1) {
		if (plane[i].bit_map == 1) {
			// assert(0);
			printf("------------------- GC running up\n");
			// GarbageCollection(event, DIE_SIZE * die_num + i);
			GarbageCollection(event,i);
			// assert(0);
			// plane[DIE_SIZE * die_num + i].bit_map == 0;
			plane[i].bit_map == 0;
		}
	}
	return;
}

void FtlImpl_DATAftl::increase_bitmap(int pbn)
{
	int i = pbn / PLANE_SIZE;
	if (block[pbn].invalid != 0) 
	{
		plane[i].bit_map = 1;  
	}
	// if (block[pbn].empty == 0) { plane[i].full --; }
	// if (plane[i].full == 0) { plane[i].bit_map = 1; }
	printf("-----------incrase run up\n");
	return;//////////////////////////////////
}

void FtlImpl_DATAftl::GarbageCollection(Event &event, int plane_num)
{

	int rs_num = PLANE_SIZE * GC_threshold;
	int rs_arr[rs_num];


	int count = 0;
	for(int i = 0; i < PLANE_SIZE; i++)
	{
		if (block[(plane_num * PLANE_SIZE) + i].is_rs == true) {
			rs_arr[count] = (plane_num * PLANE_SIZE) + i;
			count ++;
		}	
	}
	//// new in today
	int arr_sort[PLANE_SIZE];
	// *arr_sort = find_erasable_block(plane_num);
	////////////////////////////////////////////////////
	for(int i = 0; i < PLANE_SIZE; i++)
	{
		arr_sort[i] = PLANE_SIZE * plane_num + i;
	}

	int min, temp;
	for(int i = 0; i < PLANE_SIZE; i++)
	{
		min = i;
		for(int j = 0; j < PLANE_SIZE; j++)
		{
			if (block[arr_sort[j]].invalid < block[arr_sort[min]].invalid)
			{
            	min = j;
			}
      		temp = arr_sort[i];
      		arr_sort[i] = arr_sort[min];
      		arr_sort[min] = temp;
		}	
	}
	////////////////////////////////////////////////////

	for (int i = 0; i < PLANE_SIZE; i++)
	{
		printf("- %d ", arr_sort[i]);
	}
	
	printf("\n");

	// assert(0);

	for(int i = 0; i < rs_num; i++)
	{
		// int eb = find_erasable_block(plane_num);
		int eb = arr_sort[i];
		printf("--- rs:%d --- eb:%d\n", rs_arr[i], eb);
		free_up_block(eb, rs_arr[i]);

		plane[plane_num].erase ++;
		plane[plane_num].full ++;
		////////////////////////////////////////// must be checked
		Event eraseEvent = Event(ERASE, event.get_logical_address(), 1, event.get_start_time(),'-');
	 	eraseEvent.set_address(Address(0, PAGE));
		
	 	if (controller.issue(eraseEvent) == FAILURE) printf("Erase failed");
		printf("---------- %f\n", eraseEvent.get_time_taken());
		event.incr_time_taken(eraseEvent.get_time_taken());
		controller.stats.numFTLErase++;
		printf("--- numFTLErase: %d\n", controller.stats.numFTLErase);
		/////////////////////////////////////////// added erase method
	}

	plane[plane_num].bit_map = 0;


	//
	int counter = 0;	
	for(int k = 0; k < NUMBER_OF_ADDRESSABLE_BLOCKS * BLOCK_SIZE; k++)
	{
		if (k > 9) { printf("%d[%c][%d]  " , k, page[k].data, page[k].state); }
		else { printf("%d [%c][%d]  " , k, page[k].data, page[k].state); }
		counter ++;
		if (counter == 4) { 
			printf(" ---%d|empty:%d|valid:%d|invalid:%d|erase:%d|rs:%d", k/BLOCK_SIZE, block[k/BLOCK_SIZE].empty, block[k/BLOCK_SIZE].valid, block[k/BLOCK_SIZE].invalid, block[k/BLOCK_SIZE].erase, block[k/BLOCK_SIZE].is_rs);
			printf(" \n"); 
			counter = 0; 
			}
	}
	printf("---------------------------------------------\n");
	int x = BLOCK_SIZE * plane_number * (PLANE_SIZE - (PLANE_SIZE * GC_threshold) - 1);
	printf("------------------------------------------------------------------------------\n");
	for(int i = 0; i < x; i++)
	{
		printf("MT[%d]>%d | ", i , MT[i]);
	}
	printf("\n");
	printf("------------------------------------------------------------------------------\n");
	
	return;
}

void FtlImpl_DATAftl::WearLeveling(void) // baraye baAd
{
	return;
}
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////           DO - NOT - USE : NEED TO FIX
////////////////////////////////////////////////////////////
int FtlImpl_DATAftl::find_erasable_block(int plane_num)
{
	int tmp[PLANE_SIZE];
	int tmp1[PLANE_SIZE];

	for(int i = 0; i < PLANE_SIZE; i++)
	{
		tmp[i] = PLANE_SIZE * plane_num + i;
	}
	*tmp1 = sort_blocks(tmp);

	for (int i = 0; i < PLANE_SIZE; i++)
	{
		printf("- %d ", tmp1[i]);
	}
	printf("\n");
	assert(0);
	return *tmp1;
}

// avalin khone bishtarin invalid ra dard
int FtlImpl_DATAftl::sort_blocks(int *arr)
{
	int min, temp;
	for(int i = 0; i < PLANE_SIZE; i++)
	{
		min = i;
		for(int j = 0; j < PLANE_SIZE; j++)
		{
			if (block[arr[j]].invalid < block[arr[min]].invalid)
			{
            	min = j;
			}
      		temp = arr[i];
      		arr[i] = arr[min];
      		arr[min] = temp;
		}	
	}
	return *arr;
}
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

void FtlImpl_DATAftl::free_up_block(int block_num, int rs)
{
	int temp = 0;
	printf("== block_num : %d | rs : %d\n", block_num, rs);
	for(int i = 0; i < BLOCK_SIZE; i++)
	{
		if (page[block_num * BLOCK_SIZE + i].state == VALID) {
			page[rs * BLOCK_SIZE + temp].state = VALID;
			page[rs * BLOCK_SIZE + temp].data = page[block_num * BLOCK_SIZE + i].data;

			// MT[event.get_logical_address()] = rs * BLOCK_SIZE + temp;
			// printf("== MT[%d] : %d\n", event.get_logical_address(), rs * BLOCK_SIZE + temp);

			int x = BLOCK_SIZE * plane_number * (PLANE_SIZE - (PLANE_SIZE * GC_threshold) - 1);

			for (int k = 0; k < x; k++)
			{
				if (MT[k] == block_num * BLOCK_SIZE + i)
				{
					MT[k] = rs * BLOCK_SIZE + temp;
					printf("== MT[%d] : %d\n", k, MT[k]);
					break;
				}
				
			}
			
			
			

			temp ++;
		}
		page[block_num * BLOCK_SIZE + i].state = EMPTY;
		page[block_num * BLOCK_SIZE + i].data = '-';
	}

	block[rs].valid = block[block_num].valid;
	block[rs].empty = BLOCK_SIZE - block[rs].valid;
	block[rs].is_rs = false;

	// for(int i = 0; i < block_number; i++)
	// {
	// 	printf("======================= %d\n", i);
	// 	printf("======================= %d\n", MT[i] / BLOCK_SIZE);
	// 	if (MT[i] / BLOCK_SIZE == block_num) { 
	// 		MT[i] = rs; 
	// 		printf("------ MT is updated!\n"); 
	// 	}
	// }

	block[block_num].empty = BLOCK_SIZE;
	block[block_num].valid = 0;
	block[block_num].invalid = 0;
	block[block_num].erase ++;
	block[block_num].is_rs = true;

	// int counter = 0;	
	// for(int k = 0; k < NUMBER_OF_ADDRESSABLE_BLOCKS * BLOCK_SIZE; k++)
	// {
	// 	if (k > 9) { printf("%d[%c][%d]  " , k, page[k].data, page[k].state); }
	// 	else { printf("%d [%c][%d]  " , k, page[k].data, page[k].state); }
	// 	counter ++;
	// 	if (counter == 4) { 
	// 		printf(" ---%d|empty:%d|valid:%d|invalid:%d|erase:%d|rs:%d", k/BLOCK_SIZE, block[k/BLOCK_SIZE].empty, block[k/BLOCK_SIZE].valid, block[k/BLOCK_SIZE].invalid, block[k/BLOCK_SIZE].erase, block[k/BLOCK_SIZE].is_rs);
	// 		printf(" \n"); 
	// 		counter = 0; 
	// 		}
	// }
	// printf("---------------------------------------------\n");
	
	return;
}








