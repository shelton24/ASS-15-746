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
  char addr_str[50] = "", addr_str_2[50] = "";
  if(argc != 3) {
    printf("usage: test_1_1 <config_file_name> <log_file_path>\n");
    exit(EXIT_FAILURE);
  }

  strcpy(log_file_path, argv[2]);
  log_file_stream = fopen(log_file_path, "w+");
  load_config(argv[1]);

	fprintf(log_file_stream, "------------------------------------------------------------\n");

  Ssd *ssd = new Ssd(log_file_stream);
  print_config(log_file_stream);

	fprintf(log_file_stream, "----------------\nWriting LBA 0\n");

  ssd -> event_arrive(WRITE, 0, 1, 1, &ret_status, address);
  if((ret_status == FAILURE) || (!ssd->is_valid(0, address))) {
    fprintf(log_file_stream, "Error writing LBA 0\n");
    failed(ssd);
  }

  sprintf(addr_str, "[%u,%u,%u,%u,%u]", address.package, address.die,
      address.plane, address.block, address.page);
  fprintf(log_file_stream, "LBA 0 -> PBA %s\n", addr_str);

  fprintf(log_file_stream, "LBA 0 written\n");
	
	fprintf(log_file_stream, "----------------\nRewriting LBA 0\n");

  ssd -> event_arrive(WRITE, 0, 1, 2, &ret_status, address);
  if((ret_status == FAILURE) || (!ssd->is_valid(0, address))) {
    fprintf(log_file_stream, "Error rewriting LBA 0\n");
    failed(ssd);
  }

  fprintf(log_file_stream, "LBA 0 rewritten\n");

  sprintf(addr_str_2, "[%u,%u,%u,%u,%u]", address.package, address.die,
      address.plane, address.block, address.page);
  fprintf(log_file_stream, "LBA 0 -> PBA %s\n", addr_str_2);

  if(strcmp(addr_str, addr_str_2) == 0) {
    fprintf(log_file_stream, "LBA 0 not mapping to new address after first write\n");
    failed(ssd);
  }

  fflush(log_file_stream);
  fclose(log_file_stream);
  delete ssd;
  printf("SUCCESS ...Check %s for more details.\n", log_file_path);
  return 0;
}
