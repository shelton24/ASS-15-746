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
  unsigned long first_raw_lba, last_raw_lba, last_actual_lba;
  Address address;
  if(argc != 3) {
    printf("usage: test_1_4 <config_file_name> <log_file_path>\n");
    exit(EXIT_FAILURE);
  }

  strcpy(log_file_path, argv[2]);
  log_file_stream = fopen(log_file_path, "w+");

  load_config(argv[1]);
		
	fprintf(log_file_stream, "------------------------------------------------------------\n");

  Ssd *ssd = new Ssd(log_file_stream);
  print_config(log_file_stream);

  first_raw_lba = 0;
  last_raw_lba = (SSD_SIZE * PACKAGE_SIZE * DIE_SIZE * PLANE_SIZE * BLOCK_SIZE) - 1;
  last_actual_lba = (last_raw_lba - (((double)OVERPROVISIONING / 100) * last_raw_lba));
	
	fprintf(log_file_stream, "----------------\nWriting LBA 0\n");
  
	ssd->event_arrive(WRITE, first_raw_lba, 1, 1, &ret_status, address);
  if((ret_status == FAILURE) || (!ssd->is_valid(first_raw_lba, address))) {
    fprintf(log_file_stream, "Error writing LBA 0\n");
    failed(ssd);
  }
	
	fprintf(log_file_stream, "----------------\nWriting last raw LBA\n");

  ssd->event_arrive(WRITE, last_raw_lba, 1, 2, &ret_status, address);
  if((ret_status == SUCCESS) || (ssd->is_valid(last_raw_lba, address))) {
    fprintf(log_file_stream, "Since last raw LBA is getting written, there is no overprovisioning. This is not as per spec.\n");
    failed(ssd);
  }

	fprintf(log_file_stream, "----------------\nWriting last actual LBA, keeping in mind overprovisioning.\n");
  
	ssd->event_arrive(WRITE, last_actual_lba, 1, 3, &ret_status, address);
  if((ret_status == FAILURE) || (!ssd->is_valid(last_actual_lba, address))) {
    fprintf(log_file_stream, "Error writing LBA %lu (maybe you have exceeded overprovisioning?)\n", last_actual_lba);
    failed(ssd);
  }
  fflush(log_file_stream);
  fclose(log_file_stream);
  delete ssd;
  printf("SUCCESS ...Check %s for more details.\n", log_file_path);
  return 0;
}
