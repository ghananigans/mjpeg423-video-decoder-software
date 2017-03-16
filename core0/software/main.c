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

				retVal = fastForwardVideo();

				if (!retVal) {
					//
					// Close to the end of the video
					// Just stop the video
					//
					pauseVideo();
					continue;
				}

				if (!currentlyPlaying) {
					// If not currently playing, then go back to
					// waiting for user input
					continue;
				}
			} else if (keyPressed & REWIND_VIDEO_BUTTON) {
				DBG_PRINT("Rewind button pressed\n");

				rewindVideo();

				if (!currentlyPlaying) {
					// If not currently playing, then go back to
					// waiting for user input
					continue;
				}
			}

			DBG_PRINT("Playing video\n");
			playVideo(&buttonHasBeenPressed); // Can stop because video ended OR
			//playVideo(&test);
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

