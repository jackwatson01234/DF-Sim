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

// store state and data for each page 
// state => 0:empty 1:valid 2:invalid
// data => a character that recive from controller
// struct Pages
// {
// 	enum page_state state; 
// 	char data;
// };

// // store number of emttp and valid page of each block 
// struct Blocks
// {
// 	int empty;
// 	int valid;
// };
// // catch struct that keep data and index of that ppn
// struct CacheMTs
// {
// 	int index;
// 	char data;
// };

// create object of block and page struct to use az an array
// Blocks *block;
// Pages *page;
// CacheMTs *cachemt;


// int *MT; // definition of mapping table
// int last_page; // a flag for keeping last number of page that used

// int page_number; // number of pages in ssd
// int block_number; // number of block in ssd
// int plane_number; // number of plane in ssd
// int die_number; // number of die in ssd
// int package_number; //number of package in ssd

/////////////////////////////////////////////////////////////////////////

FtlImpl_NEWftl::FtlImpl_NEWftl(Controller &controller):
	FtlParent(controller)
{
	trim_map = new bool[NUMBER_OF_ADDRESSABLE_BLOCKS * BLOCK_SIZE];
	/////////////////////////////////////////////////////////////////////
	//calculation of page, block, plane, die, package number
	page_number = (SSD_SIZE * PACKAGE_SIZE * DIE_SIZE * PLANE_SIZE * BLOCK_SIZE) / VIRTUAL_PAGE_SIZE;
	block_number = (SSD_SIZE * PACKAGE_SIZE * DIE_SIZE * PLANE_SIZE) / VIRTUAL_PAGE_SIZE;
	plane_number = (SSD_SIZE * PACKAGE_SIZE * DIE_SIZE) / VIRTUAL_PAGE_SIZE;
	die_number = (SSD_SIZE * PACKAGE_SIZE) / VIRTUAL_PAGE_SIZE;
	package_number = SSD_SIZE / VIRTUAL_PAGE_SIZE;
	/////////////////////////////////////////////////////////////////////
	//create page array and initialize it
	page = new Pages[page_number];	
	for(int i = 0; i < page_number; i++)
	{
		page[i].state = EMPTY;
		page[i].data = '-';
	}
	/////////////////////////////////////////////////////////////////////
	//create block array and initialize it
	block = new Blocks[block_number];
	for(int i = 0; i < block_number; i++)
	{
		block[i].empty = BLOCK_SIZE;
		block[i].valid = 0;

	}
	
	switch (MT_IMPL)
	{
		case 1: //direct map
			MT = new int[LOGIC_NUMBER];
			for(int i = 0; i < (int) LOGIC_NUMBER; i++)
			{
				MT[i]= -1;

			}
			
			break;
		case 2:

			break;
		default:
			break;
	}
	cachemt = new CacheMTs[CACHEMT_SIZE];
	for (int i = 0; i < (int) CACHEMT_SIZE; i++)
	{
		cachemt[i].data = '-';
		cachemt[i].index = -1;
	}
	

	last_page = 0;	
	printf("------ start -----\n");
	/////////////////////////////////////////////////////////////////////
	return;
} 

FtlImpl_NEWftl::~FtlImpl_NEWftl(void)
{
	return;
}

enum status FtlImpl_NEWftl::read(Event &event)
{
	event.set_address(Address(0, PAGE));
	event.set_noop(true);
	
	// checking for data validation in ssd, MT must be have a ppn for that 
	if (MT[event.get_logical_address()] != -1) {
		int ppn = MT[(int)event.get_logical_address()] ;
		printf("data is : %c\n", page[ppn].data);
		// read data from page array and show it	
	}
	else
	{
		printf("--------------there is no data in this lpn!\n");
	}
	// add number of reading from mapping table
	// controller.MTStats.numMTRead++;
	controller.stats.numFTLRead++;
	return controller.issue(event);
	
}

enum status FtlImpl_NEWftl::write(Event &event)
{
	event.set_address(Address(1, PAGE));
	event.set_noop(true);
	
