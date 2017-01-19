/*
 * config.h
 *
 *  Created on: Jan 17, 2017
 *      Author: ggowripa
 */

#ifndef CONFIG_H_
#define CONFIG_H_

// this forces the use of the null implementation of ycbcr_to_rgb in ycbcr_to_rgb.c
// without this the coulors look weird :/ not sure why
#define NULL_COLORCONV

// The size of the display
// TODO: since this is a fixed size we could probably statically allocate the working buffers
#define DISPLAY_WIDTH 		(640)
#define DISPLAY_HEIGHT		(480)

// The number of buffers to initialize the display with
#define NUM_OUTPUT_BUFFERS	(2)

#define FRAME_RATE_MS 		(500)


#endif /* CONFIG_H_ */
