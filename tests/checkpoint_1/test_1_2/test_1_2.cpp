/* Copyright 2009, 2010 Brendan Tauras */

/* run_test.cpp is part of FlashSim. */

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

#include <string.h>
#include "ssd.h"

using namespace ssd;
FILE *log_file_stream;
char log_file_path[255];

void failed(Ssd *ssd) {
  printf("FAILED ...Check %s for more details.\n", log_file_path);
  fflush(log_file_stream);
  fclose(log_file_stream);
  delete ssd;
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
  int ret_status;
  Address address;
  char addr_str_2[50] = "";
  if(argc != 3) {
    printf("usage: test_1_2 <config_file_name> <log_file_path>\n");
    exit(EXIT_FAILURE);
  }

  strcpy(log_file_path, argv[2]);
  log_file_stream = fopen(log_file_path, "w+");

  load_config(argv[1]);
	
	fprintf(log_file_stream, "------------------------------------------------------------\n");

  Ssd *ssd = new Ssd(log_file_stream);
  print_config(log_file_stream);

	Address original_block, log_block;

		
	fprintf(log_file_stream, "----------------\nWriting LBA 0\n");
  
	ssd -> event_arrive(WRITE, 0, 1, 1, &ret_status, address);
  if((ret_status == FAILURE) || (!ssd->is_valid(0, address))) {
    fprintf(log_file_stream, "Error writing LBA 0\n");
    failed(ssd);
  }

  fprintf(log_file_stream, "LBA 0 written\n");
	original_block = address;
	original_block.page = 0;

	fprintf(log_file_stream, "\n----------------\nReading LBA 0\n");
 
 	ssd -> event_arrive(READ, 0, 1, 2, &ret_status, address);
  if((ret_status == FAILURE) || (!ssd->is_valid(0, address))) {
    fprintf(log_file_stream, "Error reading LBA 0\n");
    failed(ssd);
  }
	
	fprintf(log_file_stream, "\n----------------\nRewriting LBA 0\n");

  ssd -> event_arrive(WRITE, 0, 1, 3, &ret_status, address);
  if((ret_status == FAILURE) || (!ssd->is_valid(0, address))) {
    fprintf(log_file_stream, "Error rewriting LBA 0\n");
    failed(ssd);
  }

  fprintf(log_file_stream, "LBA 0 rewritten\n");
	log_block = address;
	log_block.page = 0;
	
	fprintf(log_file_stream, "\n----------------\nWriting LBA 1\n");

  ssd -> event_arrive(WRITE, 1, 1, 4, &ret_status, address);
  if((ret_status == FAILURE) || (!ssd->is_valid(1, address))) {
    fprintf(log_file_stream, "Error writing LBA 1\n");
    failed(ssd);
  }

  fprintf(log_file_stream, "LBA 1 written\n");

  sprintf(addr_str_2, "[%u,%u,%u,%u,%u]", address.package, address.die,
      address.plane, address.block, address.page);
  fprintf(log_file_stream, "LBA 1 -> PBA %s\n\n", addr_str_2);
	
	address.page = 0;
  
	if(address == log_block) {
    fprintf(log_file_stream, "LBA 1 has been unnecessarily mapped to the log block.\n");
    failed(ssd);
  }

  if(!(address == original_block)) {
    fprintf(log_file_stream, "LBA 1 and LBA0 are on different blocks, have you implemented a page-level mapping scheme?\n");
    failed(ssd);
  }
  fflush(log_file_stream);
  fclose(log_file_stream);

  delete ssd;
  printf("SUCCESS ...Check %s for more details.\n", log_file_path);
  return 0;
}
