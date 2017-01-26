/*
 * profile.h
 *
 *  Created on: Jan 25, 2017
 *      Author: ggowripa
 */

#ifndef PROFILE_H_
#define PROFILE_H_

#include "config.h"
#include <sys/alt_timestamp.h>
#include <stdint.h>
#include "utils.h"

extern uint16_t profile_time_flag;
extern uint64_t profile_time_ticks[10];
extern uint64_t profile_time_count[10];
extern uint64_t profile_time_max[10];
extern uint64_t profile_time_min[10];
extern uint64_t profile_time_temp;

extern int profile_time_retVal;

#define NUM_TIMING_TESTS							(10)

#ifdef TIMING_TESTS
#define TIMING_TEST_EMPTY							(0)
#define TIMING_TEST_SD_READ							(1)
#define TIMING_TEST_LOSSLESS_Y						(2)
#define TIMING_TEST_LOSSLESS_CB						(3)
#define TIMING_TEST_LOSSLESS_CR						(4)
#define TIMING_TEST_IDCT_ONE_FRAME					(5)
//#define TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT		(6)
//#define TIMING_TEST_IDCT_ONE_8_X_8_BLOCK			(7)
#define TIMING_TEST_YCBCR_TO_RGB_ONE_FRAME			(8)
//#define TIMING_TEST_YCBCR_TO_RGB_8_X_8_BLOCK		(9)
#endif // #ifdef TIMING_TESTS

#define PROFILE_TIME_START(test)									\
	{																\
		assert(profile_time_flag == 0, 								\
				"Something has already started to be profiled: %d",	\
				profile_time_flag);									\
		profile_time_flag = (test);									\
		profile_time_retVal = alt_timestamp_start();				\
	}

#define PROFILE_TIME_END(test)										\
	{																\
		profile_time_temp = alt_timestamp();						\
		assert(profile_time_flag == (test), 						\
				"Different profile is started: %d",					\
				profile_time_flag);									\
		assert(profile_time_retVal == 0,							\
				"alt_timestamp_start failed! %d %d\n",				\
				profile_time_flag, profile_time_retVal);			\
		profile_time_flag = 0;										\
		profile_time_ticks[(test)] += profile_time_temp;			\
		profile_time_count[(test)] += 1;							\
		profile_time_max[(test)] = MAX(profile_time_max[(test)],	\
		profile_time_temp);											\
		profile_time_min[(test)] = MIN(profile_time_min[(test)],	\
		profile_time_temp);											\
		TIMING_PRINT("TICKS: %d\n", profile_time_temp);				\
	}

#endif /* PROFILE_H_ */

#define PROFILE_TIME_PRINT(test)										\
	{																	\
		TIMING_PRINT("Test %d:\n", (test));								\
		TIMING_PRINT("    Ticks: %d\n", profile_time_ticks[(test)]); 	\
		TIMING_PRINT("    Count: %d\n", profile_time_count[(test)]);	\
		TIMING_PRINT("    Max : %d\n", profile_time_max[(test)]);		\
		TIMING_PRINT("    Min : %d\n", profile_time_min[(test)]);		\
	}


int initProfileTime (void);
