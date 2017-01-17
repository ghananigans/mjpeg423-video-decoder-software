/*
 * mpeg423_decoder_ext.c
 *
 *  Created on: Jan 17, 2017
 *      Author: ggowripa
 */

#include "mpeg423_decoder_ext.h"

int read_mpeg_header(FAT_FILE_HANDLE hFile, MPEG_FILE_HEADER* mpegHeader) {
	//header and payload info
	uint32_t file_header[5];

	//read header
	if(!Fat_FileRead(hFile, (void*)file_header, 5 * sizeof(uint32_t)))
	{
		printf("Failed to read the file.\n");
		return 0;
	}

	mpegHeader->num_frames = file_header[0];
	mpegHeader->w_size = file_header[1];
	mpegHeader->h_size = file_header[2];
	mpegHeader->num_iframes = file_header[3];
	mpegHeader->payload_size = file_header[4];

	printf("Decoder start. Num frames #%u\n", mpegHeader->num_frames);
	printf("Width %u\n", mpegHeader->w_size);
	printf("Height %u\n", mpegHeader->h_size);
	printf("Num i frames %u\n", mpegHeader->num_iframes);
	printf("Payload size %u\n", mpegHeader->payload_size);
}



