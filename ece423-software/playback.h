/*
 * playback.h
 *
 *  Created on: Jan 17, 2017
 *      Author: ggowripa
 */

#ifndef PLAYBACK_H_
#define PLAYBACK_H_

#include "libs/ece423_sd/ece423_sd.h"
#include "libs/ece423_vid_ctl/ece423_vid_ctl.h"

void loadVideo(FAT_HANDLE hFat, char* filename);
void playVideo(ece423_video_display* display, bool *functionToStopPlayingFrames(void));
int fastforwardVideo(void);
void rewindVideo(void);
void previewVideo(ece423_video_display* display);
bool isVideoPlaying(void);
void pauseVideo(void);
void closeVideo(void);



#endif /* PLAYBACK_H_ */
