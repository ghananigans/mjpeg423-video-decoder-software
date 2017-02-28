/*
 * "Hello World" example.
 *
 * This example prints 'Hello from Nios II' to the STDOUT stream. It runs on
 * the Nios II 'standard', 'full_featured', 'fast', and 'low_cost' example
 * designs. It runs with or without the MicroC/OS-II RTOS and requires a STDOUT
 * device in your system's hardware.
 * The memory footprint of this hosted application is ~69 kbytes by default
 * using the standard reference design.
 *
 * For a reduced footprint version of this template, and an explanation of how
 * to reduce the memory footprint for a given application, see the
 * "small_hello_world" template.
 *
 */

#include <stdio.h>
#include "../../common/config.h"
#include "../../common/utils.h"
#include "idct_accel.h"
#include "ycbcr_to_rgb_accel.h"

int main()
{
	int retVal;

	printf("Application Starting (Slave Core - Core 1)...\n");

	//
	// Init IDCT accel
	//
	retVal = init_idct_accel();
	assert(retVal, "Failed to init idct accel!\n");
	//test_idct_accel();

	//
	// Init ycbcr_to_rgb accell
	//
	retVal = init_ycbcr_to_rgb_accel();
	assert(retVal, "Failed to ycbcr_to_rgb accel!\n");

	while(1);

  return 0;
}
