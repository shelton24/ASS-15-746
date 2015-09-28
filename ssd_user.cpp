/* Copyright 2009, 2010 Brendan Tauras */

/* ssd_ftl.cpp is part of FlashSim. */

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

/* Ftl class
 * Brendan Tauras 2009-11-04
 *
 * This class is a stub class for the user to use as a template for implementing
 * his/her FTL scheme.  A few functions to gather information from lower-level
 * hardware are added to assist writing a FTL scheme.  The Ftl class should
 * rely on the Garbage_collector and Wear_leveler classes for modularity and
 * simplicity. */

#include <new>
#include <assert.h>
#include <stdio.h>
#include "ssd.h"

#define LOG_BLOCK_MAPPED_SUCCESS 1
#define LOG_BLOCK_MAPPED_FAILURE 0

using namespace ssd;

/** @brief Initialize Ftl data structures.
 *         
 *  This function initializes data structures like finding out the raw SSD
 *  SSD capacity and actual SSD capacity. A bool array of size equal to
 *  the number of pages in the SSD raw is initialized to all 0's. Each element
 *  in the array can have two states.
 *  Page Empty : 0
 *  Page Valid : 1
 *
 *  @param None
 *  @return Void
 */
void Ftl::init_ftl_user()
{
  /* get the SSD raw capacity */
  total_num_blocks_raw = SSD_SIZE * PACKAGE_SIZE * DIE_SIZE * PLANE_SIZE;
  /* get the overprovisioning limit */
  num_blocks_log_reservation = (OVERPROVISIONING/100) * total_num_blocks_raw;
  /* get the number of data blocks available */
  num_blocks_available = total_num_blocks_raw - num_blocks_log_reservation;
  /* get the cleaning block logical addr */
  cleaning_block_logical_addr = (total_num_blocks_raw - 1) * BLOCK_SIZE;
  unsigned int package_num_mod,dies_num_mod,plane_num_mod;
  cleaning_package_num = GET_DIVIDE_NUM(cleaning_block_logical_addr,PACKAGE_SIZE * DIE_SIZE * \
                               PLANE_SIZE * BLOCK_SIZE);
  package_num_mod = GET_MOD_NUM(cleaning_block_logical_addr,PACKAGE_SIZE * DIE_SIZE * \
                                PLANE_SIZE * BLOCK_SIZE);
  cleaning_dies_num = GET_DIVIDE_NUM(package_num_mod,DIE_SIZE * PLANE_SIZE * BLOCK_SIZE);
  dies_num_mod = GET_MOD_NUM(package_num_mod,DIE_SIZE * PLANE_SIZE * \
                             BLOCK_SIZE);
  cleaning_plane_num = GET_DIVIDE_NUM(dies_num_mod,PLANE_SIZE * BLOCK_SIZE);
  plane_num_mod = GET_MOD_NUM(dies_num_mod,PLANE_SIZE * BLOCK_SIZE);
  cleaning_block_num = GET_DIVIDE_NUM(plane_num_mod,BLOCK_SIZE);


  /* Initialize a bool array which stores the page states for the SSD.
   * The array stores the page state for each page in the SSD. I may change
   * this implementation in the next checkpoint. I thought of a bitmap 
   * instead of a bool array but I feel it will complicate the method in 
   * in finding the page state when a data block will be merged with a log
   * block into another data block.
   */
  page_status = new bool[total_num_blocks_raw * BLOCK_SIZE];

  /* Initialize the bool array with all 0's to indicate all pages are empty */
  std::fill_n(page_status,total_num_blocks_raw * BLOCK_SIZE,0);
}

/** @brief Convert LBA to PBA.
 *         
 *  Converts an LBA to a PBA depending on whether its a read event or a write
 *  event.
 *
 *  @param event Read or Write event.
 *  @return Success or Failure
 */
enum status Ftl::translate( Event &event ){
  /*
   * TODO: Translate logical address to physical address and return it.
   *
   * Remember to call event.set_address with the physical address that you
   * translate to, otherwise the framework will perform your event at the
   * wrong location.
   *
   * For Garbage Collection (checkpoint 2), invoke the garbage_collect method
   * defined in the FTL class. That will call the collect method with the
   * selected garbage collection policy.
   *
   * Finally, for wear leveling (checkpoint 3), please call the level method
   * directly.
   */

  /* Result of the read/write SUCCESS or FAILURE */
  enum status result;

  /* LPA to PBA mapping */
  Address addr_allocate;

