//
//  mjpeg423_encoder.h
//  mjpeg423app
//
//  Created by Rodolfo Pellizzoni on 12/23/13.
//  Copyright (c) 2013 __MyCompanyName__. All rights reserved.
//

#ifndef mjpeg423app_mjpeg423_encoder_h
#define mjpeg423app_mjpeg423_encoder_h

#include "../common/mjpeg423_types.h"

void mjpeg423_encode(uint32_t num_frames, int first, double stride, uint32_t max_I_interval, uint32_t w_size, uint32_t h_size, const char* filenamebase_in, const char* filename_out);
void rgb_to_ycbcr(int h, int w, uint32_t w_size, rgb_pixel_t* rgbblock, pcolor_block_t Y, pcolor_block_t Cb, pcolor_block_t Cr);
void fdct(pcolor_block_t block, pdct_block_t DCAC);
void quantize_I(DCTELEM *prev, pdct_block_t quant, pdct_block_t DCAC, pdct_block_t DCACq, pdct_block_t DCACq_next);
void quantize_P(pdct_block_t quant, pdct_block_t DCACq_prev, pdct_block_t DCAC, pdct_block_t DCACq);
uint32_t lossless_encode(int num_blocks, dct_block_t* DCACq, void* bitstream);

#endif
