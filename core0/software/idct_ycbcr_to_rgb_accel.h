/*
 * ycbr_to_rgb_accel.h
 *
 *  Created on: Feb 16, 2017
 *      Author: ggowripa
 */

#ifndef YCBR_TO_RGB_ACCEL_H_
#define YCBR_TO_RGB_ACCEL_H_

#include "../../common/libs/mjpeg423/common/mjpeg423_types.h"

int init_idct_ycbcr_to_rgb_accel (void);
void idct_accel_calculate_buffer_y (void * inputBuffer, uint32_t sizeOfInputBuffer);
void idct_accel_calculate_buffer_cb (void * inputBuffer, uint32_t sizeOfInputBuffer);
void idct_accel_calculate_buffer_cr (void * inputBuffer, uint32_t sizeOfInputBuffer);
void ycbcr_to_rgb_accel_get_results (void * outputBuffer, uint32_t sizeOfOutputBuffer);

void ycbcr_to_rgb_accel_calculate_buffer(color_block_t* yBlock, color_block_t* crBlock, color_block_t* cbBlock,
		rgb_pixel_t* outputBuffer, int hCb_size, int wCb_size, int w_size);
void wait_for_ycbcr_to_rgb_finsh (void);

#endif /* YCBR_TO_RGB_ACCEL_H_ */
