/*
 * mpeg423_decoder.h
 *
 *  Created on: Jan 17, 2017
 *      Author: ggowripa
 */

#ifndef MPEG423_DECODER_EXT_H_
#define MPEG423_DECODER_EXT_H_

#include "../../libs/ece423_sd/ece423_sd.h"
#include <stdint.h>

typedef struct MPEG_FILE_HEADER_STRUCT
{
	uint32_t num_frames, w_size, h_size, num_iframes, payload_size;
} MPEG_FILE_HEADER;

int read_mpeg_header(FAT_FILE_HANDLE hFile, MPEG_FILE_HEADER* mpegHeader);

#endif /* MPEG423_DECODER_EXT_H_ */
