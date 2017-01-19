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

#define MAX_IFRAME_OFFSET (24)

typedef struct PLAYBACK_DATA_STRUCT{
	bool playing;
	uint32_t currentFrame; // 0 index frames
	FAT_FILE_HANDLE hFile;
	MPEG_FILE_HEADER mpegHeader;
	MPEG_FILE_TRAILER mpegTrailer;
	MPEG_WORKING_BUFFER mpegFrameBuffer;
}PLAYBACK_DATA;

volatile bool interuptFlag = false;
volatile bool switchFrame  = false;
static PLAYBACK_DATA playbackData;


/*
 *   Timer control for controlling the frame rate
 */

static alt_alarm frameTimer;

static inline alt_u32 frameTimerCallback (void* context) {
	switchFrame = true;
	return FRAME_RATE_MS;
}

static inline int startPlaybackFrameTimer (void) {
	return alt_alarm_start(&frameTimer, FRAME_RATE_MS, frameTimerCallback, NULL);
}

static inline void stopPlaybackFrameTimer (void) {
	alt_alarm_stop(&frameTimer);
}

/*
 *   Private playback api
 */

static void playFrame (ece423_video_display* display, bool forcePeriodic) {
	int retVal;
	uint32_t* currentOutputBuffer;

	// Wait until a buffer is available again
	while (ece423_video_display_buffer_is_available(display) != 0){}

	// Get a pointer to the available buffer
	currentOutputBuffer = ece423_video_display_get_buffer(display);

	// Read and decode frame and write it to the buffer
	DBG_PRINT("Frame #%u:\n", playbackData.currentFrame);
	retVal = read_next_frame(playbackData.hFile, &playbackData.mpegHeader, &playbackData.mpegFrameBuffer, (void*) currentOutputBuffer);
	assert(retVal, "Failed to load next frame!");

	// Flag the buffer as written
	ece423_video_display_register_written_buffer(display);

	// Flip to next frame
	if (forcePeriodic) {
		assert(!switchFrame, "Frame rate too fast, decoding cant keep up!")
		while (!switchFrame){}  // wait for the frame timer to fire
	}
	ece423_video_display_switch_frames(display);
	switchFrame = false;

	playbackData.currentFrame++;
}


/*
 * 	 Public playback api
 */

void loadVideo (FAT_HANDLE hFat, char* filename) {
	int retVal;

	playbackData.playing = false;
	playbackData.currentFrame = 0;

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


void previewVideo (ece423_video_display* display) {
	//startPlaybackFrameTimer();

	if (playbackData.currentFrame < playbackData.mpegHeader.num_frames) {
		playFrame(display, 0);
	}

	//stopPlaybackFrameTimer();
}

// TODO: we need to make this timer based
void playVideo (ece423_video_display* display, bool *functionToStopPlayingFrames(void)) {
	DBG_PRINT("Play the video\n");

	playbackData.playing = true;
	startPlaybackFrameTimer();

	while (playbackData.currentFrame < playbackData.mpegHeader.num_frames && !functionToStopPlayingFrames()) {
		playFrame(display, 1);
	}

	stopPlaybackFrameTimer();
}

bool isVideoPlaying (void) {
	return playbackData.playing;
}

int fastforwardVideo (void) {
	int index;
	int error;

	if (playbackData.mpegHeader.num_frames - playbackData.currentFrame < 120) {
		DBG_PRINT("Less than 120 frames left in the video!\n");
		return 0;
	}

	index = playbackData.currentFrame / MAX_IFRAME_OFFSET;

	//
	// We know we wont go past the end of the
	// iframe trailer information since
	// we have at least 120 frames between current frame
	// and last frame
	//
	while (1) {
		if ((int)playbackData.mpegTrailer.iframe_info[++index].frame_index - (int)playbackData.currentFrame > 110) {
			DBG_PRINT("Fast forwarded to frame #%u from current frame #%u\n",
					playbackData.mpegTrailer.iframe_info[index].frame_index,
					playbackData.currentFrame);

			playbackData.currentFrame = playbackData.mpegTrailer.iframe_info[index].frame_index;

			// move to fast forward-ed frame
			error = Fat_FileSeek(playbackData.hFile, FILE_SEEK_BEGIN,
					playbackData.mpegTrailer.iframe_info[index].frame_position);
			assert(error, "Failed to seek file\n");

			break;
		}
	}

	return 1;
}

void rewindVideo (void) {
	int index;
	int error;

	if (playbackData.currentFrame < 120) {
		DBG_PRINT("Less than 120 frames from beginning of the video; go to start!\n");
		index = 0;
	} else {
		index = playbackData.currentFrame / MAX_IFRAME_OFFSET;
	}

	while (index > 0) {
		if (playbackData.currentFrame - playbackData.mpegTrailer.iframe_info[--index].frame_index > 110) {
			break;
		}
	}

	DBG_PRINT("Rewind to frame #%u from current frame #%u\n",
			playbackData.mpegTrailer.iframe_info[index].frame_index,
			playbackData.currentFrame);

	playbackData.currentFrame = playbackData.mpegTrailer.iframe_info[index].frame_index;

	// move to rewinded frame
	error = Fat_FileSeek(playbackData.hFile, FILE_SEEK_BEGIN,
			playbackData.mpegTrailer.iframe_info[index].frame_position);
	assert(error, "Failed to seek file\n");
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


