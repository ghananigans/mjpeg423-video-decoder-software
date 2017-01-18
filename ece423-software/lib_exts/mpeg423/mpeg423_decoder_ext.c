/*
 * mpeg423_decoder_ext.c
 *
 *  Created on: Jan 17, 2017
 *      Author: ggowripa
 */

#include "mpeg423_decoder_ext.h"

#define ERROR_AND_EXIT(str)  {       \
    printf("Error: %s\n", str);      \
    return 0;                        \
}

int read_mpeg_header(FAT_FILE_HANDLE hFile, MPEG_FILE_HEADER* mpegHeader) {
	//header and payload info
	uint32_t file_header[5];

	// move to beginning of file
	if(!Fat_FileSeek(hFile, FILE_SEEK_BEGIN, 0))
	{
		printf("Failed to seek file\n");
		return 0;
	}

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

int allocate_frame_buffer(MPEG_FILE_HEADER* mpegHeader, MPEG_WORKING_BUFFER* mpegFrameBuffer)
{
    int hCb_size = mpegHeader->h_size/8;           //number of chrominance blocks
    int wCb_size = mpegHeader->w_size/8;
    int hYb_size = mpegHeader->h_size/8;           //number of luminance blocks. Same as chrominance in the sample app
    int wYb_size = mpegHeader->w_size/8;

    memset(mpegFrameBuffer,0,sizeof(MPEG_WORKING_BUFFER)); //initialize to NULL
    if((mpegFrameBuffer->Yblock = malloc(hYb_size * wYb_size * 64))==NULL) ERROR_AND_EXIT("cannot allocate Yblock");
    if((mpegFrameBuffer->Cbblock = malloc(hCb_size * wCb_size * 64))==NULL) ERROR_AND_EXIT("cannot allocate Cbblock");
    if((mpegFrameBuffer->Crblock = malloc(hCb_size * wCb_size * 64))==NULL) ERROR_AND_EXIT("cannot allocate Crblock");;
    if((mpegFrameBuffer->YDCAC = malloc(hYb_size * wYb_size * 64 * sizeof(DCTELEM)))==NULL) ERROR_AND_EXIT("cannot allocate YDCAC");
    if((mpegFrameBuffer->CbDCAC = malloc(hCb_size * wCb_size * 64 * sizeof(DCTELEM)))==NULL) ERROR_AND_EXIT("cannot allocate CbDCAC");
    if((mpegFrameBuffer->CrDCAC = malloc(hCb_size * wCb_size * 64 * sizeof(DCTELEM)))==NULL) ERROR_AND_EXIT("cannot allocate CrDCAC");

    if((mpegFrameBuffer->Ybitstream = malloc(hYb_size * wYb_size * 64 * sizeof(DCTELEM) + 2 * hCb_size * wCb_size * 64 * sizeof(DCTELEM)))==NULL) ERROR_AND_EXIT("cannot allocate bitstream");

    return 1;
}

void deallocate_frame_buffer(MPEG_WORKING_BUFFER* mpegFrameBuffer)
{
	free(mpegFrameBuffer->Yblock);
	free(mpegFrameBuffer->Cbblock);
	free(mpegFrameBuffer->Crblock);
	free(mpegFrameBuffer->YDCAC);
	free(mpegFrameBuffer->CbDCAC);
	free(mpegFrameBuffer->CrDCAC);

	free(mpegFrameBuffer->Ybitstream);

	memset(mpegFrameBuffer,0,sizeof(MPEG_WORKING_BUFFER)); //initialize to NULL
}

int read_next_frame(FAT_FILE_HANDLE hFile, MPEG_FILE_HEADER* mpegHeader, MPEG_WORKING_BUFFER* mpegFrameBuffer, rgb_pixel_t* outputBuffer)
{
	uint32_t frame_header[4];
    uint32_t Ysize, Cbsize, frame_size, frame_type;

    int hCb_size = mpegHeader->h_size/8;           //number of chrominance blocks
    int wCb_size = mpegHeader->w_size/8;
    int hYb_size = mpegHeader->h_size/8;           //number of luminance blocks. Same as chrominance in the sample app
    int wYb_size = mpegHeader->w_size/8;

	//read frame payload
	if(!Fat_FileRead(hFile, frame_header, 4*sizeof(uint32_t))) ERROR_AND_EXIT("cannot read input file2");
	frame_size  = frame_header[0];
	frame_type  = frame_header[1];
	Ysize       = frame_header[2];
	Cbsize      = frame_header[3];

	printf("Frame_size %u\n",frame_size);
	printf("Frame_type %u\n",frame_type);

	bool read = Fat_FileRead(hFile, mpegFrameBuffer->Ybitstream, frame_size - 4 * sizeof(uint32_t));

	if(!read)
		ERROR_AND_EXIT("cannot read input file3");

	//set the Cb and Cr bitstreams to point to the right location
	mpegFrameBuffer->Cbbitstream = mpegFrameBuffer->Ybitstream + Ysize;
	mpegFrameBuffer->Crbitstream = mpegFrameBuffer->Cbbitstream + Cbsize;

	//lossless decoding
	lossless_decode(hYb_size*wYb_size, mpegFrameBuffer->Ybitstream, mpegFrameBuffer->YDCAC, Yquant, frame_type);
	lossless_decode(hCb_size*wCb_size, mpegFrameBuffer->Cbbitstream, mpegFrameBuffer->CbDCAC, Cquant, frame_type);
	lossless_decode(hCb_size*wCb_size, mpegFrameBuffer->Crbitstream, mpegFrameBuffer->CrDCAC, Cquant, frame_type);

	//fdct
	for(int b = 0; b < hYb_size*wYb_size; b++) idct(mpegFrameBuffer->YDCAC[b], mpegFrameBuffer->Yblock[b]);
	for(int b = 0; b < hCb_size*wCb_size; b++) idct(mpegFrameBuffer->CbDCAC[b], mpegFrameBuffer->Cbblock[b]);
	for(int b = 0; b < hCb_size*wCb_size; b++) idct(mpegFrameBuffer->CrDCAC[b], mpegFrameBuffer->Crblock[b]);

	//ybcbr to rgb conversion
	for (int h = 0; h < hCb_size; h++)
		for (int w = 0; w < wCb_size; w++) {
			int b = h * wCb_size + w;
			ycbcr_to_rgb(h << 3, w << 3, mpegHeader->w_size, mpegFrameBuffer->Yblock[b], mpegFrameBuffer->Cbblock[b], mpegFrameBuffer->Crblock[b], outputBuffer);
		}
}
