/*
 * profile.h
 *
 *  Created on: Jan 25, 2017
 *      Author: ggowripa
 */

#ifndef PROFILE_H_
#define PROFILE_H_

#include "../../common/config.h"
#include <sys/alt_timestamp.h>
#include <stdint.h>
#include "../../common/utils.h"

#define NUM_TIMING_TESTS							(10)
#define NUM_SIZE_TESTS								(4)

extern uint16_t profile_time_flag;
extern uint64_t profile_time_ticks[NUM_TIMING_TESTS][2];
extern uint64_t profile_time_count[NUM_TIMING_TESTS][2];
extern uint64_t profile_time_max[NUM_TIMING_TESTS][2];
extern uint64_t profile_time_min[NUM_TIMING_TESTS][2];
extern uint64_t profile_time_temp;
extern int profile_time_retVal;

extern uint64_t profile_size_total[NUM_SIZE_TESTS][2];
extern uint64_t profile_size_count[NUM_SIZE_TESTS][2];
extern uint64_t profile_size_max[NUM_SIZE_TESTS][2];
extern uint64_t profile_size_min[NUM_SIZE_TESTS][2];

#ifdef TIMING_TESTS
#define TIMING_TEST_EMPTY							(0)
//#define TIMING_TEST_SD_READ							(1)
//#define TIMING_TEST_LOSSLESS_Y						(2)
//#define TIMING_TEST_LOSSLESS_CB						(3)
//#define TIMING_TEST_LOSSLESS_CR						(4)
//#define TIMING_TEST_IDCT_ONE_FRAME					(5)
//#define TIMING_TEST_IDCT_ONE_COLOUR_COMPONENT		(6)
#define TIMING_TEST_IDCT_ONE_8_X_8_BLOCK			(7)
//#define TIMING_TEST_YCBCR_TO_RGB_ONE_FRAME			(8)
#define TIMING_TEST_YCBCR_TO_RGB_8_X_8_BLOCK		(9)
#endif // #ifdef TIMING_TESTS

#ifdef SIZE_TESTS
#define SIZE_TEST_SD_READ_OUTPUT					(0)
#define SIZE_TEST_LOSSLESS_Y_INPUT					(1)
#define SIZE_TEST_LOSSLESS_CB_INPUT					(2)
#define SIZE_TEST_LOSSLESS_CR_INPUT					(3)
#endif

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
		TIMING_PRINT("Timing Test %d with Flag %d\n", (test), (flag));								\
		TIMING_PRINT("    Ticks: %llu\n", profile_time_ticks[(test)][(flag)]); 	\
		TIMING_PRINT("    Count: %llu\n", profile_time_count[(test)][(flag)]);	\
		TIMING_PRINT("    Max : %llu\n", profile_time_max[(test)][(flag)]);		\
		TIMING_PRINT("    Min : %llu\n", profile_time_min[(test)][(flag)]);		\
	}

#define PROFILE_SIZE(test, flag, value)						\
	{																\
		profile_size_total[(test)][(flag)] += (value);	\
		profile_size_count[(test)][(flag)] += 1;					\
		profile_size_max[(test)][(flag)] = MAX(profile_size_max[(test)][(flag)],	\
		(value));											\
		profile_size_min[(test)][(flag)] = MIN(profile_size_min[(test)][(flag)],	\
		(value));																\
	}

#define PROFILE_SIZE_PRINT(test, flag)									\
	{																	\
		TIMING_PRINT("Size Test %d with Flag %d\n", (test), (flag));								\
		TIMING_PRINT("    Total: %llu\n", profile_size_total[(test)][(flag)]); 	\
		TIMING_PRINT("    Count: %llu\n", profile_size_count[(test)][(flag)]);	\
		TIMING_PRINT("    Max : %llu\n", profile_size_max[(test)][(flag)]);		\
		TIMING_PRINT("    Min : %llu\n", profile_size_min[(test)][(flag)]);		\
	}

int initProfile (void);

#endif /* PROFILE_H_ */
