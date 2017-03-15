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
#include <stdbool.h>
#include <io.h>
#include <string.h>

#define MAX_IFRAME_OFFSET (24)
#define NUM_H_BLOCKS (DISPLAY_HEIGHT / 8)
#define NUM_W_BLOCKS (DISPLAY_WIDTH / 8)
#define YBISTREAM_BYTES (NUM_H_BLOCKS * NUM_W_BLOCKS * 64 * sizeof(DCTELEM) + \
    		2 * NUM_H_BLOCKS * NUM_W_BLOCKS * 64 * sizeof(DCTELEM))
#define NUM_Y_DCT_BLOCKS (NUM_H_BLOCKS * NUM_W_BLOCKS)
#define NUM_CB_DCT_BLOCKS (NUM_H_BLOCKS * NUM_W_BLOCKS)
#define NUM_CR_DCT_BLOCKS (NUM_H_BLOCKS * NUM_W_BLOCKS)

typedef struct PLAYBACK_DATA_STRUCT{
	bool playing;
	uint32_t volatile currentFrame; // 0 index frames
	uint32_t processedFrame;
	MPEG_FILE_HEADER volatile mpegHeader;
	MPEG_FILE_TRAILER volatile mpegTrailer;
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
static inline void ld_idct_cr_cb (mailbox_msg_t volatile * msg) {
	uint8_t volatile * ptr;

	ptr = playbackData.mpegFrameBuffer.Ybitstream + msg->type_data.done_read_next_frame.cbOffset;
	lossless_decode(NUM_CB_DCT_BLOCKS, ptr, playbackData.mpegFrameBuffer.CbDCAC,
			Cquant, msg->type_data.done_read_next_frame.frameType);

	ptr = playbackData.mpegFrameBuffer.Ybitstream + msg->type_data.done_read_next_frame.crOffset;
	lossless_decode(NUM_CR_DCT_BLOCKS, ptr, playbackData.mpegFrameBuffer.CrDCAC,
			Cquant, msg->type_data.done_read_next_frame.frameType);

	DBG_PRINT("LD for CB and CR done\n");
}

static inline void idct_y_ycbcr (mailbox_msg_t volatile * msg, void * outputBuffer) {
	idct_accel_calculate_buffer_cb(playbackData.mpegFrameBuffer.CbDCAC,
			NUM_CB_DCT_BLOCKS * sizeof(dct_block_t));

	idct_accel_calculate_buffer_cr(playbackData.mpegFrameBuffer.CrDCAC,
			NUM_CR_DCT_BLOCKS * sizeof(dct_block_t));

	idct_accel_calculate_buffer_y(playbackData.mpegFrameBuffer.YDCAC,
			NUM_Y_DCT_BLOCKS * sizeof(dct_block_t));

	DBG_PRINT("IDCT for Y, CB, CR started\n");

	ycbcr_to_rgb_accel_get_results(outputBuffer,
			DISPLAY_HEIGHT * DISPLAY_WIDTH * sizeof(rgb_pixel_t));

	DBG_PRINT("Wait for ycbcr_to_rgb to finish\n");

	wait_for_ycbcr_to_rgb_finsh();

	DBG_PRINT("Done ycbcr_to_rgb\n");

	ece423_video_display_register_written_buffer(playbackData.display);
	playbackData.processedFrame++;

	DBG_PRINT("Registered output frame buffer\n");
}

void loadVideo (void) {
	mailbox_msg_t volatile * msg;
	uint32_t* currentOutputBuffer;
	mailbox_msg_t temp;

	playbackData.playing = false;
	playbackData.currentFrame = 0;
	playbackData.processedFrame = 0;

	while (ece423_video_display_buffer_is_available(playbackData.display) != 0){}
	currentOutputBuffer = ece423_video_display_get_buffer(playbackData.display);

	DBG_PRINT("Send msg to slave to read next file (and do LD Y)\n");
	send_read_next_file (&playbackData.mpegHeader, &playbackData.mpegTrailer,
			playbackData.mpegFrameBuffer.Ybitstream, playbackData.mpegFrameBuffer.YDCAC);

	DBG_PRINT("Wait for response from slave so we can start LD CB and CR\n");
	msg = recv_msg();
	assert(msg->header.type == DONE_READ_NEXT_FRAME,
			"Received msg is not DONE_READ_NEXT_FRAME; %d\n", msg->header.type);
	DBG_PRINT("Got response from slave so we can start LD CB and CR\n");

	//
	// Flush cache so that our mpeg header and trailer stuff is updated
	//
	alt_dcache_flush_all();

	memcpy(&temp, msg, sizeof(temp));

	DBG_PRINT("Wait for response from slave so we can start IDCT Y\n");
	msg = recv_msg();
	assert(msg->header.type == DONE_LD_Y,
			"Received msg is not DONE_LD_Y; %d\n", msg->header.type);
	DBG_PRINT("Got response from slave so we can start IDCT Y\n");

	ld_idct_cr_cb(&temp);
	idct_y_ycbcr(msg, currentOutputBuffer);

	ece423_video_display_switch_frames(playbackData.display);
	playbackData.currentFrame++;

	DBG_PRINT("Done load video\n");
}

void previewVideo (void) {
	//startPlaybackFrameTimer();

	if (playbackData.currentFrame < playbackData.mpegHeader.num_frames) {
		playFrame(0);
	}

	//stopPlaybackFrameTimer();
}

void playVideo (int (*functionToStopPlayingFrames)(void)) {
	mailbox_msg_t volatile * msg;
	mailbox_msg_t temp;
	uint32_t* currentOutputBuffer;
	int flag = 1;

	DBG_PRINT("Playing the video\n");

	playbackData.playing = playbackData.currentFrame < playbackData.mpegHeader.num_frames;
	if (!playbackData.playing) {
		DBG_PRINT("At the end of the video, can't play\n");
		return;
	}

	if (FORCE_PERIODIC) {
		startTimer();
	}

	while (ece423_video_display_buffer_is_available(playbackData.display) != 0){}
	currentOutputBuffer = ece423_video_display_get_buffer(playbackData.display);

	DBG_PRINT("Send msg to slave to read next frame (and do LD Y)\n");
	send_ok_to_read_next_frame(playbackData.mpegFrameBuffer.Ybitstream);
	send_ok_to_ld_y(playbackData.mpegFrameBuffer.YDCAC);

	while (flag) {
		//playFrame(FORCE_PERIODIC);
		DBG_PRINT("Waiting for DONE_READ_NEXT_FRAME msg\n");
		msg = recv_msg();
		DBG_PRINT("Got msg type: %d\n", msg->header.type);
		assert(msg->header.type == DONE_READ_NEXT_FRAME,
				"Msg request is not DONE_READ_NEXT_FRAME: %d", msg->header.type);

		DBG_PRINT("DONE_READ_NEXT_FRAME msg received\n");

		memcpy(&temp, msg, sizeof(temp));

		DBG_PRINT("Waiting for DONE_LD_Y msg\n");
		msg = recv_msg();
		DBG_PRINT("Got msg type: %d\n", msg->header.type);
		assert(msg->header.type == DONE_LD_Y,
				"Msg request is not DONE_LD_Y: %d", msg->header.type);

		DBG_PRINT("DONE_LD_Y msg received\n");

		ld_idct_cr_cb(&temp);
		idct_y_ycbcr(msg, currentOutputBuffer);

		if (!FORCE_PERIODIC) {
			ece423_video_display_switch_frames(playbackData.display);
			playbackData.currentFrame++;
		}

		if ((playbackData.processedFrame < playbackData.mpegHeader.num_frames)
				&& (functionToStopPlayingFrames() == 0)) {
			send_ok_to_read_next_frame(playbackData.mpegFrameBuffer.Ybitstream);
			DBG_PRINT("OK_TO_READ_NEXT_FRAME msg sent\n");

			send_ok_to_ld_y(playbackData.mpegFrameBuffer.YDCAC);
			DBG_PRINT("OK_TO_LD_Y msg sent\n");

			while (ece423_video_display_buffer_is_available(playbackData.display) != 0){}
			currentOutputBuffer = ece423_video_display_get_buffer(playbackData.display);
		} else {
			flag = 0;
		}
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

	playbackData.mpegFrameBuffer.Ybitstream = (uint8_t volatile *) alt_uncached_malloc(
			YBISTREAM_BYTES);
	if (!playbackData.mpegFrameBuffer.Ybitstream) {
		DBG_PRINT("Ybistream malloc failed!\n");
		return 0;
	}

	playbackData.mpegFrameBuffer.YDCAC = (dct_block_t volatile *) alt_uncached_malloc(
			NUM_Y_DCT_BLOCKS * sizeof(dct_block_t));
	if (!playbackData.mpegFrameBuffer.YDCAC) {
		DBG_PRINT("YDCAC malloc failed!\n");
		return 0;
	}

	playbackData.mpegFrameBuffer.CbDCAC = (dct_block_t volatile *)alt_uncached_malloc(
			NUM_CB_DCT_BLOCKS * sizeof(dct_block_t));
	if (!playbackData.mpegFrameBuffer.CbDCAC) {
		DBG_PRINT("CbDCAC malloc failed!\n");
		return 0;
	}

	playbackData.mpegFrameBuffer.CrDCAC = (dct_block_t volatile *)alt_uncached_malloc(
			NUM_CR_DCT_BLOCKS * sizeof(dct_block_t));
	if (!playbackData.mpegFrameBuffer.CrDCAC) {
		DBG_PRINT("CrDCAC malloc failed!\n");
		return 0;
	}

	return initTimer(FRAME_RATE_MS, &timerFunction);
}

