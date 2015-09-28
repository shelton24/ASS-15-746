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
  int time = 0;
  unsigned long lba = 0;
  char addr_str[50] = "";
  int max_block_rewrites_allowed = 0;
  Address ref_addr;
  Address address;

  if(argc != 3) {
    printf("usage: test_2_5 <config_file_name> <log_file_path>\n");
    exit(EXIT_FAILURE);
  }

  strcpy(log_file_path, argv[2]);
  log_file_stream = fopen(log_file_path, "w+");

  load_config(argv[1]);

  Ssd *ssd = new Ssd(log_file_stream);
  print_config(log_file_stream);

  max_block_rewrites_allowed = (((float) OVERPROVISIONING / 100) * SSD_SIZE * PACKAGE_SIZE * DIE_SIZE * PLANE_SIZE);

  lba = 48;
  fprintf(log_file_stream, "----------------\nWriting LBA %lu\n", lba);
  ssd -> event_arrive(WRITE, lba, 1, (++time), &ret_status, address);
  if((ret_status == FAILURE) || (!ssd->is_valid(lba, address))) {
    fprintf(log_file_stream, "Error writing LBA %lu first time\n", lba);
    failed(ssd);
  }

  sprintf(addr_str, "[%u,%u,%u,%u,%u]", address.package, address.die,
      address.plane, address.block, address.page);
  fprintf(log_file_stream, "LBA %lu -> PBA %s\n", lba, addr_str);

  ref_addr = address;

  fprintf(log_file_stream, "----------------\nRewriting LBA %lu\n", lba);
  ssd -> event_arrive(WRITE, lba, 1, (++time), &ret_status, address);
  if((ret_status == FAILURE) || (!ssd->is_valid(lba, address))) {
    fprintf(log_file_stream, "Error writing LBA %lu first time\n", lba);
    failed(ssd);
  }

  sprintf(addr_str, "[%u,%u,%u,%u,%u]", address.package, address.die,
      address.plane, address.block, address.page);
  fprintf(log_file_stream, "LBA %lu -> PBA %s\n", lba, addr_str);

  lba = 49;
  fprintf(log_file_stream, "----------------\nWriting LBA %lu\n", lba);
  ssd -> event_arrive(WRITE, lba, 1, (++time), &ret_status, address);
  if((ret_status == FAILURE) || (!ssd->is_valid(lba, address))) {
    fprintf(log_file_stream, "Error writing LBA %lu first time\n", lba);
    failed(ssd);
  }

  sprintf(addr_str, "[%u,%u,%u,%u,%u]", address.package, address.die,
      address.plane, address.block, address.page);
  fprintf(log_file_stream, "LBA %lu -> PBA %s\n", lba, addr_str);

  if(ssd->get_total_erases_performed() > 0) {
    fprintf(log_file_stream, "Unnecessary erasures\n");
    failed(ssd);
  }

  i = 0;
  while (ssd->get_total_erases_performed() == 0) {
    if(i > max_block_rewrites_allowed) {
      fprintf(log_file_stream, "Overprovisioning limits violated!\n");
      failed(ssd);
    }

    if(i == 3) {
      i++;
      continue;
    }

    lba = (i * BLOCK_SIZE);

    fprintf(log_file_stream, "----------------\nWriting LBA %lu\n", lba);
    ssd -> event_arrive(WRITE, lba, 1, (++time), &ret_status, address);
    if((ret_status == FAILURE) || (!ssd->is_valid(lba, address))) {
      fprintf(log_file_stream, "Error writing LBA %lu\n", lba);
      failed(ssd);
    }
    sprintf(addr_str, "[%u,%u,%u,%u,%u]", address.package, address.die,
        address.plane, address.block, address.page);
    fprintf(log_file_stream, "LBA %lu -> PBA %s\n", lba, addr_str);

    fprintf(log_file_stream, "----------------\nRewriting LBA %lu\n", lba);
    ssd -> event_arrive(WRITE, lba, 1, (++time), &ret_status, address);
    if((ret_status == FAILURE) || (!ssd->is_valid(lba, address))) {
      fprintf(log_file_stream, "Error writing LBA %lu\n", lba);
      failed(ssd);
    }

    sprintf(addr_str, "[%u,%u,%u,%u,%u]", address.package, address.die,
        address.plane, address.block, address.page);
    fprintf(log_file_stream, "LBA %lu -> PBA %s\n", lba, addr_str);

    lba++;

    fprintf(log_file_stream, "----------------\nWriting LBA %lu\n", lba);
    ssd -> event_arrive(WRITE, lba, 1, (++time), &ret_status, address);
    if((ret_status == FAILURE) || (!ssd->is_valid(lba, address))) {
      fprintf(log_file_stream, "Error writing LBA %lu\n", lba);
      failed(ssd);
    }
    sprintf(addr_str, "[%u,%u,%u,%u,%u]", address.package, address.die,
        address.plane, address.block, address.page);
    fprintf(log_file_stream, "LBA %lu -> PBA %s\n", lba, addr_str);

    i++;
  }

  lba = 48;
  fprintf(log_file_stream, "----------------\nReading LBA %lu\n", lba);
  ssd -> event_arrive(READ, lba, 1, (++time), &ret_status, address);
  if((ret_status == FAILURE) || (!ssd->is_valid(lba, address))) {
      fprintf(log_file_stream, "Error reading LBA %lu\n", lba);
      failed(ssd);
  }
  sprintf(addr_str, "[%u,%u,%u,%u,%u]", address.package, address.die,
        address.plane, address.block, address.page);
  fprintf(log_file_stream, "LBA %lu -> PBA %s\n", lba, addr_str);

  if(ref_addr != address)
  {
    fprintf(log_file_stream, "You cleaned the wrong block as per the cleaning policy\n");
    failed(ssd);
  }

  delete ssd;
  printf("SUCCESS ...Check %s for more details.\n", log_file_path);
  return 0;
}
