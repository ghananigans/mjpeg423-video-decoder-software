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
#include <stdio.h>
#include <system.h>
#include <stdint.h>
#include "libs/ece423_vid_ctl/ece423_vid_ctl.h"
#include "common/config.h"
#include "common/utils.h"
#include "key_controls.h"
#include "idct_ycbcr_to_rgb_accel.h"
#include "common/mailbox/mailbox.h"
#include "playback.h"
#include <stdbool.h>

#define PLAY_PAUSE_VIDEO_BUTTON		(1)
#define LOAD_NEXT_VIDEO_BUTTON		(2)
#define FAST_FORWARD_VIDEO_BUTTON	(4)
#define REWIND_VIDEO_BUTTON			(8)

static test (void) {
	return 1;
}

static void doWork (void) {
	int retVal;
	int keyPressed;
	bool currentlyPlaying;

	while (1) {
		//
		// Load video related info
		//
		DBG_PRINT("Loading video...\n");
		loadVideo();

		//
		// Play video and handle
		// push button presses
		//
		while (1) {
			DBG_PRINT("Press Push Button 0 to play video\n");

			keyPressed = waitForButtonPress();
			DBG_PRINT("Key pressed %d\n", keyPressed);

			currentlyPlaying = isVideoPlaying();

			if (keyPressed & PLAY_PAUSE_VIDEO_BUTTON) {
				DBG_PRINT("Play/Pause button pressed\n");
				DBG_PRINT("Currently %s, Going to %s\n",
						currentlyPlaying ? "Playing" : "Paused",
						currentlyPlaying ? "Pause" : "Play");

				if (currentlyPlaying)
				{
					// Currently Playing so go to beginning of this loop to wait for
					// Push button 0 to be pressed.
					// If not currently playing (paused) continue on to play
					pauseVideo();
					continue;
				}
			} else if (keyPressed & LOAD_NEXT_VIDEO_BUTTON) {
				DBG_PRINT("Load next video button pressed\n");

				break;
			} else if (keyPressed & FAST_FORWARD_VIDEO_BUTTON) {
				DBG_PRINT("Fast forward button pressed\n");
				continue; // NULL IMPLEMENTATION

				/*retVal = fastForwardVideo();

				if (!retVal) {
					//
					// Close to the end of the video
					// Just stop the video
					//
					pauseVideo();
					continue;
				}

				// Preview the video
				previewVideo();

				if (!currentlyPlaying) {
					// If not currently playing, then go back to
					// waiting for user input
					continue;
				}*/
			} else if (keyPressed & REWIND_VIDEO_BUTTON) {
				DBG_PRINT("Rewind button pressed\n");
				continue; // NULL IMPLEMENTATION

				/*rewindVideo();
				previewVideo();

				if (!currentlyPlaying) {
					// If not currently playing, then go back to
					// waiting for user input
					continue;
				}(*/
			}

			DBG_PRINT("Playing video\n");
			//playVideo(&buttonHasBeenPressed); // Can stop because video ended OR
			playVideo(&test);
			DBG_PRINT("Video stopped\n");
		}

		closeVideo();
	}
}

int main () {
	int retVal;

	printf("Application Starting (Master Core - Core 0)...\n");

	//
	// Init video display using ece423 video api
	//
	ece423_video_display* display = ece423_video_display_init(
		VIDEO_DMA_CSR_NAME, DISPLAY_HEIGHT, DISPLAY_WIDTH, NUM_OUTPUT_BUFFERS);
	assert(display, "Video display init failed!");

	//
	// Init Keypress interrupts
	//
	retVal = initKeyIrq();
	assert(retVal, "Failed to init keys");

	//
	// Init IDCT accel
	//
	retVal = init_idct_ycbcr_to_rgb_accel();
	assert(retVal, "Failed to init idct and ycbcr_to_rgb accel!\n");
	//test_idct_accel();

	//
	// Init playback
	//
	retVal = initPlayback(display);
	assert(retVal, "Failed to init playback");

	//
	// Init Mailbox
	//
	retVal = init_send_mailbox(MAILBOX_SIMPLE_CPU0_TO_CPU1_NAME);
	assert(retVal, "Failed to init send mailbox!");

	retVal = init_recv_mailbox(MAILBOX_SIMPLE_CPU1_TO_CPU0_NAME);
	assert(retVal, "Failed to init recv mailbox!");

	DBG_PRINT("Initialization complete on Core 0 (Master Core)!\n");

	doWork();

	return 0;
}

