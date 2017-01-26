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

#define NUM_TIMING_TESTS							(10)

extern uint16_t profile_time_flag;
extern uint64_t profile_time_ticks[NUM_TIMING_TESTS][2];
extern uint64_t profile_time_count[NUM_TIMING_TESTS][2];
extern uint64_t profile_time_max[NUM_TIMING_TESTS][2];
extern uint64_t profile_time_min[NUM_TIMING_TESTS][2];
extern uint64_t profile_time_temp;

extern int profile_time_retVal;

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

#define PROFILE_TIME_START(test, flag)								\
	{																\
		profile_time_flag = (test);									\
		profile_time_retVal = alt_timestamp_start();				\
	}

#define PROFILE_TIME_END(test, flag)								\
	{																\
		profile_time_temp = alt_timestamp();						\
		profile_time_flag = 0;										\
		profile_time_ticks[(test)][(flag)] += profile_time_temp;	\
		profile_time_count[(test)][(flag)] += 1;					\
		profile_time_max[(test)][(flag)] = MAX(profile_time_max[(test)][(flag)],	\
		profile_time_temp);											\
		profile_time_min[(test)][(flag)] = MIN(profile_time_min[(test)][(flag)],	\
		profile_time_temp);											\
	}

#define PROFILE_TIME_PRINT(test, flag)										\
	{																	\
		TIMING_PRINT("Test %d with Flag %d\n", (test), (flag));								\
		TIMING_PRINT("    Ticks: %lu\n", profile_time_ticks[(test)][(flag)]); 	\
		TIMING_PRINT("    Count: %lu\n", profile_time_count[(test)][(flag)]);	\
		TIMING_PRINT("    Max : %lu\n", profile_time_max[(test)][(flag)]);		\
		TIMING_PRINT("    Min : %lu\n", profile_time_min[(test)][(flag)]);		\
	}

#endif /* PROFILE_H_ */


int initProfileTime (void);
