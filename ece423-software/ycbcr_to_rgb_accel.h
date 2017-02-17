/*
 * ycbr_to_rgb_accel.h
 *
 *  Created on: Feb 16, 2017
 *      Author: ggowripa
 */

#ifndef YCBR_TO_RGB_ACCEL_H_
#define YCBR_TO_RGB_ACCEL_H_

#include "libs/mjpeg423/decoder/mjpeg423_decoder.h"

int init_ycbcr_to_rgb_accel (void);
void ycbcr_to_rgb_accel_calculate_buffer(color_block_t* yBlock, color_block_t* crBlock, color_block_t* cbBlock,
		rgb_pixel_t* outputBuffer, int hCb_size, int wCb_size, int w_size);

#endif /* YCBR_TO_RGB_ACCEL_H_ */
