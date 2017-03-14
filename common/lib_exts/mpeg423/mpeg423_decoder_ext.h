/*
 * mpeg423_decoder.h
 *
 *  Created on: Jan 17, 2017
 *      Author: ggowripa
 */

#ifndef MPEG423_DECODER_EXT_H_
#define MPEG423_DECODER_EXT_H_

#include "../../libs/mjpeg423/decoder/mjpeg423_decoder.h"
#include <stdint.h>

typedef struct MPEG_FILE_HEADER_STRUCT
{
	uint32_t num_frames;
	uint32_t w_size;
	uint32_t h_size;
	uint32_t num_iframes;
	uint32_t payload_size;
} MPEG_FILE_HEADER;

typedef struct MPEG_IFRAME_INFO_STRUCT
{
	uint32_t frame_index;
	uint32_t frame_position;
} MPEG_IFRAME_INFO;

typedef struct MPEG_FILE_TRAILER_STRUCT
{
	uint32_t num_iframes;
	MPEG_IFRAME_INFO* iframe_info;
} MPEG_FILE_TRAILER;

typedef struct MPEG_WORKING_BUFFER_STRUCT
{
	uint8_t volatile * Ybitstream;
	dct_block_t volatile * YDCAC;
	dct_block_t * CbDCAC;
	dct_block_t * CrDCAC;
} MPEG_WORKING_BUFFER;

#endif /* MPEG423_DECODER_EXT_H_ */

