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
#include <sys/alt_cache.h>

typedef struct PLAYBACK_DATA_STRUCT{
	bool playing;
	uint32_t volatile currentFrame; // 0 index frames
	uint32_t processedFrame;
	MPEG_FILE_HEADER volatile mpegHeader;
	MPEG_FILE_TRAILER volatile mpegTrailer;
	MPEG_WORKING_BUFFER mpegFrameBuffer;
	ece423_video_display* display;
}PLAYBACK_DATA;

static PLAYBACK_DATA playbackData;


/*
 *   Timer control for controlling the frame rate
 */
static void timerFunction (void) {
	int retVal;
	retVal = ece423_video_display_switch_frames(playbackData.display);
	if (retVal != -1) {
		playbackData.currentFrame++;
	}
}

/*
 * 	 Public playback api
 */
static inline void ld_idct_cr_cb (mailbox_msg_t volatile * msg) {
	uint8_t volatile * ptr;

	ptr = playbackData.mpegFrameBuffer.Ybitstream + msg->type_data.done_read_next_frame.cbOffset;
	alt_dcache_flush_no_writeback(ptr, YBISTREAM_BYTES - msg->type_data.done_read_next_frame.cbOffset);

	lossless_decode(NUM_CB_DCT_BLOCKS, ptr, playbackData.mpegFrameBuffer.CbDCAC,
			Cquant, msg->type_data.done_read_next_frame.frameType);

	ptr = playbackData.mpegFrameBuffer.Ybitstream + msg->type_data.done_read_next_frame.crOffset;
	lossless_decode(NUM_CR_DCT_BLOCKS, ptr, playbackData.mpegFrameBuffer.CrDCAC,
			Cquant, msg->type_data.done_read_next_frame.frameType);

	DBG_PRINT("LD for CB and CR done\n");

	alt_dcache_flush(playbackData.mpegFrameBuffer.CbDCAC, NUM_CB_DCT_BLOCKS * sizeof(dct_block_t));
	alt_dcache_flush(playbackData.mpegFrameBuffer.CrDCAC, NUM_CR_DCT_BLOCKS * sizeof(dct_block_t));

	idct_accel_calculate_buffer_cb(playbackData.mpegFrameBuffer.CbDCAC,
			NUM_CB_DCT_BLOCKS * sizeof(dct_block_t));

	idct_accel_calculate_buffer_cr(playbackData.mpegFrameBuffer.CrDCAC,
			NUM_CR_DCT_BLOCKS * sizeof(dct_block_t));

	DBG_PRINT("IDCT for CB and CR started\n");
}

static inline void process (void * const currentOutputBuffer, bool const sendRequest, bool const noTimer) {
	mailbox_msg_t volatile * msg;

	DBG_PRINT("Wait for response from slave so we can start LD CB and CR\n");
	msg = recv_msg();
	assert(msg->header.type == DONE_READ_NEXT_FRAME,
			"Received msg is not DONE_READ_NEXT_FRAME; %d\n", msg->header.type);
	DBG_PRINT("Got response from slave so we can start LD CB and CR\n");

	ld_idct_cr_cb(msg);

	if (sendRequest) {
		send_ok_to_read_next_frame(playbackData.mpegFrameBuffer.Ybitstream);
		DBG_PRINT("OK_TO_READ_NEXT_FRAME msg sent\n");
	}

	DBG_PRINT("Wait for response from slave so we can start IDCT Y\n");
	msg = recv_msg();
	assert(msg->header.type == DONE_LD_Y,
			"Received msg is not DONE_LD_Y; %d\n", msg->header.type);
	DBG_PRINT("Got response from slave so we can start IDCT Y\n");

	idct_accel_calculate_buffer_y(playbackData.mpegFrameBuffer.YDCAC,
			NUM_Y_DCT_BLOCKS * sizeof(dct_block_t));

	DBG_PRINT("IDCT for Y started\n");


	ycbcr_to_rgb_accel_get_results(currentOutputBuffer,
			DISPLAY_HEIGHT * DISPLAY_WIDTH * sizeof(rgb_pixel_t));

	DBG_PRINT("Wait for ycbcr_to_rgb to finish\n");

	if (sendRequest) {
		wait_for_idct_y_finsh();

		send_ok_to_ld_y(&playbackData.mpegHeader,
				playbackData.mpegFrameBuffer.Ybitstream, playbackData.mpegFrameBuffer.YDCAC);
		DBG_PRINT("OK_TO_LD_Y msg sent\n");
	}

	wait_for_ycbcr_to_rgb_finsh();

	DBG_PRINT("Done ycbcr_to_rgb\n");

	ece423_video_display_register_written_buffer(playbackData.display);
	playbackData.processedFrame++;

	DBG_PRINT("Registered output frame buffer\n");

	if (noTimer) {
		ece423_video_display_switch_frames(playbackData.display);
		playbackData.currentFrame++;
	}
}

