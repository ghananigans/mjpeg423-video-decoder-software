//
//  lossless_encode.c
//  mjpeg423app
//
//  Created by Rodolfo Pellizzoni on 12/28/13.
//  Copyright (c) 2013 __MyCompanyName__. All rights reserved.
//

#include <stdio.h>
#include "../common/mjpeg423_types.h"
#include "mjpeg423_encoder.h"
#include "../common/util.h"


#ifndef NULL_LOSSLESS

int bitcount;
uint32_t bitbuffer;
uint8_t* stream;

void output_rest(void);
void output_DC(DCTELEM e);
void output_AC(uint32_t runlength, DCTELEM e);
void output_ZRL(void);
void output_END(void);
void output_bits(int n, uint32_t bits);
uint32_t encode_VLI(int32_t x, uint32_t* size);

//returns length of bitstream
uint32_t lossless_encode(int num_blocks, dct_block_t* DCACq, void* bitstream)
{
    bitcount = 0;
    bitbuffer = 0;
    stream = (uint8_t*)bitstream;
    
    for(int count = 0; count < num_blocks; count++){
        DCTELEM* dct = (DCTELEM*)(DCACq[count]);
        
        output_DC(dct[0]);
        
        uint32_t runlength = 0, index = 1, lastindex = 63;
        
        while(dct[zigzag_table[lastindex]] == 0 && lastindex > 0) lastindex--;
        
        while(index <= lastindex){
            while(runlength < 16 && dct[zigzag_table[index]] == 0) {
                runlength ++;
                index ++;
            }
            if(runlength == 16) output_ZRL();
            else {output_AC(runlength, dct[zigzag_table[index]]); index++;}
            runlength = 0;
        }
        if(lastindex < 63) output_END();
        
    }
    
    output_rest();
    return bitcount/8 + ((bitcount%8 == 0) ? 0 : 1);
}

//n must be less than 24
//endian-independent (creates big-endian)
void output_bits(int n, uint32_t bits)
{
    //int base = bitcount/8*8;
    int base = bitcount/8;
    bitbuffer |= (bits << (32 - bitcount%8 - n) );
    bitcount += n;
    //while(bitcount-base >= 8){
    while(bitcount/8 > base){
        //stream[base/8] = (uint8_t)(bitbuffer >> 24);
        stream[base] = (uint8_t)(bitbuffer >> 24);
        bitbuffer <<= 8;
        //base +=8;
        base++;
    }
}

void output_rest()
{
    stream[bitcount/8] = *((int8_t*)(&bitbuffer));
}

//this are not the actual functions!!!!!!!
void output_DC(DCTELEM e)
{
    uint32_t size = 0;
    if (e == 0)
        output_bits(4, size);
    else {
        uint32_t res = encode_VLI(e, &size);
        output_bits(4, size);
        output_bits(size, res);
    }
}

void output_AC(uint32_t runlength, DCTELEM e)
{
    uint32_t size;
    uint32_t res = encode_VLI(e, &size);
    output_bits(4, runlength);
    output_bits(4, size);
    output_bits(size, res);
}

void output_ZRL()
{
    output_bits(4, 15);
    output_bits(4, 0);
}

void output_END()
{
    output_bits(4, 0);
    output_bits(4, 0);
}

//return encoded amplitude and size
//this function could be better optimized...
uint32_t encode_VLI(int32_t x, uint32_t* size)
{
    if(x == 0){
        *size = 0;
        return 0;
    }else if(x >= -1 && x <= 1) *size = 1;
    else if(x >= -3 && x <= 3) *size = 2;
    else if(x >= -7 && x <= 7) *size = 3;
    else if(x >= -15 && x <= 15) *size = 4;
    else if(x >= -31 && x <= 31) *size = 5;
    else if(x >= -63 && x <= 63) *size = 6;
    else if(x >= -127 && x <= 127) *size = 7;
    else if(x >= -255 && x <= 255) *size = 8;
    else if(x >= -511 && x <= 511) *size = 9;
    else if(x >= -1023 && x <= 1023) *size = 10;
    else *size = 11;
    return (x > 0) ? x : (((uint32_t)(x-1)) & ( ((uint32_t)(-1)) >> (32 - *size) ));
}


#else
uint32_t lossless_encode(int num_blocks, dct_block_t* DCACq, void* bitstream)
{
    uint32_t len = 0;
    for(int blocks = 0; blocks < num_blocks; blocks++)
        for(int count = 0; count < 64; count ++){
            ((DCTELEM*)bitstream)[len++] = ((DCTELEM*)(DCACq[blocks]))[count];
        }
    return len*sizeof(DCTELEM);
    } 

#endif

