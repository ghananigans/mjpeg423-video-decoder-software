/*
 * mpeg423_decoder_ext.c
 *
 *  Created on: Jan 17, 2017
 *      Author: ggowripa
 */

#include "mpeg423_decoder_ext.h"
#include "../../utils.h"
#include "../../libs/mjpeg423/decoder/mjpeg423_decoder.h"

#define ERROR_AND_EXIT(str) {		\
    DBG_PRINT("Error: %s\n", str);	\
    return 0;						\
}

int load_mpeg_header (FAT_FILE_HANDLE hFile, MPEG_FILE_HEADER* mpegHeader) {
	//
	// Seek to the beginning of the file
	//
	if (!Fat_FileSeek(hFile, FILE_SEEK_BEGIN, 0)) {
		printf("Failed to seek file\n");
		return 0;
	}

	//
	// Read MPEG423 File header data
	//
	if (!Fat_FileRead(hFile, (void*)mpegHeader, sizeof(MPEG_FILE_HEADER))) {
		printf("Failed to read the file.\n");
		return 0;
	}

	DBG_PRINT("MPG File header read: \n");
	DBG_PRINT("   Num frames %u\n", mpegHeader->num_frames);
	DBG_PRINT("   Width %u\n", mpegHeader->w_size);
	DBG_PRINT("   Height %u\n", mpegHeader->h_size);
	DBG_PRINT("   Num i frames %u\n", mpegHeader->num_iframes);
	DBG_PRINT("   Payload size %u\n", mpegHeader->payload_size);

	return 1;
}

int load_mpeg_trailer (FAT_FILE_HANDLE hFile, MPEG_FILE_HEADER* mpegHeader, MPEG_FILE_TRAILER* mpegTrailer) {
	mpegTrailer->num_iframes = mpegHeader->num_iframes;
	mpegTrailer->iframe_info = malloc(mpegHeader->num_iframes * sizeof(MPEG_IFRAME_INFO));

	//
	// Move to the trailer
	//
	if (!Fat_FileSeek(hFile, FILE_SEEK_BEGIN, mpegHeader->payload_size + sizeof(MPEG_FILE_HEADER))) {
		printf("Failed to seek file\n");
		return 0;
	}

	//
	// Read trailer data
	//
	if (!Fat_FileRead(hFile, (void*)mpegTrailer->iframe_info, mpegHeader->num_iframes * sizeof(MPEG_IFRAME_INFO))) {
		printf("Failed to read the trailer.\n");
		return 0;
	}

	for (int i = 0; i < mpegTrailer->num_iframes; i++) {
		DBG_PRINT("Iframe #%d\n", i);
		DBG_PRINT("    Frame Number: %d\n", mpegTrailer->iframe_info[i].frame_index);
		DBG_PRINT("    Frame payload offset: %d\n", mpegTrailer->iframe_info[i].frame_position);
	}

	//
	// Move file pointer to the beginning of the paylod
	// (Right after the header)
	//
	if (!Fat_FileSeek(hFile, FILE_SEEK_BEGIN, sizeof(MPEG_FILE_HEADER))) {
		printf("Failed to seek back to beginning of payload\n");
		return 0;
	}

	return 1;
}

void unload_mpeg_trailer (MPEG_FILE_TRAILER* mpegTrailer) {
	free(mpegTrailer->iframe_info);
	mpegTrailer->iframe_info = NULL;
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
		ERROR_AND_EXIT("cannot read input file");
	}

	frame_size  = frame_header[0];
	frame_type  = frame_header[1];
	Ysize       = frame_header[2];
	Cbsize      = frame_header[3];

	DBG_PRINT("    MPG Frame size: %d: \n", frame_size);
	DBG_PRINT("    MPG Frame type: 0x%x: \n", frame_type);

	bool read = Fat_FileRead(hFile, mpegFrameBuffer->Ybitstream, frame_size - 4 * sizeof(uint32_t));

	if (!read) {
		ERROR_AND_EXIT("cannot read input file");
	}

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
	for (int h = 0; h < hCb_size; h++){
		for (int w = 0; w < wCb_size; w++) {
			int b = h * wCb_size + w;
			ycbcr_to_rgb(h << 3, w << 3, mpegHeader->w_size, mpegFrameBuffer->Yblock[b],
					mpegFrameBuffer->Cbblock[b], mpegFrameBuffer->Crblock[b], outputBuffer);
		}
	}

	return 1;
}
