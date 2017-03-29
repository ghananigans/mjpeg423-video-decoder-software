//
//  rgb_to_ycbcr.c
//  mjpeg423app
//
//  Created by Rodolfo Pellizzoni on 12/23/13.
//  Copyright (c) 2013 __MyCompanyName__. All rights reserved.
//

#include <stdio.h>
#include "mjpeg423_encoder.h"
#include "../common/util.h"

void miniblock_rgb_to_ycbcr(int base_index, int y, int x, int addy, int addx, uint32_t w_size, 
                            rgb_pixel_t* rgbblock, pcolor_block_t YY, pcolor_block_t Cb, pcolor_block_t Cr);

/*
//perform rgb to Y'CbCr conversion. 
//Note input rgb values are in the range [0, 255] while output Y'CbCr values are in the range [-128, 127]
void rgb_to_ycbcr(int h, int w, uint32_t w_size, rgb_pixel_t* rgbblock, color_block_t* Y, pcolor_block_t Cb, pcolor_block_t Cr)
{
    pcolor_block_t YY0 = Y[0];
    pcolor_block_t YY1 = Y[1];
    pcolor_block_t YY2 = Y[2];
    pcolor_block_t YY3 = Y[3];
    for (int y = 0; y < 4; y++)
        for(int x = 0; x < 4; x++){
            int base_index = (h+y*2) * w_size + w + x*2;
            miniblock_rgb_to_ycbcr(base_index, y, x, 0, 0, w_size, rgbblock, YY0, Cb, Cr);
            base_index += 8;
            miniblock_rgb_to_ycbcr(base_index, y, x, 0, 4, w_size, rgbblock, YY1, Cb, Cr);
            base_index += 8 * w_size;
            miniblock_rgb_to_ycbcr(base_index, y, x, 4, 4, w_size, rgbblock, YY3, Cb, Cr);
            base_index -= 8;
            miniblock_rgb_to_ycbcr(base_index, y, x, 4, 0, w_size, rgbblock, YY2, Cb, Cr);
        }
}

void miniblock_rgb_to_ycbcr(int base_index, int y, int x, int addy, int addx, uint32_t w_size, 
                            rgb_pixel_t* rgbblock, pcolor_block_t YY, pcolor_block_t Cb, pcolor_block_t Cr)
{
    rgb_pixel_t TL = rgbblock[base_index];
    rgb_pixel_t TR = rgbblock[base_index + 1];
    rgb_pixel_t BL = rgbblock[base_index + w_size];
    rgb_pixel_t BR = rgbblock[base_index + w_size + 1];
    uint8_t red_avg = ((uint32_t)TL.red + (uint32_t)TR.red + (uint32_t)BL.red + (uint32_t)BR.red)/4;
    uint8_t blue_avg = ((uint32_t)TL.blue + (uint32_t)TR.blue + (uint32_t)BL.blue + (uint32_t)BR.blue)/4;
    uint8_t green_avg = ((uint32_t)TL.green + (uint32_t)TR.green + (uint32_t)BL.green + (uint32_t)BR.green)/4;
    YY[0+y*2][0+x*2] = 0.299 * TL.red + 0.587 * TL.green + 0.114 * TL.blue - 128;
    YY[0+y*2][1+x*2] = 0.299 * TR.red + 0.587 * TR.green + 0.114 * TR.blue - 128;
    YY[1+y*2][0+x*2] = 0.299 * BL.red + 0.587 * BL.green + 0.114 * BL.blue - 128;
    YY[1+y*2][1+x*2] = 0.299 * BR.red + 0.587 * BR.green + 0.114 * BR.blue - 128;
    Cb[y+addy][x+addx] = - 0.168736 * red_avg - 0.331264 * green_avg + 0.5 * blue_avg;
    Cr[y+addy][x+addx] = 0.5 * red_avg - 0.418688 * green_avg - 0.081312 * blue_avg;
}*/

#ifndef NULL_COLORCONV

void rgb_to_ycbcr(int h, int w, uint32_t w_size, rgb_pixel_t* rgbblock, pcolor_block_t Y, pcolor_block_t Cb, pcolor_block_t Cr)
{
    int index; 
    for (int y = 0; y < 8; y++){
        index = (y+h) * w_size + w;
        for(int x = 0; x < 8; x++){
            Y[y][x] = 0.299 * rgbblock[index].red + 0.587 * rgbblock[index].green + 0.114 * rgbblock[index].blue;
            Cb[y][x] = - 0.168736 * rgbblock[index].red - 0.331264 * rgbblock[index].green + 0.5 * rgbblock[index].blue + 128;
            Cr[y][x] = 0.5 * rgbblock[index].red - 0.418688 * rgbblock[index].green - 0.081312 * rgbblock[index].blue+ 128;
            index++;
        }
    }
}

#else

void rgb_to_ycbcr(int h, int w, uint32_t w_size, rgb_pixel_t* rgbblock, pcolor_block_t Y, pcolor_block_t Cb, pcolor_block_t Cr)
{
    int index; 
    for (int y = 0; y < 8; y++){
        index = (y+h) * w_size + w;
        for(int x = 0; x < 8; x++){
            Y[y][x] = ((DCTELEM)(rgbblock[index].green));
            Cb[y][x] = ((DCTELEM)(rgbblock[index].blue));
            Cr[y][x] = ((DCTELEM)(rgbblock[index].red));
            index++;
        }
    }
}

#endif
