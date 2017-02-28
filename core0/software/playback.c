/*
 * playback.c
 *
 *  Created on: Jan 17, 2017
 *      Author: ggowripa
 */

#include "playback.h"
#include "config.h"

#include "utils.h"
#include "lib_exts/mpeg423/mpeg423_decoder_ext.h"
#include "libs/ece423_vid_ctl/ece423_vid_ctl.h"
#include "system.h"
#include "sys/alt_alarm.h"
#include "timer.h"

#define MAX_IFRAME_OFFSET (24)

typedef struct PLAYBACK_DATA_STRUCT{
	bool playing;
	uint32_t volatile currentFrame; // 0 index frames
	uint32_t processedFrame;
	FAT_FILE_HANDLE hFile;
	MPEG_FILE_HEADER mpegHeader;
	MPEG_FILE_TRAILER mpegTrailer;
	MPEG_WORKING_BUFFER mpegFrameBuffer;
	ece423_video_display* display;
}PLAYBACK_DATA;

volatile bool interuptFlag = false;
volatile bool switchFrame  = false;
static PLAYBACK_DATA playbackData;


/*
 *   Timer control for controlling the frame rate
 */

static void timerFunction (void) {
	//switchFrame = true;
	int retVal;
	retVal = ece423_video_display_switch_frames(playbackData.display);
	if (retVal != -1) {
		playbackData.currentFrame++;
	}
}

/*
 *   Private playback api
 */

static inline void playFrame (bool forcePeriodic) {
	int retVal;
	uint32_t* currentOutputBuffer;

	// Wait until a buffer is available again
	while (ece423_video_display_buffer_is_available(playbackData.display) != 0){}

	// Get a pointer to the available buffer
	currentOutputBuffer = ece423_video_display_get_buffer(playbackData.display);

	// Read and decode frame and write it to the buffer
	DBG_PRINT("Frame #%u:\n", playbackData.processedFrame);
	retVal = read_next_frame(playbackData.hFile, &playbackData.mpegHeader,
			&playbackData.mpegFrameBuffer, (void*) currentOutputBuffer);
	assert(retVal, "Failed to load next frame! %d", retVal);

	// Flag the buffer as written
	ece423_video_display_register_written_buffer(playbackData.display);

	playbackData.processedFrame++;

	// Flip to next frame
	//if (forcePeriodic) {
	//	assert(!switchFrame, "Frame rate too fast, decoding can't keep up!")
	//	while (!switchFrame){}  // wait for the frame timer to fire
	//}
	//ece423_video_display_switch_frames(playbackData.display);
	//switchFrame = false;
	//playbackData.currentFrame++;
	if (!forcePeriodic) {
		ece423_video_display_switch_frames(playbackData.display);
		playbackData.currentFrame++;
	}
}

static void seekFrame (uint32_t frameIndex, uint32_t framePosition) {
	bool error;

	DBG_PRINT("Move to frame #%u from current frame #%u\n",
			frameIndex,
			playbackData.currentFrame);

	playbackData.currentFrame = frameIndex;
	playbackData.processedFrame = frameIndex;

	//
	// Move file to frame
	//
	error = Fat_FileSeek(playbackData.hFile, FILE_SEEK_BEGIN,
			framePosition);
	assert(error, "Failed to seek file\n");
}

/*
 * 	 Public playback api
 */

void loadVideo (FAT_HANDLE hFat, char* filename) {
	int retVal;

	playbackData.playing = false;
	playbackData.currentFrame = 0;
	playbackData.processedFrame = 0;

	// opening the file
	playbackData.hFile = Fat_FileOpen(hFat, filename);
	assert(playbackData.hFile, "Error in opening file!\n")

	load_mpeg_header(playbackData.hFile, &playbackData.mpegHeader);
	assert(playbackData.mpegHeader.w_size == DISPLAY_WIDTH, "Video file width unrecognized\n");
	assert(playbackData.mpegHeader.h_size == DISPLAY_HEIGHT, "Video file height unrecognized\n");

	load_mpeg_trailer(playbackData.hFile, &playbackData.mpegHeader, &playbackData.mpegTrailer);

	retVal = allocate_frame_buffer(&playbackData.mpegHeader, &playbackData.mpegFrameBuffer);
	assert(retVal, "Failed to allocate frame buffer");
}