  /* Find if it's a read or write event */
  switch(event.get_event_type())
  {
    case WRITE:
      result = Ftl::write_event(event,&addr_allocate);
      break;
    case READ:
      result = Ftl::read_event(event,&addr_allocate);
      break;
    default:
      break;
  }

  event.set_address(&addr_allocate);

  fprintf(log_file, "Translating LBA %lu\n", event.get_logical_address());

  return result;
}

/** @brief Convert LBA to PBA in case of a write event
 *         
 *  Converts an LBA to a PBA. It first checks if the page is empty and writes
 *  to it if found empty. If not, it checks if a log-block is mapped to the
 *  data block. the folllowing two conditions are possible :
 *  1. Yes: Is a page from log block empty?
 *     1. Yes: Associate LBA with the page.
 *     2. No: Return Failure.
 *  2. No: Is a log-block unmapped
 *     1. Yes: Map log-block to data block.
 *     2. No : Return Failure.
 *
 *  @param event Read or Write event.
 *  @param addr_allocated The address to which LBA is to be mapped.
 *  @return Success or Failure
 */
enum status Ftl::write_event(Event &event,Address* addr_allocate)
{
  /* Get the LBA */
  unsigned logical_address = event.get_logical_address();

  /* Get the package number, die number, plane number, block number and 
   * page number from the LBA.
   */
  unsigned int package_num,dies_num,plane_num,block_num,page_num;
  unsigned int package_num_mod,dies_num_mod,plane_num_mod;
  package_num = GET_DIVIDE_NUM(logical_address,PACKAGE_SIZE * DIE_SIZE * \
                               PLANE_SIZE * BLOCK_SIZE);
  package_num_mod = GET_MOD_NUM(logical_address,PACKAGE_SIZE * DIE_SIZE * \
                                PLANE_SIZE * BLOCK_SIZE);
  dies_num = GET_DIVIDE_NUM(package_num_mod,DIE_SIZE * PLANE_SIZE * BLOCK_SIZE);
  dies_num_mod = GET_MOD_NUM(package_num_mod,DIE_SIZE * PLANE_SIZE * \
                             BLOCK_SIZE);
  plane_num = GET_DIVIDE_NUM(dies_num_mod,PLANE_SIZE * BLOCK_SIZE);
  plane_num_mod = GET_MOD_NUM(dies_num_mod,PLANE_SIZE * BLOCK_SIZE);
  block_num = GET_DIVIDE_NUM(plane_num_mod,BLOCK_SIZE);
  page_num = GET_MOD_NUM(package_num_mod,BLOCK_SIZE);

  package_number = package_num;
  dies_number = dies_num;
  plane_number = plane_num;
  block_number = block_num;

  /* Return with a failure status if the LBA maps to a location which falls
   * into the overprovisioned space.
   */
  if(logical_address >= (num_blocks_available * BLOCK_SIZE))
      return FAILURE;

