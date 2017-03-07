//
//  fdct.c
//  mjpeg423app
//
//  Created by Rodolfo Pellizzoni on 12/23/13.
//  Copyright (c) 2013 __MyCompanyName__. All rights reserved.
//

#include <stdio.h>
#include "mjpeg423_encoder.h"
#include "../common/dct_math.h"
#include "../common/util.h"


#ifndef NULL_DCT

void fdct(pcolor_block_t block, pdct_block_t DCAC)
{
    int32_t tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    int32_t tmp10, tmp11, tmp12, tmp13;
    int32_t z1, z2, z3, z4, z5;
    uint8_t *inputptr;
    DCTELEM *dataptr;
    int ctr;
    SHIFT_TEMPS
    
    /* Pass 1: process rows. */
    /* Note results are scaled up by sqrt(8) compared to a true DCT; */
    /* furthermore, we scale the results by 2**PASS1_BITS. */
    
    inputptr = block[0]; 
    dataptr = DCAC[0];
    for (ctr = DCTSIZE-1; ctr >= 0; ctr--) {
        tmp0 = (int32_t)inputptr[0] + (int32_t)inputptr[7];
        tmp7 = (int32_t)inputptr[0] - (int32_t)inputptr[7];
        tmp1 = (int32_t)inputptr[1] + (int32_t)inputptr[6];
        tmp6 = (int32_t)inputptr[1] - (int32_t)inputptr[6];
        tmp2 = (int32_t)inputptr[2] + (int32_t)inputptr[5];
        tmp5 = (int32_t)inputptr[2] - (int32_t)inputptr[5];
        tmp3 = (int32_t)inputptr[3] + (int32_t)inputptr[4];
        tmp4 = (int32_t)inputptr[3] - (int32_t)inputptr[4];
        
        /* Even part per LL&M figure 1 --- note that published figure is faulty;
         * rotator "sqrt(2)*c1" should be "sqrt(2)*c6".
         */
        
        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;
        
        dataptr[0] = (DCTELEM) ((tmp10 + tmp11) << PASS1_BITS);
        dataptr[4] = (DCTELEM) ((tmp10 - tmp11) << PASS1_BITS);
        
        z1 = MULTIPLY(tmp12 + tmp13, FIX_0_541196100);
        dataptr[2] = (DCTELEM) DESCALE(z1 + MULTIPLY(tmp13, FIX_0_765366865),
                                       CONST_BITS-PASS1_BITS);
        dataptr[6] = (DCTELEM) DESCALE(z1 + MULTIPLY(tmp12, - FIX_1_847759065),
                                       CONST_BITS-PASS1_BITS);
        
        /* Odd part per figure 8 --- note paper omits factor of sqrt(2).
         * cK represents cos(K*pi/16).
         * i0..i3 in the paper are tmp4..tmp7 here.
         */
        
        z1 = tmp4 + tmp7;
        z2 = tmp5 + tmp6;
        z3 = tmp4 + tmp6;
        z4 = tmp5 + tmp7;
        z5 = MULTIPLY(z3 + z4, FIX_1_175875602); /* sqrt(2) * c3 */
        
        tmp4 = MULTIPLY(tmp4, FIX_0_298631336); /* sqrt(2) * (-c1+c3+c5-c7) */
        tmp5 = MULTIPLY(tmp5, FIX_2_053119869); /* sqrt(2) * ( c1+c3-c5+c7) */
        tmp6 = MULTIPLY(tmp6, FIX_3_072711026); /* sqrt(2) * ( c1+c3+c5-c7) */
        tmp7 = MULTIPLY(tmp7, FIX_1_501321110); /* sqrt(2) * ( c1+c3-c5-c7) */
        z1 = MULTIPLY(z1, - FIX_0_899976223); /* sqrt(2) * (c7-c3) */
        z2 = MULTIPLY(z2, - FIX_2_562915447); /* sqrt(2) * (-c1-c3) */
        z3 = MULTIPLY(z3, - FIX_1_961570560); /* sqrt(2) * (-c3-c5) */
        z4 = MULTIPLY(z4, - FIX_0_390180644); /* sqrt(2) * (c5-c3) */
        
        z3 += z5;
        z4 += z5;
        
        dataptr[7] = (DCTELEM) DESCALE(tmp4 + z1 + z3, CONST_BITS-PASS1_BITS);
        dataptr[5] = (DCTELEM) DESCALE(tmp5 + z2 + z4, CONST_BITS-PASS1_BITS);
        dataptr[3] = (DCTELEM) DESCALE(tmp6 + z2 + z3, CONST_BITS-PASS1_BITS);
        dataptr[1] = (DCTELEM) DESCALE(tmp7 + z1 + z4, CONST_BITS-PASS1_BITS);
        
        dataptr += DCTSIZE;		/* advance pointer to next row */
        inputptr += DCTSIZE;
    }
    
    /* Pass 2: process columns.
     * We remove the PASS1_BITS scaling, but leave the results scaled up
     * by an overall factor of 8.
     */
    
    dataptr = DCAC[0];
    for (ctr = DCTSIZE-1; ctr >= 0; ctr--) {
        tmp0 = dataptr[DCTSIZE*0] + dataptr[DCTSIZE*7];
        tmp7 = dataptr[DCTSIZE*0] - dataptr[DCTSIZE*7];
        tmp1 = dataptr[DCTSIZE*1] + dataptr[DCTSIZE*6];
        tmp6 = dataptr[DCTSIZE*1] - dataptr[DCTSIZE*6];
        tmp2 = dataptr[DCTSIZE*2] + dataptr[DCTSIZE*5];
        tmp5 = dataptr[DCTSIZE*2] - dataptr[DCTSIZE*5];
        tmp3 = dataptr[DCTSIZE*3] + dataptr[DCTSIZE*4];
        tmp4 = dataptr[DCTSIZE*3] - dataptr[DCTSIZE*4];
        
        /* Even part per LL&M figure 1 --- note that published figure is faulty;
         * rotator "sqrt(2)*c1" should be "sqrt(2)*c6".
         */
        
        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;
        
        dataptr[DCTSIZE*0] = (DCTELEM) DESCALE(tmp10 + tmp11, PASS1_BITS+3);
        dataptr[DCTSIZE*4] = (DCTELEM) DESCALE(tmp10 - tmp11, PASS1_BITS+3);
        
        z1 = MULTIPLY(tmp12 + tmp13, FIX_0_541196100);
        dataptr[DCTSIZE*2] = (DCTELEM) DESCALE(z1 + MULTIPLY(tmp13, FIX_0_765366865),
                                               CONST_BITS+PASS1_BITS+3);
        dataptr[DCTSIZE*6] = (DCTELEM) DESCALE(z1 + MULTIPLY(tmp12, - FIX_1_847759065),
                                               CONST_BITS+PASS1_BITS+3);
        
        /* Odd part per figure 8 --- note paper omits factor of sqrt(2).
         * cK represents cos(K*pi/16).
         * i0..i3 in the paper are tmp4..tmp7 here.
         */
        
        z1 = tmp4 + tmp7;
        z2 = tmp5 + tmp6;
        z3 = tmp4 + tmp6;
        z4 = tmp5 + tmp7;
        z5 = MULTIPLY(z3 + z4, FIX_1_175875602); /* sqrt(2) * c3 */
        
        tmp4 = MULTIPLY(tmp4, FIX_0_298631336); /* sqrt(2) * (-c1+c3+c5-c7) */
        tmp5 = MULTIPLY(tmp5, FIX_2_053119869); /* sqrt(2) * ( c1+c3-c5+c7) */
        tmp6 = MULTIPLY(tmp6, FIX_3_072711026); /* sqrt(2) * ( c1+c3+c5-c7) */
        tmp7 = MULTIPLY(tmp7, FIX_1_501321110); /* sqrt(2) * ( c1+c3-c5-c7) */
        z1 = MULTIPLY(z1, - FIX_0_899976223); /* sqrt(2) * (c7-c3) */
        z2 = MULTIPLY(z2, - FIX_2_562915447); /* sqrt(2) * (-c1-c3) */
        z3 = MULTIPLY(z3, - FIX_1_961570560); /* sqrt(2) * (-c3-c5) */
        z4 = MULTIPLY(z4, - FIX_0_390180644); /* sqrt(2) * (c5-c3) */
        
        z3 += z5;
        z4 += z5;
        
        dataptr[DCTSIZE*7] = (DCTELEM) DESCALE(tmp4 + z1 + z3,
                                               CONST_BITS+PASS1_BITS+3);
        dataptr[DCTSIZE*5] = (DCTELEM) DESCALE(tmp5 + z2 + z4,
                                               CONST_BITS+PASS1_BITS+3);
        dataptr[DCTSIZE*3] = (DCTELEM) DESCALE(tmp6 + z2 + z3,
                                               CONST_BITS+PASS1_BITS+3);
        dataptr[DCTSIZE*1] = (DCTELEM) DESCALE(tmp7 + z1 + z4,
                                               CONST_BITS+PASS1_BITS+3);
        
        dataptr++;			/* advance pointer to next column */
    }
}

#else /* Null implementation */

 
void fdct(pcolor_block_t block, pdct_block_t DCAC)
{
    for(int row = 0 ; row < 8; row ++)
        for(int column = 0; column < 8; column++)
            DCAC[row][column] = block[row][column];
}

#endif