void previewVideo (void) {
	//startPlaybackFrameTimer();

	if (playbackData.currentFrame < playbackData.mpegHeader.num_frames) {
		playFrame(0);
	}

	//stopPlaybackFrameTimer();
}

void playVideo (int (*functionToStopPlayingFrames)(void)) {
	DBG_PRINT("Playing the video\n");

	playbackData.playing = playbackData.currentFrame < playbackData.mpegHeader.num_frames;
	if (!playbackData.playing) {
		DBG_PRINT("At the end of the video, can't play\n");
		return;
	}

	if (FORCE_PERIODIC) {
		startTimer();
	}

	while (playbackData.processedFrame < playbackData.mpegHeader.num_frames
			&& functionToStopPlayingFrames() == 0) {
		playFrame(FORCE_PERIODIC);
	}

	//
	// Wait until all frames are outputted
	//
	while (playbackData.currentFrame < playbackData.processedFrame);

	//
	// If there are still frames left to be played
	// we know that we are not exiting the loop due to
	// video being done; it is because of some other
	// reason (functionToStopPlayingFrames returned
	// logical true).
	//
	playbackData.playing = playbackData.currentFrame < playbackData.mpegHeader.num_frames;

	if (FORCE_PERIODIC) {
		stopTimer();
	}
}

bool isVideoPlaying (void) {
	return playbackData.playing;
}

int fastForwardVideo (void) {
	int index;
	uint32_t currentFrame;

	currentFrame = playbackData.currentFrame;

	if (playbackData.mpegHeader.num_frames - currentFrame < 120) {
		DBG_PRINT("Less than 120 frames left in the video!\n");
		return 0;
	}

	index = currentFrame / MAX_IFRAME_OFFSET;

	//
	// We know we wont go past the end of the
	// iframe trailer information since
	// we have at least 120 frames (atleats 108) between
	// current frame and last frame
	//
	while (++index < playbackData.mpegTrailer.num_iframes) {
		//
		// Check if next possible iframe's frame index is atleast 108 frames
		// from the current frame. 108 is the magic number because
		// Iframes will occur ever 24 frames (at max), we just need to be
		// 120 +- 12 frames. If there is an iframe at 108, then we can be sure
		// there will be an iframe by 132. This means we will always get
		// a fast forward by 4.5s - 5.5s (5s +- 0.5s).
		//
		if ((int)playbackData.mpegTrailer.iframe_info[index].frame_index - (int)currentFrame >= 108) {
			break;
		}
	}

	DBG_PRINT("Fast Forwarding...\n");
	seekFrame(playbackData.mpegTrailer.iframe_info[index].frame_index,
			playbackData.mpegTrailer.iframe_info[index].frame_position);

	return 1;
}

void rewindVideo (void) {
	int index;
	uint32_t currentFrame;

	currentFrame = playbackData.currentFrame;

	if (currentFrame < 120) {
		DBG_PRINT("Less than 120 frames from beginning of the video; go to start!\n");
		index = 0;
	} else {
		index = currentFrame / MAX_IFRAME_OFFSET;

		while (--index >= 0) {
			//
			// Check if next possible iframe's frame index is atleast 108 frames
			// from the current frame. 108 is the magic number because
			// Iframes will occur ever 24 frames (at max), we just need to be
			// 120 +- 12 frames. If there is an iframe at 108, then we can be sure
			// there will be an iframe by 132. This means we will always get
			// a fast forward by 4.5s - 5.5s (5s +- 0.5s).
			//
			if (currentFrame - playbackData.mpegTrailer.iframe_info[index].frame_index >= 108) {
				break;
			}
		}
	}

	DBG_PRINT("Rewinding...\n");
	seekFrame(playbackData.mpegTrailer.iframe_info[index].frame_index,
				playbackData.mpegTrailer.iframe_info[index].frame_position);
}

void pauseVideo (void) {
	DBG_PRINT("Setting playing status to false\n");

	playbackData.playing = false;
}

void closeVideo (void) {
	DBG_PRINT("Closing video\n");

	playbackData.playing = false;
	Fat_FileClose(playbackData.hFile);
	deallocate_frame_buffer(&playbackData.mpegFrameBuffer);
	unload_mpeg_trailer(&playbackData.mpegTrailer);
}

int initPlayback (ece423_video_display* display) {
	playbackData.display = display;
	return initTimer(FRAME_RATE_MS, &timerFunction);
}