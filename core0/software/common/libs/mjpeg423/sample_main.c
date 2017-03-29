//
//  main.c
//  mjpeg423app
//
//  Created by Rodolfo Pellizzoni on 12/23/13.
//  Copyright (c) 2013 __MyCompanyName__. All rights reserved.
//

#if 0
#include "encoder/mjpeg423_encoder.h"
#include "decoder/mjpeg423_decoder.h"

//input bmp base filename for the encoder. Must end in 0000.bmp
#define BMP_INPUT_FILENAME "test0000.bmp"
//output bmp base filename for the decoder. Must end in 0000.bmp
#define BMP_OUTPUT_FILENAME "testout0000.bmp"
//video file
#define VIDEO_FILENAME "video.mpg"

//number of frames to encode
#define NUM_FRAMES 5
//starting bmp number for the encoder. I.e., if the first bmp to encode is "name0002.bmp", set it to 2.
#define START_BMP 0
//bmp input stride. The encoder encodes one bmp every BMP_STRIDE.
//I.e., if set to 1, every bmp is included; if set to 2, one bmp every 2; etc.
//This is useful to adjust the frame rate compared to the original bmp sequence.
#define BMP_STRIDE 1

//max separation between successive I frames. The encoder will insert an I frame at most every MAX_I_INTERVAL frames.
#define MAX_I_INTERVAL 24
//resolution: width
#define WIDTH 280
//resolution: height
#define HEIGHT 200


int main (int argc, const char * argv[])
{
    
    mjpeg423_encode(NUM_FRAMES, START_BMP, BMP_STRIDE, MAX_I_INTERVAL, WIDTH, HEIGHT, BMP_INPUT_FILENAME, VIDEO_FILENAME);

    mjpeg423_decode(VIDEO_FILENAME, BMP_OUTPUT_FILENAME);
    
    return 0;
}

#endif

