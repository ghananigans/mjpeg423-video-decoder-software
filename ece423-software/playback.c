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
#include "key_controls.h"

typedef struct PLAYBACK_DATA_STRUCT{
	bool playing;
	uint32_t currentFrame; // 0 index frames
	FAT_FILE_HANDLE hFile;
	MPEG_FILE_HEADER mpegHeader;
	MPEG_WORKING_BUFFER mpegFrameBuffer;
}PLAYBACK_DATA;

volatile bool interuptFlag = false;
volatile bool switchFrame  = false;
static PLAYBACK_DATA playbackData;


/*
 *   Timer control for controlling the frame rate
 */

static alt_alarm frameTimer;

static inline alt_u32 frameTimerCallback(void* context){
	switchFrame = true;
	return FRAME_RATE_MS;
}

static inline int startPlaybackFrameTimer(){
	return alt_alarm_start(&frameTimer, FRAME_RATE_MS, frameTimerCallback, NULL);
}

static inline void stopPlaybackFrameTimer(){
	alt_alarm_stop(&frameTimer);
}

/*
 * 	 Public playback api
 */

void loadVideo(FAT_HANDLE hFat, char* filename){

	int retVal;

	playbackData.playing = false;
	playbackData.currentFrame = 0;

	// opening the file
	playbackData.hFile = Fat_FileOpen(hFat, filename);
	assert(playbackData.hFile, "Error in opening file!\n")

	read_mpeg_header(playbackData.hFile, &playbackData.mpegHeader);
	assert(playbackData.mpegHeader.w_size == DISPLAY_WIDTH, "Video file width unrecognized\n");
	assert(playbackData.mpegHeader.h_size == DISPLAY_HEIGHT, "Video file height unrecognized\n");

	retVal = allocate_frame_buffer(&playbackData.mpegHeader, &playbackData.mpegFrameBuffer);
	assert(retVal, "Failed to allocate frame buffer");
}

// TODO: we need to make this timer based
void playVideo(ece423_video_display* display) {
	playbackData.playing = true;
	uint32_t* currentOutputBuffer;
	int retVal;

	startPlaybackFrameTimer();

	while (playbackData.currentFrame < playbackData.mpegHeader.num_frames && !buttonHasBeenPressed()) {

		// Wait until a buffer is available again
		while (ece423_video_display_buffer_is_available(display) != 0){}

		// Get a pointer to the available buffer
		currentOutputBuffer = ece423_video_display_get_buffer(display);

		// Read and decode frame and write it to the buffer
		retVal = read_next_frame(playbackData.hFile, &playbackData.mpegHeader, &playbackData.mpegFrameBuffer, (void*) currentOutputBuffer);
		assert(retVal, "Failed to load next frame!");

		// Flag the buffer as written
		ece423_video_display_register_written_buffer(display);

		//flip to next frame

		assert(!switchFrame, "Frame rate too fast, decoding cant keep up!")
		while(!switchFrame){}  // wait for the frame timer to fire
		ece423_video_display_switch_frames(display);
		switchFrame = false;

		playbackData.currentFrame++;
	}

	stopPlaybackFrameTimer();
}

bool isVideoPlaying(void) {
	return playbackData.playing;
}

void pauseVideo(void) {
	playbackData.playing = false;
}

void closeVideo(void){
	playbackData.playing = false;
	Fat_FileClose(playbackData.hFile);
	deallocate_frame_buffer(&playbackData.mpegFrameBuffer);
}


