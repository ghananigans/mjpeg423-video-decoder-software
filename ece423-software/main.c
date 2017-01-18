/*
 * "Hello World" example.
 *
 * This example prints 'Hello from Nios II' to the STDOUT stream. It runs on
 * the Nios II 'standard', 'full_featured', 'fast', and 'low_cost' example
 * designs. It runs with or without the MicroC/OS-II RTOS and requires a STDOUT
 * device in your system's hardware.
 * The memory footprint of this hosted application is ~69 kbytes by default
 * using the standard reference design.
 *
 * For a reduced footprint version of this template, and an explanation of how
 * to reduce the memory footprint for a given application, see the
 * "small_hello_world" template.
 *
 */


#define NULL_COLORCONV

#include <stdio.h>
#include <system.h>
#include "libs/ece423_sd/ece423_sd.h"
#include "lib_exts/mpeg423/mpeg423_decoder_ext.h"
#include "libs/ece423_vid_ctl/ece423_vid_ctl.h"

#include "config.h"
#include "utils.h"

typedef struct PLAYBACK_DATA_STRUCT{
	bool playing;
	uint32_t currentFrame; // 0 index frames
	FAT_FILE_HANDLE hFile;
	MPEG_FILE_HEADER mpegHeader;
	MPEG_WORKING_BUFFER mpegFrameBuffer;
}PLAYBACK_DATA;

volatile bool interuptFlag = false;
PLAYBACK_DATA playbackData;

void loadVideo(FAT_HANDLE hFat, char* filename){

	playbackData.playing = false;
	playbackData.currentFrame = 0;

	// opening the file
	playbackData.hFile = Fat_FileOpen(hFat, filename);
	if (!playbackData.hFile) {
		printf("Error in opening file!\n");
		while (1){}
	}

	read_mpeg_header(playbackData.hFile, &playbackData.mpegHeader);

	if (!allocate_frame_buffer(&playbackData.mpegHeader, &playbackData.mpegFrameBuffer)) {
		printf("Failed to allocate working buffers\n");
		while (1){}
	}
}

void playVideo(ece423_video_display* display) {
	playbackData.playing = true;
	uint32_t* currentOutputBuffer;

	while (playbackData.currentFrame < playbackData.mpegHeader.num_frames && !interuptFlag) {

		// Wait until a buffer is available again
		while (ece423_video_display_buffer_is_available(display) != 0){}

		// Get a pointer to the available buffer
		currentOutputBuffer = ece423_video_display_get_buffer(display);

		// Read and decode frame and write it to the buffer
		if (!read_next_frame(playbackData.hFile, &playbackData.mpegHeader, &playbackData.mpegFrameBuffer, currentOutputBuffer)) {
			printf("Failed to read next frame\n");
			while (1){}
		}

		// Flag the buffer as written
		ece423_video_display_register_written_buffer(display);

		//flip to next frame
		ece423_video_display_switch_frames(display);

		playbackData.currentFrame++;
	}
}

void closeVideo(){
	playbackData.playing = false;
	deallocate_frame_buffer(&playbackData.mpegFrameBuffer);
}

void print_result(bool result, char* string) {
	if (result) {
		printf("%s successful :D\n", string);
	} else {
		printf("%s failed D:\n", string);
		while (1) {
		}
	}
}

int main() {
	printf("Application Starting...\n");

	// File System

	FAT_HANDLE hFAT;
	FAT_BROWSE_HANDLE FatBrowseHandle;
	FILE_CONTEXT fileContext;
	uint8_t* currentOutputBuffer;

	print_result(SDLIB_Init(SD_CONT_BASE), "SDLIB_Init");

	hFAT = Fat_Mount();
	print_result(hFAT, "Fat_Mount");

	print_result(Fat_FileBrowseBegin(hFAT, &FatBrowseHandle),
			"Fat_FileBrowseBegin");

	bool fileFound = 0;

	while (Fat_FileBrowseNext(&FatBrowseHandle, &fileContext)) {
		if (Fat_CheckExtension(&fileContext, ".MPG")) {
			fileFound = 1;
			break;
		}
	}

	if (!fileFound) {
		printf("No MPEG file found\n");
		while (1) {}
	}

	printf("File Name is: %s, file size %d\n", Fat_GetFileName(&fileContext),
			fileContext.FileSize);

	// init video display
	ece423_video_display* display = ece423_video_display_init(
			VIDEO_DMA_CSR_NAME, DISPLAY_HEIGHT, DISPLAY_WIDTH, NUM_OUTPUT_BUFFERS);

	loadVideo(hFAT, Fat_GetFileName(&fileContext));
	playVideo(display);

	printf("Video done\n");

	closeVideo();

	printf("Program end\n");

	return 0;
}

// old code
#if 0

	// opening the file
	FAT_FILE_HANDLE hFile = Fat_FileOpen(hFAT, Fat_GetFileName(&fileContext));
	if (!hFile) {
		printf("Error in opening file!\n");
		while (1) {
		}
	}

	// read the file header
	MPEG_FILE_HEADER mpegHeader;
	read_mpeg_header(hFile, &mpegHeader);

	// init video display
	ece423_video_display* display = ece423_video_display_init(
			VIDEO_DMA_CSR_NAME, mpegHeader.w_size, mpegHeader.h_size,
			FRAME_BUFFER_SIZE);
	if (!display) {
		printf("Failed to init video display\n");
		while (1) {
		}
	}
	printf("Video DMA initialized!\n");

	// set up frame buffers
	MPEG_WORKING_BUFFER mpegFrameBuffer;
	if (!allocate_frame_buffer(&mpegHeader, &mpegFrameBuffer)) {
		printf("Failed to allocate working buffers\n");
		while (1) {
		}
	}

	for (int i = 0; i < mpegHeader.num_frames; i++) {
		// Wait until a buffer is available again
		while (ece423_video_display_buffer_is_available(display) != 0) {}

		// Get a pointer to the available buffer
		currentOutputBuffer = ece423_video_display_get_buffer(display);

		// Read and decode frame and write it to the buffer
		if (!read_next_frame(hFile, &mpegHeader, &mpegFrameBuffer, currentOutputBuffer)) {
			printf("Failed to read next frame\n");
			while (1) {
			}
		}
		printf("Generated first buffer\n");

		// Flag the buffer as written
		ece423_video_display_register_written_buffer(display);

		//flip to next frame
		printf("Switching frames\n");
		ece423_video_display_switch_frames(display);
		printf("Done Switching frames\n");
	}
#endif