static void seekFrame (uint32_t frameIndex, uint32_t framePosition) {
	void * currentOutputBuffer;

	DBG_PRINT("Move to frame #%u from current frame #%u at frame position of %d\n",
			frameIndex, playbackData.currentFrame, framePosition);

	playbackData.currentFrame = frameIndex;
	playbackData.processedFrame = frameIndex;

	while (ece423_video_display_buffer_is_available(playbackData.display) != 0){}
	currentOutputBuffer = ece423_video_display_get_buffer(playbackData.display);

	DBG_PRINT("Send msg to slave to read next file (and do LD Y)\n");
	send_seek_video (&playbackData.mpegHeader, playbackData.mpegFrameBuffer.Ybitstream,
			playbackData.mpegFrameBuffer.YDCAC, framePosition);

	process(currentOutputBuffer, 0, 1);

	DBG_PRINT("Done load video\n");
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

void loadVideo (void) {
	uint32_t* currentOutputBuffer;

	alt_dcache_flush_all();

	playbackData.playing = false;
	playbackData.currentFrame = 0;
	playbackData.processedFrame = 0;

	while (ece423_video_display_buffer_is_available(playbackData.display) != 0){}
	currentOutputBuffer = ece423_video_display_get_buffer(playbackData.display);

	DBG_PRINT("Send msg to slave to read next file (and do LD Y)\n");
	send_read_next_file (&playbackData.mpegHeader, &playbackData.mpegTrailer,
			playbackData.mpegFrameBuffer.Ybitstream, playbackData.mpegFrameBuffer.YDCAC);

	process(currentOutputBuffer, 0, 1);

	DBG_PRINT("Done load video\n");
}

void playVideo (int (*functionToStopPlayingFrames)(void)) {
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
	send_ok_to_ld_y(&playbackData.mpegHeader,
			playbackData.mpegFrameBuffer.Ybitstream, playbackData.mpegFrameBuffer.YDCAC);

	while (flag) {
		flag = ((playbackData.processedFrame < (playbackData.mpegHeader.num_frames - 1))
				&& (functionToStopPlayingFrames() == 0));

		process(currentOutputBuffer, flag, !FORCE_PERIODIC);

		if (flag) {
			while (ece423_video_display_buffer_is_available(playbackData.display) != 0){}
			currentOutputBuffer = ece423_video_display_get_buffer(playbackData.display);
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

	playbackData.mpegFrameBuffer.Ybitstream = (uint8_t volatile *) malloc(
			YBISTREAM_BYTES);
	if (!playbackData.mpegFrameBuffer.Ybitstream) {
		DBG_PRINT("Ybistream malloc failed!\n");
		return 0;
	}

	playbackData.mpegFrameBuffer.YDCAC = (dct_block_t volatile *) malloc(
			NUM_Y_DCT_BLOCKS * sizeof(dct_block_t));
	if (!playbackData.mpegFrameBuffer.YDCAC) {
		DBG_PRINT("YDCAC malloc failed!\n");
		return 0;
	}

	playbackData.mpegFrameBuffer.CbDCAC = (dct_block_t volatile *) malloc(
			NUM_CB_DCT_BLOCKS * sizeof(dct_block_t));
	if (!playbackData.mpegFrameBuffer.CbDCAC) {
		DBG_PRINT("CbDCAC malloc failed!\n");
		return 0;
	}

	playbackData.mpegFrameBuffer.CrDCAC = (dct_block_t volatile *) malloc(
			NUM_CR_DCT_BLOCKS * sizeof(dct_block_t));
	if (!playbackData.mpegFrameBuffer.CrDCAC) {
		DBG_PRINT("CrDCAC malloc failed!\n");
		return 0;
	}

	return initTimer(FRAME_RATE_US, &timerFunction);
}
