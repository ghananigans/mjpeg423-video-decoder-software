/*
 * config.h
 *
 *  Created on: Jan 17, 2017
 *      Author: ggowripa
 */

#ifndef CONFIG_H_
#define CONFIG_H_

// this forces the use of the null implementation of ycbcr_to_rgb in ycbcr_to_rgb.c
// without this the colours look weird :/ not sure why
#define NULL_COLORCONV

//
// use this to turn off software IDCT
//
#define NULL_DCT

// The size of the display
// TODO: since this is a fixed size we could probably statically allocate the working buffers
#define DISPLAY_WIDTH 		(640)
#define DISPLAY_HEIGHT		(480)

// The number of buffers to initialize the display with
#define NUM_OUTPUT_BUFFERS	(2)

#define FRAME_RATE_MS 		(750)

#define FORCE_PERIODIC      (0)

//
// Comment this out to disable timing reports
//
//#define TIMING_TESTS
//#define SIZE_TESTS

//
// Comment this out to remove print statements
//
#define DEBUG_PRINT_ENABLED


#endif /* CONFIG_H_ */
