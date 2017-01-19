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
#include <stdint.h>

#include "config.h"
#include "utils.h"

#include "playback.h"
#include "libs/ece423_sd/ece423_sd.h"

int main() {
	printf("Application Starting...\n");

	// File System

	FAT_HANDLE hFAT;
	FAT_BROWSE_HANDLE FatBrowseHandle;
	FILE_CONTEXT fileContext;
	uint8_t* currentOutputBuffer;
	uint32_t retVal;
	int keyPressed;



	retVal = SDLIB_Init(SD_CONT_BASE);
	assert(retVal, "SDLIB_Init failed!")

	hFAT = Fat_Mount();
	assert(hFAT, "Fat_Mount failed!")

	retVal = Fat_FileBrowseBegin(hFAT, &FatBrowseHandle);
	assert(retVal, "Fat_FileBrowseBegin failed!")

	ece423_video_display* display = ece423_video_display_init(
		VIDEO_DMA_CSR_NAME, DISPLAY_HEIGHT, DISPLAY_WIDTH, NUM_OUTPUT_BUFFERS);
	assert(display, "Video display init failed!")

	retVal = initKeyIrq();
	assert(display, "Failed to init keys")

	DBG_PRINT("Initialization complete!\n");




	bool fileFound = 0;

	while (Fat_FileBrowseNext(&FatBrowseHandle, &fileContext)) {
		if (Fat_CheckExtension(&fileContext, ".MPG")) {
			fileFound = 1;
			break;
		}
	}

	assert(fileFound, "No MPEG file found\n");

	DBG_PRINT("File Name is: %s, file size %d\n", Fat_GetFileName(&fileContext),
			fileContext.FileSize);

	// main loop
	while(1){
		DBG_PRINT("Starting video done\n");
		loadVideo(hFAT, Fat_GetFileName(&fileContext));
		playVideo(display);
		DBG_PRINT("Video done\n");
		closeVideo();

		keyPressed = waitForButtonPress();
		printf("Key pressed %d\n", keyPressed);
	}

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