/*
#ifdef TIMING_TESTS
#include "profile.h"
#endif // #ifdef TIMING_TESTS


static void doWork (FAT_HANDLE hFAT, FAT_BROWSE_HANDLE* FatBrowseHandle, ece423_video_display *display) {
	FILE_CONTEXT fileContext;
	int retVal;
	int keyPressed;
	bool fileFound;
	bool currentlyPlaying;

	//
	// Main loop
	//
	// Steps:
	//   1) Find next file
	//   2) Handle button presses and play video
	//   3) Close video if needed
	//   4) Repeat (back to 1)
	//
	while (1) {
		//
		// Reset fileFound flag to 0 (false -- not found)
		//
		fileFound = 0;

		//
		// Try to find the next available MPG file
		//
		DBG_PRINT("Finding next file to play\n");
		while (Fat_FileBrowseNext(FatBrowseHandle, &fileContext)) {
			//
			// Check if the file is a .MPG file
			//
			if (Fat_CheckExtension(&fileContext, ".MPG")) {
				DBG_PRINT("Found an MPG files!\n");
				fileFound = 1;
				break;
			}
		}

		// Assume that if file not found
		// we are at the end of the directory
		//
		// TODO: Don't make this assumption
		if (!fileFound) {
			// End of FAT system, so loop back to beginning
			DBG_PRINT("Reached end of file list; Re-Begin File browse\n");
			retVal = Fat_FileBrowseBegin(hFAT, FatBrowseHandle);
			assert(retVal, "Fat_FileBrowseBegin failed!");
			continue;
		}

		assert(fileFound, "No MPEG file found\n");

		DBG_PRINT("File Found; File Name is: %s, file size %d\n", Fat_GetFileName(&fileContext),
				fileContext.FileSize);

		//
		// Load video related info
		//
		DBG_PRINT("Loading video...\n");
		loadVideo(hFAT, Fat_GetFileName(&fileContext));

#ifdef TIMING_TESTS
		//
		// init Profiling stuff
		//
		retVal = initProfile();
		assert(retVal == 0, "Profile init failed!");
#endif

		//
		// Preview the video
		//
		previewVideo();

		//
		// Play video and handle
		// push button presses
		//
		while (1) {
			DBG_PRINT("Press Push Button 0 to play video\n");

			keyPressed = waitForButtonPress();
			DBG_PRINT("Key pressed %d\n", keyPressed);

			currentlyPlaying = isVideoPlaying();

			if (keyPressed & PLAY_PAUSE_VIDEO_BUTTON) {\
				DBG_PRINT("Play/Pause button pressed\n");
				DBG_PRINT("Currently %s, Going to %s\n",
						currentlyPlaying ? "Playing" : "Paused",
						currentlyPlaying ? "Pause" : "Play");

				if (currentlyPlaying)
				{
					// Currently Playing so go to beginning of this loop to wait for
					// Push button 0 to be pressed.
					// If not currently playing (paused) continue on to play
					pauseVideo();
					continue;
				}
			} else if (keyPressed & LOAD_NEXT_VIDEO_BUTTON) {
				DBG_PRINT("Load next video button pressed\n");

				break;
			} else if (keyPressed & FAST_FORWARD_VIDEO_BUTTON) {
				DBG_PRINT("Fast forward button pressed\n");

				retVal = fastForwardVideo();

				if (!retVal) {
					//
					// Close to the end of the video
					// Just stop the video
					//
					pauseVideo();
					continue;
				}

				// Preview the video
				previewVideo();

				if (!currentlyPlaying) {
					// If not currently playing, then go back to
					// waiting for user input
					continue;
				}
			} else if (keyPressed & REWIND_VIDEO_BUTTON) {
				DBG_PRINT("Rewind button pressed\n");

				rewindVideo();
				previewVideo();

				if (!currentlyPlaying) {
					// If not currently playing, then go back to
					// waiting for user input
					continue;
				}
			}

			DBG_PRINT("Playing video\n");
			playVideo(&buttonHasBeenPressed); // Can stop because video ended OR

			DBG_PRINT("Video stopped\n");

#ifdef TIMING_TESTS
			for (int i = 0; i < NUM_TIMING_TESTS; ++i) {
				for (int k = 0; k < 2; ++k) {
					PROFILE_TIME_PRINT(i, k);
				}
			}
#endif

#ifdef SIZE_TESTS
			for (int i = 0; i < NUM_SIZE_TESTS; ++i) {
				for (int k = 0; k < 2; ++k) {
					PROFILE_SIZE_PRINT(i, k);
				}
			}
#endif
		}

		closeVideo();
	}
}

int main() {
	printf("Application Starting (Master Core - Core 0)...\n");

	// File System
	FAT_HANDLE hFAT;
	FAT_BROWSE_HANDLE FatBrowseHandle;
	int retVal;

#ifdef TIMING_TESTS
	//
	// init Profiling stuff
	//
	retVal = initProfile();
	assert(retVal == 0, "Profile init failed!");
#endif

#ifdef TIMING_TEST_EMPTY
	int timingCount;

	for (timingCount = 10; timingCount != 0; --timingCount) {
		PROFILE_TIME_START(TIMING_TEST_EMPTY, 0);
		PROFILE_TIME_END(TIMING_TEST_EMPTY, 0);
	}

	PROFILE_TIME_PRINT(TIMING_TEST_EMPTY, 0);

#endif // #ifdef TIMING_TEST_EMPTY

	//
	// Init video display using ece423 video api
	//
	ece423_video_display* display = ece423_video_display_init(
		VIDEO_DMA_CSR_NAME, DISPLAY_HEIGHT, DISPLAY_WIDTH, NUM_OUTPUT_BUFFERS);
	assert(display, "Video display init failed!");

	//
	// Init Keypress interrupts
	//
	retVal = initKeyIrq();
	assert(retVal, "Failed to init keys");

	//
	// Init IDCT accel
	//
	retVal = init_idct_accel();
	assert(retVal, "Failed to init idct accel!\n");
	//test_idct_accel();

	//
	// Init ycbcr_to_rgb accell
	//
	retVal = init_ycbcr_to_rgb_accel();
	assert(retVal, "Failed to ycbcr_to_rgb accel!\n");

	//
	// Init playback
	//
	retVal = initPlayback(display);
	assert(retVal, "Failed to init playback");

	DBG_PRINT("Initialization complete on Core 0 (Master Core)!\n");

	doWork(hFAT, &FatBrowseHandle, display);

	return 0;
}
*/
