/*
 * playback.c
 *
 *  Created on: Jan 17, 2017
 *      Author: ggowripa
 */

#include "playback.h"
#include "common/config.h"

#include "common/utils.h"
#include "common/lib_exts/mpeg423/mpeg423_decoder_ext.h"
#include "libs/ece423_vid_ctl/ece423_vid_ctl.h"
#include "system.h"
#include "sys/alt_alarm.h"
#include "timer.h"
#include "common/mailbox/mailbox.h"
#include "idct_ycbcr_to_rgb_accel.h"

#define MAX_IFRAME_OFFSET (24)
#define NUM_H_BLOCKS (DISPLAY_HEIGHT / 8)
#define NUM_W_BLOCKS (DISPLAY_WIDTH / 8)
#define YBISTREAM_BYTES (NUM_H_BLOCKS * NUM_W_BLOCKS * 64 * sizeof(DCTELEM) + \
    		2 * NUM_H_BLOCKS * NUM_W_BLOCKS * 64 * sizeof(DCTELEM))
#define NUM_Y_DCT_BLOCKS (NUM_H_BLOCKS * NUM_W_BLOCKS)
#define NUM_CB_DCT_BLOCKS (NUM_H_BLOCKS * NUM_W_BLOCKS)
#define NUM_CR_DCT_BLOCKS (NUM_H_BLOCKS * NUM_W_BLOCKS)
#define NUM_Y_COLOR_BLOCKS (NUM_H_BLOCKS * NUM_W_BLOCKS)
#define NUM_CB_COLOR_BLOCKS (NUM_H_BLOCKS * NUM_W_BLOCKS)
#define NUM_CR_COLOR_BLOCKS (NUM_H_BLOCKS * NUM_W_BLOCKS)

static uint8_t YBitstream[YBISTREAM_BYTES];
static color_block_t Yblock[NUM_Y_COLOR_BLOCKS];
static color_block_t Cbblock[NUM_CB_COLOR_BLOCKS];
static color_block_t Crblock[NUM_CR_COLOR_BLOCKS];
static dct_block_t YDCAC[NUM_Y_DCT_BLOCKS];
static dct_block_t CbDCAC[NUM_CB_DCT_BLOCKS];
static dct_block_t CrDCAC[NUM_CR_DCT_BLOCKS];
static MPEG_FILE_HEADER mpegHeader;
static MPEG_FILE_TRAILER mpegTrailer;

typedef struct PLAYBACK_DATA_STRUCT{
	bool playing;
	uint32_t volatile currentFrame; // 0 index frames
	uint32_t processedFrame;
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
	// Send request to slave to read SD
	// Wait for response saying done reading SD
	// Send request to Slave to do LD Y
	// Do LD CB and Cr
	// Tell slave done LD cb and Cr (read SD)
	// DO IDCT for all 3
	// let slave know that idct y is done

	// Flag the buffer as written
	ece423_video_display_register_written_buffer(playbackData.display);

	playbackData.processedFrame++;

	if (!forcePeriodic) {
		ece423_video_display_switch_frames(playbackData.display);
		playbackData.currentFrame++;
	}
}

/*
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
*/

/*
 * 	 Public playback api
 */

void loadVideo (void) {
	mailbox_msg_t * msg;
	uint32_t* currentOutputBuffer;

	playbackData.playing = false;
	playbackData.currentFrame = 0;
	playbackData.processedFrame = 0;
	playbackData.mpegFrameBuffer = (MPEG_WORKING_BUFFER) {
		&YBitstream, // Ybitstream
		&YBitstream, // Cbbitstream
		&YBitstream, // Crbitstream
		&Yblock, // Yblock
		&Cbblock, // Cbblock
		&Crblock, // Crblock
		&YDCAC, // YDCAC
		&CbDCAC, // CbDCAC
		&CrDCAC // CrDCAC
	};

	while (ece423_video_display_buffer_is_available(playbackData.display) != 0){}
	currentOutputBuffer = ece423_video_display_get_buffer(playbackData.display);

	DBG_PRINT("Send msg to slave to read next file (and do LD Y)\n");
	send_read_next_file (&mpegHeader, &mpegTrailer, &YBitstream, &YDCAC);

	DBG_PRINT("Wait for response from slave so we can start LD CB and CR\n");
	msg = recv_msg();
	assert(msg->header.type == DONE_READ_NEXT_FRAME,
			"Received msg is not DONE_READ_NEXT_FRAME; %d\n", msg->header.type);
	DBG_PRINT("Got response from slave so we can start LD CB and CR\n");

	uint8_t * ptr = &YBitstream + msg->type_data.done_read_next_frame.cbOffset;
	lossless_decode(NUM_CB_DCT_BLOCKS, ptr,
			&CbDCAC, Cquant, msg->type_data.done_read_next_frame.frameType);

	ptr += msg->type_data.done_read_next_frame.crOffset;
	lossless_decode(NUM_CR_DCT_BLOCKS, ptr,
			&CrDCAC, Cquant, msg->type_data.done_read_next_frame.frameType);

	alt_dcache_flush_all();
	idct_accel_calculate_buffer_cb(&CbDCAC, sizeof(CbDCAC));
	idct_accel_calculate_buffer_cr(&CrDCAC, sizeof(CrDCAC));

	DBG_PRINT("Wait for response from slave so we can start IDCT Y\n");
	msg = recv_msg();
	assert(msg->header.type == DONE_LD_Y,
			"Received msg is not DONE_LD_Y; %d\n", msg->header.type);
	DBG_PRINT("Got response from slave so we can start IDCT Y\n");
	idct_accel_calculate_buffer_y(&YDCAC, sizeof(YDCAC));

	ycbcr_to_rgb_accel_get_results(currentOutputBuffer, DISPLAY_HEIGHT * DISPLAY_WIDTH * sizeof(rgb_pixel_t));
	DBG_PRINT("Wait for ycbcr_to_rgb to finish\n");
	wait_for_ycbcr_to_rgb_finsh();
	DBG_PRINT("Done ycbcr_to_rgb\n");

	ece423_video_display_register_written_buffer(playbackData.display);
	playbackData.processedFrame++;

	ece423_video_display_switch_frames(playbackData.display);
	playbackData.currentFrame++;

	DBG_PRINT("Done readfile\n");
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

void pauseVideo (void) {
	DBG_PRINT("Setting playing status to false\n");

	playbackData.playing = false;
}

void closeVideo (void) {
	DBG_PRINT("Closing video\n");

	playbackData.playing = false;
}

int initPlayback (ece423_video_display* display) {
	playbackData.display = display;
	return initTimer(FRAME_RATE_MS, &timerFunction);
}