    /* Page is not empty, that is, it was written before */
    if(page_status[logical_address] == 1)
    {

      int logical_block_number = logical_address/BLOCK_SIZE;
      if(check_log_block_mapped_data_block(logical_block_number))
      {

        /* Log-reservation block mapped to data block */
        int page_index;
        if((page_index = check_page_empty_log_block(logical_block_number)) == -1) 
        /* No empty pages left in the log block. Return with a failure sttaus. */
        {

          garbage.collect(event,FIFO);
          LOG_BLOCK* log_block = log_block_map.at(logical_block_number);
          /* fill all the page entries with -1 */
          std::fill_n(log_block->page_entries,BLOCK_SIZE, -1);
          /* Map the lba to the first page of the log block */
          log_block-> page_entries[0] = page_num;
          package_num = log_block -> package_num;
          dies_num = log_block -> die_num;
          plane_num = log_block -> plane_num;
          block_num = log_block -> block_num;
          page_num = 0; 
        }

        else
        {
          /* There is an empty page in the block */
          LOG_BLOCK* log_block = log_block_map.at(logical_block_number);
          log_block -> page_entries[page_index] = page_num;
          package_num = log_block -> package_num;
          dies_num = log_block -> die_num;
          plane_num = log_block -> plane_num;
          block_num = log_block -> block_num;
          page_num = page_index; 
        }

      }

      /* Log-reservation block not mapped to data block */
      else
      {
        /* if size of the map is equal to the numbe of overprovisioned blocks
         * return with a failure.
         */
        if((num_blocks_available + log_block_map.size()) == (total_num_blocks_raw - 1))
        {
          /* No unmapped log-reservation blocks left */
          return FAILURE;
        }

        /* There are additional log-reservations blocks left. Get the package 
         * number, die number, plane number, block number and page number of 
         * the log-blockfrom the LBA.
         */
        unsigned long log_block_logical_addr = ((num_blocks_available) + \
                                                 log_block_map.size()) *  \
                                                 BLOCK_SIZE;
        package_num = GET_DIVIDE_NUM(log_block_logical_addr,PACKAGE_SIZE * \
                                     DIE_SIZE * PLANE_SIZE * BLOCK_SIZE);
        package_num_mod = GET_MOD_NUM(log_block_logical_addr,PACKAGE_SIZE * \
                                      DIE_SIZE * PLANE_SIZE * BLOCK_SIZE);
        dies_num = GET_DIVIDE_NUM(package_num_mod,DIE_SIZE * PLANE_SIZE * \
                                  BLOCK_SIZE);
        dies_num_mod = GET_MOD_NUM(package_num_mod,DIE_SIZE * PLANE_SIZE * \
                                   BLOCK_SIZE);
        plane_num = GET_DIVIDE_NUM(dies_num_mod,PLANE_SIZE * BLOCK_SIZE);
        plane_num_mod = GET_MOD_NUM(dies_num_mod,PLANE_SIZE * BLOCK_SIZE);
        block_num = GET_DIVIDE_NUM(plane_num_mod,BLOCK_SIZE);
        page_num = GET_MOD_NUM(package_num_mod,BLOCK_SIZE);

        /* Pack the struct so that it can be inserted into the map */
        LOG_BLOCK* log_block = new LOG_BLOCK;
        log_block -> package_num = package_num;
        log_block -> die_num = dies_num;
        log_block -> plane_num = plane_num;
        log_block -> block_num = block_num;
        log_block -> page_entries = new int[BLOCK_SIZE];
        /* Initially, fill all the page entries with -1 */
        std::fill_n(log_block->page_entries,BLOCK_SIZE, -1);
        /* Map the lba to the first page of the log block */
        log_block-> page_entries[0] = page_num;
        /* Store the key in the map as the data block number */
        log_block_map[logical_address/BLOCK_SIZE] = log_block;
      }

    }

    else
      /* Page was empty before this, now it will be written, so make the
       * corresposding index in page_status as 1.
       */
      page_status[logical_address] = 1;

    /* Pack the new address to which the lba will be mapped */
    *addr_allocate = new Address(package_num,dies_num,plane_num,block_num,
                                 page_num,PAGE); 
    return SUCCESS;
}

/** @brief Convert LBA to PBA in case of a read event
 *         
 *  Converts an LBA to a PBA. It first checks if the page is empty and reads
 *  from it if found empty. If not, it checks if a log-block is mapped to the
 *  data block. the folllowing two conditions are possible :
 *  1.Is a log-block mapped to the data block?
 *     1. Yes: Read from the most recent copy in the log block.
 *     2. No: Read from orginal page in data block.
 *
 *  @param event Read or Write event.
 *  @param addr_allocated The address to which LBA is to be mapped.
 *  @return Success or Failure
 */
enum status Ftl::read_event(Event &event,Address* addr_allocate)
{
   /* Get the LBA */
  unsigned logical_address = event.get_logical_address();

  /* Get the package number, die number, plane number, block number and 
   * page number from the LBA.
   */
  unsigned int package_num,dies_num,plane_num,block_num,page_num;
  unsigned int package_num_mod,dies_num_mod,plane_num_mod;
  package_num = GET_DIVIDE_NUM(logical_address,PACKAGE_SIZE * DIE_SIZE * \
                               PLANE_SIZE * BLOCK_SIZE);
  package_num_mod = GET_MOD_NUM(logical_address,PACKAGE_SIZE * DIE_SIZE * \
                                PLANE_SIZE * BLOCK_SIZE);
  dies_num = GET_DIVIDE_NUM(package_num_mod,DIE_SIZE * PLANE_SIZE * BLOCK_SIZE);
  dies_num_mod = GET_MOD_NUM(package_num_mod,DIE_SIZE * PLANE_SIZE * \
                             BLOCK_SIZE);
  plane_num = GET_DIVIDE_NUM(dies_num_mod,PLANE_SIZE * BLOCK_SIZE);
  plane_num_mod = GET_MOD_NUM(dies_num_mod,PLANE_SIZE * BLOCK_SIZE);
  block_num = GET_DIVIDE_NUM(plane_num_mod,BLOCK_SIZE);
  page_num = GET_MOD_NUM(package_num_mod,BLOCK_SIZE);

  
  if(page_status[logical_address] == 1)
    {
      /* Page contains valid data */

      int logical_block_number = logical_address/BLOCK_SIZE;
      if(check_log_block_mapped_data_block(logical_block_number))
      {
        /* A log block is mapped to the data block */
        int page_index;
        if((page_index = check_page_exists_log_block(logical_block_number,
                                                    page_num)) != -1)
        {
          /* Get the most recent copy of the page from the log block */
          LOG_BLOCK* log_block = log_block_map.at(logical_block_number);
          package_num = log_block -> package_num;
          dies_num = log_block -> die_num;
          plane_num = log_block -> plane_num;
          block_num = log_block -> block_num;
          page_num = page_index;
        }

      }

      /* Pack the new address to which the lba will be mapped */
      *addr_allocate = new Address(package_num,dies_num,plane_num,block_num,
                                   page_num,PAGE); 
      return SUCCESS;
    }

