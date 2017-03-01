//
//  quantize_I.c
//  mjpeg423app
//
//  Created by Rodolfo Pellizzoni on 12/23/13.
//  Copyright (c) 2013 __MyCompanyName__. All rights reserved.
//

#include <stdio.h>
#include <math.h>
#include "mjpeg423_encoder.h"
#include "../common/util.h"

#ifndef NULL_QUANT

#define DOUBLE_QUANTIZE(x,y) (DCTELEM)( round( ((double)(x)) / ((double)(y)) ) )

void quantize_I(DCTELEM* prev, pdct_block_t quant, pdct_block_t DCAC, pdct_block_t DCACq, pdct_block_t DCACq_next)
{
    DCTELEM tmp;
    tmp = ((DCTELEM*)quant)[0];
    tmp = DOUBLE_QUANTIZE( ((DCTELEM*)DCAC)[0] , tmp);
    ((DCTELEM*)DCACq)[0] = tmp - *prev;
    *prev = tmp;
    ((DCTELEM*)DCACq_next)[0] = tmp;
    for(int count = 1; count < 64; count ++){
        tmp = (((DCTELEM*)quant)[count]);
        ((DCTELEM*)DCACq)[count] = DOUBLE_QUANTIZE( ((DCTELEM*)DCAC)[count], tmp);
        ((DCTELEM*)DCACq_next)[count] = ((DCTELEM*)DCACq)[count];
    }
}

void quantize_P(pdct_block_t quant, pdct_block_t DCACq_prev, pdct_block_t DCAC, pdct_block_t DCACq)
{
    DCTELEM tmp;
    for(int count = 0; count < 64; count ++){
        tmp = (((DCTELEM*)quant)[count]);
        tmp = DOUBLE_QUANTIZE( ((DCTELEM*)DCAC)[count], tmp);
        ((DCTELEM*)DCACq)[count] = tmp - ((DCTELEM*)DCACq_prev)[count];
        ((DCTELEM*)DCACq_prev)[count] = tmp;
    }
}

#else /* Null processing blocks */

void quantize_I(DCTELEM* prev, pdct_block_t quant, pdct_block_t DCAC, pdct_block_t DCACq, pdct_block_t DCACq_next)
{
    for(int count = 0; count < 64; count ++)
        ((DCTELEM*)DCACq)[count] = (((DCTELEM*)DCAC)[count]);
}

void quantize_P(pdct_block_t quant, pdct_block_t DCACq_prev, pdct_block_t DCAC, pdct_block_t DCACq)
{
    for(int count = 0; count < 64; count ++)
        ((DCTELEM*)DCACq)[count] = (((DCTELEM*)DCAC)[count]);
}

#endif
