/*
 * mpeg423_decoder_ext.c
 *
 *  Created on: Jan 17, 2017
 *      Author: ggowripa
 */

#include "mpeg423_decoder_ext.h"
#include "../../../../common/config.h"
#include "../../../../common/utils.h"
#include "../../libs/mjpeg423/decoder/mjpeg423_decoder.h"
//#include "../../idct_accel.h"
//#include "../../ycbcr_to_rgb_accel.h"
#include <sys/alt_cache.h>

#ifdef TIMING_TESTS
#include "../../profile.h"
#endif // #ifdef TIMING_TESTS

#define ERROR_AND_EXIT(str) {		\
    printf("Error: %s\n", str);		\
    return 0;						\
}

int allocate_frame_buffer (MPEG_FILE_HEADER* mpegHeader, MPEG_WORKING_BUFFER* mpegFrameBuffer) {
    int hCb_size = mpegHeader->h_size/8;           // number of chrominance blocks
    int wCb_size = mpegHeader->w_size/8;
    int hYb_size = mpegHeader->h_size/8;           // number of luminance blocks. Same as chrominance in the sample app
    int wYb_size = mpegHeader->w_size/8;

    memset(mpegFrameBuffer,0,sizeof(MPEG_WORKING_BUFFER)); // initialize to NULL
    if ((mpegFrameBuffer->Yblock = malloc(hYb_size * wYb_size * 64)) == NULL) {
    	ERROR_AND_EXIT("cannot allocate Yblock");
    }

    if ((mpegFrameBuffer->Cbblock = malloc(hCb_size * wCb_size * 64)) == NULL) {
    	ERROR_AND_EXIT("cannot allocate Cbblock");
    }

    if ((mpegFrameBuffer->Crblock = malloc(hCb_size * wCb_size * 64)) == NULL) {
    	ERROR_AND_EXIT("cannot allocate Crblock");;
    }

    if ((mpegFrameBuffer->YDCAC = malloc(hYb_size * wYb_size * 64 * sizeof(DCTELEM))) == NULL) {
    	ERROR_AND_EXIT("cannot allocate YDCAC");
    }

    if ((mpegFrameBuffer->CbDCAC = malloc(hCb_size * wCb_size * 64 * sizeof(DCTELEM))) == NULL) {
    	ERROR_AND_EXIT("cannot allocate CbDCAC");
    }

    if ((mpegFrameBuffer->CrDCAC = malloc(hCb_size * wCb_size * 64 * sizeof(DCTELEM)))==NULL) {
    	ERROR_AND_EXIT("cannot allocate CrDCAC");
    }

    if ((mpegFrameBuffer->Ybitstream = malloc(hYb_size * wYb_size * 64 * sizeof(DCTELEM) +
    		2 * hCb_size * wCb_size * 64 * sizeof(DCTELEM))) == NULL) {
    	ERROR_AND_EXIT("cannot allocate bitstream");
    }

    return 1;
}

void deallocate_frame_buffer (MPEG_WORKING_BUFFER* mpegFrameBuffer) {
	free(mpegFrameBuffer->Yblock);
	free(mpegFrameBuffer->Cbblock);
	free(mpegFrameBuffer->Crblock);
	free(mpegFrameBuffer->YDCAC);
	free(mpegFrameBuffer->CbDCAC);
	free(mpegFrameBuffer->CrDCAC);
	free(mpegFrameBuffer->Ybitstream);

	memset(mpegFrameBuffer,0,sizeof(MPEG_WORKING_BUFFER)); // Reset to NULL
}

