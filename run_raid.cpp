/* Copyright 2012 Matias Bjørling */

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

#include "ssd.h"
#include <stdio.h>
#include <string.h>
#include <iostream>

#define SIZE 10

using namespace ssd;

int main(int argc, char **argv)
{
	printf("--- config file : %s \n--- trace file : %s \n",argv[1],argv[2]);
	// load config file from input - its first arg that recived from command line
	load_config(argv[1]);
	print_config(NULL);

	FILE *trace_file = NULL;


	// create ssds from RaidSsd class
	RaidSsd *ssd = new RaidSsd();

	double result;
	double cur_time = 1; 
	//--------------------------------------------------------------------------
	// check trace file to be exist or not - its second arg that recived from command line
	if ((trace_file = fopen(argv[2], "r")) == NULL) {
		fprintf(stderr, "Config file %s not found.  Exiting.\n", argv[2]);
		exit(FILE_ERR);
	}

	// set max charachter of each line in trace file
	// set first line in trace file
	char line[80];
	int lineNum = 1;
	

	// check trace file to find a valid line
	while(fgets(line, 80, trace_file) != NULL){


		
		char type;
		int logical_address; 
		int size;
		int start_time;
		char data;

		// char * pch
		// start rading from trace file
		sscanf(line, "%c,%d,%d,%d,%c", &type, &logical_address, &size, &start_time, &data);

		// printf("%c,%d,%d,%d\n", type, logical_address, size, start_time);

		if(type == 'W')
		{
			result = ssd -> event_arrive(WRITE, logical_address, size, start_time, data);
			printf("execution time is : %f\n", result);
			cur_time += result;
		}else if(type == 'R')
		{
			result = ssd -> event_arrive(READ, logical_address, size, start_time, data);
			printf("execution time is : %f\n", result);
			cur_time += result;
		}else
		{
			printf("error Exiting in line %d\n", lineNum);
			exit(-1);
		}

		
		// printf("---------------------------\n");

		lineNum ++;		
	}

	//--------------------------------------------------------------------------

	// for (int i = 0; i < SIZE; i++)
	// {
	// 	result = ssd -> event_arrive(WRITE, i*2, 1, 0);
	// 	cur_time += result;
	// }
	// for (int i = 0; i < SIZE; i++)
	// {
	// 	result = ssd -> event_arrive(READ, i*2, 1, 0);
	// 	cur_time += result;
	// }

	printf("Total execution time %f\n", cur_time);

	// delete all of ssd and close trace file 
	delete ssd;
	fclose(trace_file);
	return 0;
}
