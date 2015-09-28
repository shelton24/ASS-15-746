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


using namespace ssd;

Ftl::Ftl(Controller &controller, FILE *log_file):
	log_file(log_file),
  controller(controller),
	garbage(*this, log_file),
	wear(*this, log_file)
{
  init_ftl_user();
	return;
}

Ftl::~Ftl(void)
{
	return;
}

enum status Ftl::read(Event &event)
{
  /*
   * Issue read commands to controller from here.
   */
  Address pba;
  enum status retVal;

  retVal = translate( event );
  if(retVal != SUCCESS) {
    fprintf(log_file, "%s: Invalid mapping of LBA %lu\n", __func__,
        event.get_logical_address());
    return FAILURE;
  }
  
  pba = event.get_address();

  fprintf(log_file, "%s: LBA %lu mapped to PBA (%u, %u, %u, %u, %u)\n",
      __func__, event.get_logical_address(), pba.package, pba.die,
      pba.plane, pba.block, pba.page);
	return controller.issue(event);
}

enum status Ftl::write(Event &event)
{
  /*
   * Issue write commands to controller from here.
   */
  Address pba;
  enum status retVal;

  retVal = translate( event );
  if(retVal != SUCCESS) {
    fprintf(log_file, "%s: Invalid mapping of LBA %lu\n", __func__,
        event.get_logical_address());
    return FAILURE;
  }
  
  pba = event.get_address();

  fprintf(log_file, "%s: LBA %lu mapped to PBA (%u, %u, %u, %u, %u)\n",
      __func__, event.get_logical_address(), pba.package, pba.die,
      pba.plane, pba.block, pba.page);
	return controller.issue(event);
}

enum status Ftl::erase(Event &event)
{
  /*
   * Issue erase commands to controller from here.
   */
  Address pba;
  enum status retVal;

  retVal = translate( event );
  if(retVal != SUCCESS) {
    fprintf(log_file, "%s: Invalid mapping of LBA %lu\n", __func__,
        event.get_logical_address());
    return FAILURE;
  }
  
  pba = event.get_address();

  fprintf(log_file, "%s: LBA %lu mapped to PBA (%u, %u, %u, %u, %u)\n",
      __func__, event.get_logical_address(), pba.package, pba.die,
      pba.plane, pba.block, pba.page);
	return controller.issue(event);
}

enum status Ftl::merge(Event &event)
{
  /*
   * Issue merge commands to controller from here.
   */
  Address pba;
  enum status retVal;

  retVal = translate( event );
  if(retVal != SUCCESS) {
    fprintf(log_file, "%s: Invalid mapping of LBA %lu\n", __func__,
        event.get_logical_address());
    return FAILURE;
  }
  
  pba = event.get_address();

  fprintf(log_file, "%s: LBA %lu mapped to PBA (%u, %u, %u, %u, %u)\n",
      __func__, event.get_logical_address(), pba.package, pba.die,
      pba.plane, pba.block, pba.page);
	return controller.issue(event);
}

enum status Ftl::garbage_collect(Event &event)
{
	return garbage.collect(event, SELECTED_GC_POLICY);
}
