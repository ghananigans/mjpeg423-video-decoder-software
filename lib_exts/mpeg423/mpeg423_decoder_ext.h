/*
 * mpeg423_decoder.h
 *
 *  Created on: Jan 17, 2017
 *      Author: ggowripa
 */

#ifndef MPEG423_DECODER_EXT_H_
#define MPEG423_DECODER_EXT_H_

#include "../../libs/ece423_sd/ece423_sd.h"
#include "../../libs/mjpeg423/common/mjpeg423_types.h"
#include <stdint.h>

#define FRAME_BUFFER_SIZE (2)

typedef struct MPEG_FILE_HEADER_STRUCT
{
	uint32_t num_frames, w_size, h_size, num_iframes, payload_size;
} MPEG_FILE_HEADER;

typedef struct MPEG_WORKING_BUFFER_STRUCT
{
	uint8_t* Ybitstream;
	uint8_t* Cbbitstream;
	uint8_t* Crbitstream;
	color_block_t* Yblock;
	color_block_t* Cbblock;
	color_block_t* Crblock;
	dct_block_t* YDCAC;
	dct_block_t* CbDCAC;
	dct_block_t* CrDCAC;
} MPEG_WORKING_BUFFER;

int read_mpeg_header(FAT_FILE_HANDLE hFile, MPEG_FILE_HEADER* mpegHeader);
int allocate_frame_buffer(MPEG_FILE_HEADER* mpegHeader, MPEG_WORKING_BUFFER* mpegFrameBuffer);
void deallocate_frame_buffer(MPEG_WORKING_BUFFER* mpegFrameBuffer);
int read_next_frame(FAT_FILE_HANDLE hFile, MPEG_FILE_HEADER* mpegHeader, MPEG_WORKING_BUFFER* mpegFrameBuffer, rgb_pixel_t* outputBuffer);

#endif /* MPEG423_DECODER_EXT_H_ */
