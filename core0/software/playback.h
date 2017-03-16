/*
 * playback.h
 *
 *  Created on: Jan 17, 2017
 *      Author: ggowripa
 */

#ifndef PLAYBACK_H_
#define PLAYBACK_H_

#include "libs/ece423_vid_ctl/ece423_vid_ctl.h"
#include <stdbool.h>

int fastForwardVideo (void);

void rewindVideo (void);

void loadVideo (void);

void playVideo (int (*functionToStopPlayingFrames)(void));

bool isVideoPlaying (void);

void pauseVideo (void);

void closeVideo (void);

int initPlayback (ece423_video_display* display);

#endif /* PLAYBACK_H_ */
