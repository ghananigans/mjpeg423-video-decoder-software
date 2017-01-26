#include "profile.h"
#include <string.h>

uint16_t profile_time_flag = 0;
uint64_t profile_time_ticks[NUM_TIMING_TESTS][2];
uint64_t profile_time_count[NUM_TIMING_TESTS][2];
uint64_t profile_time_max[NUM_TIMING_TESTS][2];
uint64_t profile_time_min[NUM_TIMING_TESTS][2];
int profile_time_retVal;
uint64_t profile_time_temp;

int initProfileTime (void) {
	int retVal;

	memset(profile_time_max, 0, sizeof(profile_time_max));
	memset(profile_time_min, -1, sizeof(profile_time_min));
	memset(profile_time_count, 0, sizeof(profile_time_count));
	memset(profile_time_ticks, 0, sizeof(profile_time_ticks));
	profile_time_flag = 0;

	retVal = alt_timestamp_start();
	alt_timestamp();

	return retVal;
}
