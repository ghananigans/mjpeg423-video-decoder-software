/*
 * ycbr_to_rgb_accel.c
 *
 *  Created on: Feb 16, 2017
 *      Author: ggowripa
 */

#include "ycbcr_to_rgb_accel.h"

int init_ycbcr_to_rgb_accel (void) {
	return 1;
}

union temp {
	uint32_t data;
	rgb_pixel_t rgb;
};

void ycbcr_to_rgb_accel_calculate_buffer(color_block_t* yBlock, color_block_t* crBlock, color_block_t* cbBlock,
		rgb_pixel_t* outputBuffer, int hCb_size, int wCb_size, int w_size) {

    for (int h = 0; h < hCb_size; h++){
		for (int w = 0; w < wCb_size; w++) {
			int b = h * wCb_size + w;

			for (int y = 0; y < 8; y++){
				int index = ((h << 3) + y) * w_size + (w << 3);

				for(int x = 0; x < 8; x++){
					union temp t;
					//rgb_pixel_t pixel;
					//pixel.alpha = 0;
					//pixel.red = crBlock[b][y][x];
					//pixel.green = yBlock[b][y][x];
					//pixel.blue = cbBlock[b][y][x];
					//outputBuffer[index] = pixel;
					t.data = ((((uint32_t)crBlock[b][y][x] & 0xFF) << 16) | (((uint32_t)yBlock[b][y][x] & 0xFF) << 8) | (((uint32_t)cbBlock[b][y][x] & 0xFF)));
					outputBuffer[index] = t.rgb;
					index++;
				}
			}
		}
    }
}