int read_next_frame (FAT_FILE_HANDLE hFile, MPEG_FILE_HEADER* mpegHeader, MPEG_WORKING_BUFFER* mpegFrameBuffer, rgb_pixel_t* outputBuffer) {
	uint32_t frame_header[4];
    uint32_t Ysize, Cbsize, frame_size, frame_type;

    int hCb_size = mpegHeader->h_size/8;           //number of chrominance blocks
    int wCb_size = mpegHeader->w_size/8;
    int hYb_size = mpegHeader->h_size/8;           //number of luminance blocks. Same as chrominance in the sample app
    int wYb_size = mpegHeader->w_size/8;

	//read frame payload
	if (!Fat_FileRead(hFile, frame_header, 4*sizeof(uint32_t))) {
		ERROR_AND_EXIT("cannot read frame header");
	}

	frame_size  = frame_header[0];
	frame_type  = frame_header[1];
	Ysize       = frame_header[2];
	Cbsize      = frame_header[3];

	DBG_PRINT("    MPG Frame size: %d: \n", frame_size);
	DBG_PRINT("    MPG Frame type: 0x%x: \n", frame_type);

#ifdef SIZE_TEST_SD_READ_OUTPUT
	PROFILE_SIZE(SIZE_TEST_SD_READ_OUTPUT, frame_type, frame_size);
#endif

#ifdef TIMING_TEST_SD_READ
	PROFILE_TIME_START(TIMING_TEST_SD_READ, frame_type);
#endif // #ifdef TIMING_TEST_SD_READ

	bool read = Fat_FileRead(hFile, mpegFrameBuffer->Ybitstream, frame_size - 4 * sizeof(uint32_t));

#ifdef TIMING_TEST_SD_READ
	PROFILE_TIME_END(TIMING_TEST_SD_READ, frame_type);
#endif // #ifdef TIMING_TEST_SD_READ

	if (!read) {
		ERROR_AND_EXIT("cannot read frame data");
	}

	//set the Cb and Cr bitstreams to point to the right location
	mpegFrameBuffer->Cbbitstream = mpegFrameBuffer->Ybitstream + Ysize;
	mpegFrameBuffer->Crbitstream = mpegFrameBuffer->Cbbitstream + Cbsize;

#ifdef SIZE_TEST_LOSSLESS_Y_INPUT
	PROFILE_SIZE(SIZE_TEST_LOSSLESS_Y_INPUT, frame_type, Ysize);
#endif

	//lossless decoding
#ifdef TIMING_TEST_LOSSLESS_Y
	PROFILE_TIME_START(TIMING_TEST_LOSSLESS_Y, frame_type);
#endif // #ifdef TIMING_TEST_LOSSLESS_Y

	lossless_decode(hYb_size*wYb_size, mpegFrameBuffer->Ybitstream, mpegFrameBuffer->YDCAC, Yquant, frame_type);

#ifdef TIMING_TEST_LOSSLESS_Y
	PROFILE_TIME_END(TIMING_TEST_LOSSLESS_Y, frame_type);
#endif // #ifdef TIMING_TEST_LOSSLESS_Y

#ifdef SIZE_TEST_LOSSLESS_CB_INPUT
	PROFILE_SIZE(SIZE_TEST_LOSSLESS_CB_INPUT, frame_type, Cbsize);
#endif

#ifdef TIMING_TEST_LOSSLESS_CB
	PROFILE_TIME_START(TIMING_TEST_LOSSLESS_CB, frame_type);
#endif // #ifdef TIMING_TEST_LOSSLESS_CB

	lossless_decode(hCb_size*wCb_size, mpegFrameBuffer->Cbbitstream, mpegFrameBuffer->CbDCAC, Cquant, frame_type);

#ifdef TIMING_TEST_LOSSLESS_CB
	PROFILE_TIME_END(TIMING_TEST_LOSSLESS_CB, frame_type);
#endif // #ifdef TIMING_TEST_LOSSLESS_CB

#ifdef SIZE_TEST_LOSSLESS_CR_INPUT
	PROFILE_SIZE(SIZE_TEST_LOSSLESS_CR_INPUT, frame_type, frame_size - 4 * sizeof(uint32_t) - Cbsize - Ysize);
#endif

#ifdef TIMING_TEST_LOSSLESS_CR
	PROFILE_TIME_START(TIMING_TEST_LOSSLESS_CR, frame_type);
#endif // #ifdef TIMING_TEST_LOSSLESS_CR

	lossless_decode(hCb_size*wCb_size, mpegFrameBuffer->Crbitstream, mpegFrameBuffer->CrDCAC, Cquant, frame_type);

#ifdef TIMING_TEST_LOSSLESS_CR
	PROFILE_TIME_END(TIMING_TEST_LOSSLESS_CR, frame_type);
#endif // #ifdef TIMING_TESTS

#ifdef IDCT_HW_ACCEL
	//
	// Flush data cache
	//
	//alt_dcache_flush_all();
#endif // #ifdef IDCT_HW_ACCEL

	//idct
#ifdef TIMING_TEST_IDCT_ONE_FRAME
	PROFILE_TIME_START(TIMING_TEST_IDCT_ONE_FRAME, 0);
#endif // #ifdef TIMING_TEST_IDCT_ONE_FRAME


#ifdef TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT
#ifndef TIMING_TEST_IDCT_ONE_FRAME
		PROFILE_TIME_START(TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT, 0);
#endif // #ifndef TIMING_TEST_IDCT_ONE_FRAME
#endif // #ifdef TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT

		//
		// Single call to idct hw accelerator which will push through ALL
		// pixel data
		//
		//idct_accel_calculate_buffer((uint32_t*)mpegFrameBuffer->YDCAC,
		//    			(uint32_t*)mpegFrameBuffer->Yblock,
		//    			hYb_size*wYb_size*sizeof(dct_block_t), hYb_size*wYb_size*sizeof(color_block_t));

		/*
	for(int b = 0; b < hYb_size*wYb_size; b++) {

#ifdef TIMING_TEST_IDCT_ONE_8_X_8_BLOCK
#ifndef TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT
#ifndef TIMING_TEST_IDCT_ONE_FRAME
    PROFILE_TIME_START(TIMING_TEST_IDCT_ONE_8_X_8_BLOCK, 0);
#endif // #ifndef TIMING_TEST_IDCT_ONE_FRAME
#endif // #ifndef TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT
#endif // #ifdef TIMING_TEST_IDCT_ONE_8_X_8_BLOCK

    	idct_accel_calculate_buffer((uint32_t*)&mpegFrameBuffer->YDCAC[b],
    			(uint32_t*)&mpegFrameBuffer->Yblock[b],
    			sizeof(mpegFrameBuffer->YDCAC[b]), sizeof(mpegFrameBuffer->Yblock[b]));

#ifdef TIMING_TEST_IDCT_ONE_8_X_8_BLOCK
#ifndef TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT
#ifndef TIMING_TEST_IDCT_ONE_FRAME
    PROFILE_TIME_END(TIMING_TEST_IDCT_ONE_8_X_8_BLOCK, 0);
#endif // #ifndef TIMING_TEST_IDCT_ONE_FRAME
#endif // #ifndef TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT
#endif // #ifdef TIMING_TEST_IDCT_ONE_8_X_8_BLOCK
	}
*/

#ifdef TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT
#ifndef TIMING_TEST_IDCT_ONE_FRAME
		PROFILE_TIME_END(TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT, 0);
#endif // #ifndef TIMING_TEST_IDCT_ONE_FRAME
#endif // #ifdef TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT

#ifdef TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT
#ifndef TIMING_TEST_IDCT_ONE_FRAME
		PROFILE_TIME_START(TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT, 0);
#endif // #ifndef TIMING_TEST_IDCT_ONE_FRAME
#endif // #ifdef TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT

		//
		// Single call to idct hw accelerator which will push through ALL
		// pixel data
		//
		//idct_accel_calculate_buffer((uint32_t*)mpegFrameBuffer->CbDCAC,
		//				(uint32_t*)mpegFrameBuffer->Cbblock,
		//				hCb_size*wCb_size*sizeof(dct_block_t), hCb_size*wCb_size*sizeof(color_block_t));
		/*
	for(int b = 0; b < hCb_size*wCb_size; b++) {

#ifdef TIMING_TEST_IDCT_ONE_8_X_8_BLOCK
#ifndef TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT
#ifndef TIMING_TEST_IDCT_ONE_FRAME
    PROFILE_TIME_START(TIMING_TEST_IDCT_ONE_8_X_8_BLOCK, 0);
#endif // #ifndef TIMING_TEST_IDCT_ONE_FRAME
#endif // #ifndef TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT
#endif // #ifdef TIMING_TEST_IDCT_ONE_8_X_8_BLOCK

		idct_accel_calculate_buffer((uint32_t*)&mpegFrameBuffer->CbDCAC[b],
				(uint32_t*)&mpegFrameBuffer->Cbblock[b],
				sizeof(mpegFrameBuffer->CbDCAC[b]), sizeof(mpegFrameBuffer->Cbblock[b]));

#ifdef TIMING_TEST_IDCT_ONE_8_X_8_BLOCK
#ifndef TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT
#ifndef TIMING_TEST_IDCT_ONE_FRAME
    PROFILE_TIME_END(TIMING_TEST_IDCT_ONE_8_X_8_BLOCK, 0);
#endif // #ifndef TIMING_TEST_IDCT_ONE_FRAME
#endif // #ifndef TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT
#endif // #ifdef TIMING_TEST_IDCT_ONE_8_X_8_BLOCK
	}
*/

#ifdef TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT
#ifndef TIMING_TEST_IDCT_ONE_FRAME
		PROFILE_TIME_END(TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT, 0);
#endif // #ifndef TIMING_TEST_IDCT_ONE_FRAME
#endif // #ifdef TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT

#ifdef TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT
#ifndef TIMING_TEST_IDCT_ONE_FRAME
		PROFILE_TIME_START(TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT, 0);
#endif // #ifndef TIMING_TEST_IDCT_ONE_FRAME
#endif // #ifdef TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT

		//
		// Single call to idct hw accelerator which will push through ALL
		// pixel data
		//
		//idct_accel_calculate_buffer((uint32_t*)mpegFrameBuffer->CrDCAC,
		//				(uint32_t*)mpegFrameBuffer->Crblock,
		//				hCb_size*wCb_size*sizeof(dct_block_t), hCb_size*wCb_size*sizeof(color_block_t));

		/*
	for(int b = 0; b < hCb_size*wCb_size; b++) {

#ifdef TIMING_TEST_IDCT_ONE_8_X_8_BLOCK
#ifndef TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT
#ifndef TIMING_TEST_IDCT_ONE_FRAME
    PROFILE_TIME_START(TIMING_TEST_IDCT_ONE_8_X_8_BLOCK, 0);
#endif // #ifndef TIMING_TEST_IDCT_ONE_FRAME
#endif // #ifndef TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT
#endif // #ifdef TIMING_TEST_IDCT_ONE_8_X_8_BLOCK

		idct_accel_calculate_buffer((uint32_t*)&mpegFrameBuffer->CrDCAC[b],
				(uint32_t*)&mpegFrameBuffer->Crblock[b],
				sizeof(mpegFrameBuffer->CrDCAC[b]), sizeof(mpegFrameBuffer->Crblock[b]));

#ifdef TIMING_TEST_IDCT_ONE_8_X_8_BLOCK
#ifndef TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT
#ifndef TIMING_TEST_IDCT_ONE_FRAME
    PROFILE_TIME_END(TIMING_TEST_IDCT_ONE_8_X_8_BLOCK, 0);
#endif // #ifndef TIMING_TEST_IDCT_ONE_FRAME
#endif // #ifndef TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT
#endif // #ifdef TIMING_TEST_IDCT_ONE_8_X_8_BLOCK
	}
*/

#ifdef TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT
#ifndef TIMING_TEST_IDCT_ONE_FRAME
		PROFILE_TIME_END(TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT, 0);
#endif // #ifndef TIMING_TEST_IDCT_ONE_FRAME
#endif // #ifdef TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT

		//
		// Wait for all Idct calculations to finish
		//
		//wait_for_idct_finsh();

#ifdef TIMING_TEST_IDCT_ONE_FRAME
	PROFILE_TIME_END(TIMING_TEST_IDCT_ONE_FRAME, 0);
#endif // #ifdef TIMING_TEST_IDCT_ONE_FRAME

#ifndef IDCT_HW_ACCEL
#ifdef YCBCR_TO_RGB_HW_ACCEL
	//
	// Flush data cache
	//
	alt_dcache_flush_all();
#endif // #ifdef YCBCR_TO_RGB_HW_ACCEL
#endif // #ifdef IDCT_HW_ACCEL

#ifdef TIMING_TEST_YCBCR_TO_RGB_ONE_FRAME
	PROFILE_TIME_START(TIMING_TEST_YCBCR_TO_RGB_ONE_FRAME, 0);
#endif // #ifdef TIMING_TEST_YCBCR_TO_RGB_ONE_FRAME

	//ycbcr_to_rgb_accel_calculate_buffer(mpegFrameBuffer->Yblock, mpegFrameBuffer->Crblock, mpegFrameBuffer->Cbblock,
	//		outputBuffer, hCb_size, wCb_size, mpegHeader->w_size);

	//
	// Wait for module to finish
	//
	//wait_for_ycbcr_to_rgb_finsh();

	/*for (int h = 0; h < hCb_size; h++){
		for (int w = 0; w < wCb_size; w++) {
			int b = h * wCb_size + w;
#ifdef TIMING_TEST_YCBCR_TO_RGB_8_X_8_BLOCK
#ifndef TIMING_TEST_YCBCR_TO_RGB_ONE_FRAME
    PROFILE_TIME_START(TIMING_TEST_YCBCR_TO_RGB_8_X_8_BLOCK, 0);
#endif // #ifndef TIMING_TEST_YCBCR_TO_RGB_ONE_FRAME
#endif // #ifdef TIMING_TEST_YCBCR_TO_RGB_8_X_8_BLOCK

			ycbcr_to_rgb(h << 3, w << 3, mpegHeader->w_size, mpegFrameBuffer->Yblock[b],
					mpegFrameBuffer->Cbblock[b], mpegFrameBuffer->Crblock[b], outputBuffer);

#ifdef TIMING_TEST_YCBCR_TO_RGB_8_X_8_BLOCK
#ifndef TIMING_TEST_YCBCR_TO_RGB_ONE_FRAME
    PROFILE_TIME_END(TIMING_TEST_YCBCR_TO_RGB_8_X_8_BLOCK, 0);
#endif // #ifndef TIMING_TEST_YCBCR_TO_RGB_ONE_FRAME
#endif // #ifdef TIMING_TEST_YCBCR_TO_RGB_8_X_8_BLOCK
		}
	}
*/

#ifdef TIMING_TEST_YCBCR_TO_RGB_ONE_FRAME
	PROFILE_TIME_END(TIMING_TEST_YCBCR_TO_RGB_ONE_FRAME, 0);
#endif // #ifdef TIMING_TEST_YCBCR_TO_RGB_ONE_FRAME


	return 1;
}