    else
      /* Page is empty, that is, it was never written before. It may contain
       * junk data., thus, return with a FAILURE.
       */
      return FAILURE;

}

/** @brief Checks if log block is mapped to a data block.
 *         
 *  Checks if a data block exists as a key in the map using the count
 *  function.
 *
 *  @param logical_block_num data block to be checked in the map.
 *  @return Success or Failure
 */
bool Ftl::check_log_block_mapped_data_block(unsigned int logical_block_num)
{
  /* Use the count function to find out if a block exists */
  if(log_block_map.count(logical_block_num) != 0)
  {
    /* Ensure that there are no duplicate entries */
    assert(log_block_map.count(logical_block_num) == 1);
    return LOG_BLOCK_MAPPED_SUCCESS;
  }

    return LOG_BLOCK_MAPPED_FAILURE;
}

/** @brief Checks if an empty page exists in the log block
 *         
 *  Return the index in the page_entries array where a page is empty.
 *
 *  @param logical_block_num data block to be checked in the map.
 *  @return index 
 */
unsigned int Ftl::check_page_empty_log_block(unsigned int logical_block_num)
{
  LOG_BLOCK* log_block = log_block_map.at(logical_block_num);

  for(int index = 0; index < (int)BLOCK_SIZE; index++)
  {
    if(log_block -> page_entries[index] == -1)
      /* There is an empty page at location index */
      return index;
  }

  /* No empty page in log block */
  return -1;

}

/** @brief Check if page exists in the log block
 *         
 *  Check if a page exists in the log block and get the latest copy if there
 *  exists one.
 *
 *  @param logical_block_num data block to be checked in the map.
 *  @param page_offset page number to be checked in the array.
 *  @return index.
 */
unsigned int Ftl::check_page_exists_log_block(unsigned int logical_block_number,
                                              int page_offset)
{
  LOG_BLOCK* log_block = log_block_map.at(logical_block_number);

  /* Start a loop from beginning from the end of the array so that we can 
   * find the latest copy of a page.
   */
  for(int index = BLOCK_SIZE - 1; index >= 0; index--)
  {
    if((log_block -> page_entries[index]) == page_offset)
      /* Page exists in log block, return index. */
      return index;
  }

  return -1;
}

enum status Garbage_collector::collect(Event &event __attribute__((unused)), enum GC_POLICY policy __attribute__((unused)))
{
  /*
   * TODO: Implement garbage collection policies and perform cleaning.
   */
   perform_cleaning(event.get_logical_address(),event.get_logical_address() / BLOCK_SIZE);

  return SUCCESS;
}

void Garbage_collector::perform_cleaning(unsigned long logical_address,unsigned int logical_block_num)
{

   merge_data_and_log_block(logical_address,logical_block_num);

   erase_data_and_log_block(logical_address,logical_block_num);

   copy_cleaning_block_to_data_block(logical_address);

   erase_cleaning_block();

    return;

}

