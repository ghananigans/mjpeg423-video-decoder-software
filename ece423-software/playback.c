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

	load_mpeg_header(playbackData.hFile, &playbackData.mpegHeader);
	assert(playbackData.mpegHeader.w_size == DISPLAY_WIDTH, "Video file width unrecognized\n");
	assert(playbackData.mpegHeader.h_size == DISPLAY_HEIGHT, "Video file height unrecognized\n");

	load_mpeg_trailer(playbackData.hFile, &playbackData.mpegHeader, &playbackData.mpegTrailer);

	retVal = allocate_frame_buffer(&playbackData.mpegHeader, &playbackData.mpegFrameBuffer);
	assert(retVal, "Failed to allocate frame buffer");
}

void playFrame(ece423_video_display* display) {
	int retVal;
	uint32_t* currentOutputBuffer;

	// Wait until a buffer is available again
	while (ece423_video_display_buffer_is_available(display) != 0){}

	// Get a pointer to the available buffer
	currentOutputBuffer = ece423_video_display_get_buffer(display);

	// Read and decode frame and write it to the buffer
	DBG_PRINT("Frame #%d:\n", playbackData.currentFrame);
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

void previewVideo(ece423_video_display* display) {
	startPlaybackFrameTimer();

	if (playbackData.currentFrame < playbackData.mpegHeader.num_frames) {
		playFrame(display);
	}

	stopPlaybackFrameTimer();
}

// TODO: we need to make this timer based
void playVideo(ece423_video_display* display) {
	playbackData.playing = true;
	startPlaybackFrameTimer();

	while (playbackData.currentFrame < playbackData.mpegHeader.num_frames && !buttonHasBeenPressed()) {
		playFrame(display);
	}

	stopPlaybackFrameTimer();
}

bool isVideoPlaying(void) {
	return playbackData.playing;
}

int fastforwardVideo() {
	int tempIndex;
	int error;

	if (playbackData.mpegHeader.num_frames - playbackData.currentFrame < 120)
	{
		DBG_PRINT("Fast forwarded to end of video\n");
		return 0;
	}

	tempIndex = playbackData.currentFrame / MAX_IFRAME_OFFSET;

	while (1) {
		++tempIndex;

		if (playbackData.mpegTrailer.iframe_info[tempIndex].frame_index - playbackData.currentFrame > 110)
		{
			DBG_PRINT("Fast forwarded to frame #%d from current frame #%d\n",
					playbackData.mpegTrailer.iframe_info[tempIndex].frame_index,
					playbackData.currentFrame);

			playbackData.currentFrame = playbackData.mpegTrailer.iframe_info[tempIndex].frame_index;

			// move to fast forward-ed frame
			error = Fat_FileSeek(playbackData.hFile, FILE_SEEK_BEGIN,
					playbackData.mpegTrailer.iframe_info[tempIndex].frame_position);
			assert(error, "Failed to seek file\n");

			break;
		}
	}

	return 1;
}

void pauseVideo(void) {
	playbackData.playing = false;
}

void closeVideo(void){
	playbackData.playing = false;
	Fat_FileClose(playbackData.hFile);
	deallocate_frame_buffer(&playbackData.mpegFrameBuffer);
	unload_mpeg_trailer(&playbackData.mpegTrailer);
}


