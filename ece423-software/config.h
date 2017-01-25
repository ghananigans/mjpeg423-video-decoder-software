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

// The size of the display
// TODO: since this is a fixed size we could probably statically allocate the working buffers
#define DISPLAY_WIDTH 		(640)
#define DISPLAY_HEIGHT		(480)

// The number of buffers to initialize the display with
#define NUM_OUTPUT_BUFFERS	(2)

#define FRAME_RATE_MS 		(750)

//
// Comment this out to disable timing reports
//
#define TIMING_TESTS

#ifdef TIMING_TESTS
#define TIMING_TEST_SD_READ
#define TIMING_TEST_LOSSLESS_Y
#define TIMING_TEST_LOSSLESS_CB
#define TIMING_TEST_LOSSLESS_CR
#define TIMING_TEST_IDCT_ONE_FRAME
//#define TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT
//#define TIMING_TEST_IDCT_ONE_8_X_8_BLOCK
#define TIMING_TEST_YCBCR_TO_RGB_ONE_FRAME
//#define TIMING_TEST_YCBCR_TO_RGB_8_X_8_BLOCK
#endif // #ifdef TIMING_TESTS
//
// Comment this out to remove print statements
//
//#define DEBUG_PRINT_ENABLED


#endif /* CONFIG_H_ */
