




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
  fclose(log_file_stream);
  delete ssd;
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
  int ret_status;
  int i = 0;
  Address address;
  unsigned long lba = 35;
  if(argc != 3) {
    printf("usage: test_2_1 <config_file_name> <log_file_path>\n");
    exit(EXIT_FAILURE);
  }

  strcpy(log_file_path, argv[2]);
  log_file_stream = fopen(log_file_path, "w+");

  load_config(argv[1]);

  Ssd *ssd = new Ssd(log_file_stream);
  print_config(log_file_stream);

  // write lba = 35, 18 times, ALL writes should succeed
  fprintf(log_file_stream, "Writing LBA %lu\n", lba);
  for(i = 1; i <= (int) BLOCK_SIZE + 2; i++) {
    ssd -> event_arrive(WRITE, lba, 1, i, &ret_status, address);
    if((ret_status == FAILURE) || (!ssd->is_valid(lba, address))) {
      fprintf(log_file_stream, "Error writing LBA %lu %dth time, may be you didn't clean the block!\n", lba, i);
      failed(ssd);
    }
  }

  // check if cleaning is invoked or not
  if (ssd->get_total_erases_performed() == 0)
  { 
    fprintf(log_file_stream, "You ensured successful rewrites without invoking cleaning !\n");
    failed(ssd);
  }

  delete ssd;
  printf("SUCCESS ...Check %s for more details.\n", log_file_path);
  return 0;
}