void Garbage_collector::merge_data_and_log_block(unsigned long logical_address,unsigned int logical_block_num)
{
  LOG_BLOCK* log_block = ftl.log_block_map.at(logical_block_num);
  unsigned long log_block_address = GET_LOGICAL_ADDR(log_block->package_num,log_block->die_num,log_block->plane_num,log_block->block_num);
  int page_index = 0;
  int page_offset = 0;
  Address addr_allocate;
  Event *event = NULL;

  for(;page_offset < (int)BLOCK_SIZE;page_offset++)
   {
      page_index = ftl.check_page_exists_log_block(logical_block_num,
                                                    page_offset);
        if(page_index != -1)
        {
          addr_allocate = new Address(log_block->package_num,log_block->die_num,log_block->plane_num,log_block->block_num,page_offset,PAGE);
          event = new Event(READ,log_block_address + page_index,1,0);
          event->set_address(&addr_allocate);
          ftl.controller.issue(*event);
          addr_allocate = new Address(ftl.cleaning_package_num,ftl.cleaning_dies_num,ftl.cleaning_plane_num,ftl.cleaning_block_num,page_offset,PAGE);
          event = new Event(WRITE,ftl.cleaning_block_logical_addr + page_offset,1,0);
          event->set_address(&addr_allocate);
          ftl.controller.issue(*event);
        }

        else
        {
          if(ftl.page_status[logical_address + page_offset] == 1)
          {
          addr_allocate = new Address(ftl.package_number,ftl.dies_number,ftl.plane_number,ftl.block_number,page_offset,PAGE);
          event = new Event(READ,logical_address + page_offset,1,0);
          event->set_address(&addr_allocate);
          ftl.controller.issue(*event);
          addr_allocate = new Address(ftl.cleaning_package_num,ftl.cleaning_dies_num,ftl.cleaning_plane_num,ftl.cleaning_block_num,page_offset,PAGE);
          event = new Event(WRITE,ftl.cleaning_block_logical_addr + page_offset,1,0);
          event->set_address(&addr_allocate);
          ftl.controller.issue(*event);
          }
        }

    }  
}

void Garbage_collector::erase_data_and_log_block(unsigned long logical_address,unsigned int logical_block_num)
{
    Address addr_allocate;
    Event *event = NULL;

    unsigned long data_block_address = (logical_address/BLOCK_SIZE) * BLOCK_SIZE;
    LOG_BLOCK* log_block = ftl.log_block_map.at(logical_block_num);
    unsigned long log_block_address = GET_LOGICAL_ADDR(log_block->package_num,log_block->die_num,log_block->plane_num,log_block->block_num);

    addr_allocate = new Address(ftl.package_number,ftl.dies_number,ftl.plane_number,ftl.block_number,0,BLOCK);
    event = new Event(ERASE,data_block_address,1,0);
    event->set_address(&addr_allocate);
    ftl.controller.issue(*event);
    addr_allocate = new Address(log_block->package_num,log_block->die_num,log_block->plane_num,log_block->block_num,0,BLOCK);
    event = new Event(ERASE,log_block_address,1,0);
    event->set_address(&addr_allocate);
    ftl.controller.issue(*event);
}

void Garbage_collector::copy_cleaning_block_to_data_block(unsigned long logical_address)
{
    int page_offset = 0;
    Address addr_allocate;
    Event *event = NULL;
    unsigned long data_block_address = (logical_address/BLOCK_SIZE) * BLOCK_SIZE;

    for(page_offset = 0;page_offset < (int)BLOCK_SIZE;page_offset++)
    {
      if(ftl.page_status[logical_address + page_offset] == 1)
      {
      addr_allocate = new Address(ftl.cleaning_package_num,ftl.cleaning_dies_num,ftl.cleaning_plane_num,ftl.cleaning_block_num,page_offset,PAGE);
      event = new Event(READ,ftl.cleaning_block_logical_addr + page_offset,1,0);
      event->set_address(&addr_allocate);
      ftl.controller.issue(*event);
      addr_allocate = new Address(ftl.package_number,ftl.dies_number,ftl.plane_number,ftl.block_number,page_offset,PAGE);
      event = new Event(WRITE,data_block_address + page_offset,1,0);
      event->set_address(&addr_allocate);
      ftl.controller.issue(*event);
      }
    }
}

void Garbage_collector::erase_cleaning_block()
{
    Address addr_allocate;
    Event *event = NULL;

    addr_allocate = new Address(ftl.cleaning_package_num,ftl.cleaning_dies_num,ftl.cleaning_plane_num,ftl.cleaning_block_num,0,BLOCK);
    event = new Event(ERASE,ftl.cleaning_block_logical_addr,1,0);
    event->set_address(&addr_allocate);
    ftl.controller.issue(*event);
}


enum status Wear_leveler::level( Event &event __attribute__((unused)))
{
  /*
   * TODO: Develop your wear leveling scheme here.
   */
  return FAILURE;
}