	assert(event.get_logical_address() < LOGIC_NUMBER);
	// sreaching data in page array
	// if data is found, return number of page that this data store on it
	// if not return -1
	int index  = search_in_catch(event);
	if (index == -1)
	{
		printf("miss is happend!\n");
		int ppn = search_in_page(event);  /// ------------ ask that -------
		if (ppn != -1) {
			if (MT[event.get_logical_address()] != -1 && MT[event.get_logical_address()] == ppn) 
			{
				printf("===== this event is happend in the past XD !");
			}
			else{
				if (MT[event.get_logical_address()] != -1) 
				{ 
					controller.MTStats.numMTErase++; 
					printf("helllllllllo\n");
				}
				controller.issueRamWrite(event); //timing
				// adding to mapping table;
				printf("=== timing issueRamWrite===\n");
				MT[event.get_logical_address()] = ppn;
				controller.issueRamRead(event); //timing	
				// check if must be edited!!!!!!!
				printf("=== timing issueRamRead===\n");
				controller.MTStats.numMTWrite++; 
				printf("helllllllllo\n");
			}
		}
		else
		{
			// find a free page for storing new data and return ppn
			// after that mapping table must be set with new address 
			ppn = find_free_page(event);
			page[ppn].data = event.get_data();
			controller.issueBusLock(event, BUS_CTRL_DELAY + BUS_DATA_DELAY); //timing  /// this line must be check!
			printf("=== timing issueBusLock===\n");
			controller.issueWrite(event); //timing
			printf("=== timing issueWrite===\n");
			controller.stats.numFTLWrite++;
			// data in ppn --- must be added
			controller.MTStats.PPN_USED ++;
			if (MT[event.get_logical_address()] != -1) 
			{ 
				controller.MTStats.numMTErase++; 
				printf("helllllllllo\n");
			}
			controller.issueRamRead(event); //timing
			printf("=== timing issueRamRead===\n");
			MT[event.get_logical_address()] = ppn;
			controller.issueRamWrite(event); //timing
			printf("=== timing issueRamWrite===\n");
			controller.MTStats.numMTWrite++; 
		}
		// find row in catch mapping table that must be deleted!
		// must be added later	// catch_allocation();
		///////////////////////////////////////////////////////
		cachemt[ppn % CACHEMT_SIZE].data = event.get_data();
		cachemt[ppn % CACHEMT_SIZE].index = ppn;
		controller.issueRamWrite(event); //timing    /// how many is needed? 2 or 1?
		printf("=== timing issueRamWrite===\n");
		///////////////////////////////////////////////////////
	}
	else
	{
		if (MT[event.get_logical_address()] != -1 && MT[event.get_logical_address()] == index) 
			{
				printf("===== this event is happend in the past XD ! for index :)");
			}
			else{
				if (MT[event.get_logical_address()] != -1) 
				{ 
					controller.MTStats.numMTErase++; 
					printf("helllllllllo\n");
				}
				MT[event.get_logical_address()] = index;
				controller.MTStats.numMTWrite++; 
				controller.issueRamRead(event); //timing
				printf("=== timing issueRamRead===\n");
				printf("byyyyyyyyyyye\n");
			}
	}
	
	printf("\n------- CATCH ------\n");
	for (int i = 0; i < (int) CACHEMT_SIZE; i++)
	{
		printf(" %d[%c][%d] | ", i , cachemt[i].data, cachemt[i].index);
	}
	printf("\n------- Page ------\n");
	for (int i = 0; i < (int) CACHEMT_SIZE; i++)
	{
		printf(" %d[%c] | ", i , page[i].data);
	}
	printf("\n------ MT -------\n");
	for (int i = 0; i < (int) LOGIC_NUMBER; i++)
	{
		printf(" %d[%d] | ", i , MT[i]);
	}
	printf("\n");
	printf("-------------------------------------------------------------\n");

	// (void) event.incr_time_taken(read_delay * event.get_size());
	// controller.issueBusLock(event, BUS_CTRL_DELAY + BUS_DATA_DELAY);
	// controller.issueRamRead(event);
	// controller.issueRamWrite(event);
	// controller.issueWrite(event);
	// return controller.issue(event);
	return SUCCESS;
}
 
enum status FtlImpl_NEWftl::trim(Event &event)
{
	controller.stats.numFTLTrim++;
	return SUCCESS;
}

// return address of last free page
int FtlImpl_NEWftl::find_free_page(Event &event)
{
	return last_page ++;
}

// sreach in page array to find recived data from controller
int FtlImpl_NEWftl::search_in_page(Event &event)
{
	int time_taken = 0;
	for(int i = 0; i < page_number; i++)
	{
		time_taken = time_taken + BUS_CTRL_DELAY + BUS_DATA_DELAY;
		controller.issueRead(event); //timing
		printf("=== timing issueRead===\n");
		if (page[i].data == '-') { 	
			break; 
		}
		controller.stats.numFTLRead++;
		if (page[i].data == event.get_data()) {
			controller.issueBusLock(event , BUS_CTRL_DELAY + BUS_DATA_DELAY); //timing
			printf("=== timing issueBusLock===\n");
			return i;
		}
	}
	controller.issueBusLock(event, time_taken); //timing
	printf("=== timing issueBusLock===\n");
	return -1;
}

int FtlImpl_NEWftl::search_in_catch(Event &event)
{
	for (int i = 0; i < (int) CACHEMT_SIZE; i++)
	{
		controller.issueRamRead(event); //timing
		printf("=== timing issueRamRead===\n");
		if (cachemt[i].index == -1) { return -1; }
		if (cachemt[i].data == event.get_data())
		{
			return i;
		}
	}
	return -1;
}

