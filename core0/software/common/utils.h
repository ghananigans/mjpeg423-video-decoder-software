/*
 * utils.h
 *
 *  Created on: Jan 17, 2017
 *      Author: ggowripa
 */

#ifndef UTILS_H_
#define UTILS_H_

#include "config.h"

#include <stdio.h>

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#ifdef TIMING_TESTS
#define TIMING_PRINT(str, ...) printf("[%s:%d] " str,	\
				__FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define TIMING_PRINT(str, ...) ((void) 0)
#endif // #ifdef TIMING_TESTS

//
// DBG_PRINT used to print debug statements
// Use this so we can easily remove debug statements
// with minimal code changes
//
#ifdef DEBUG_PRINT_ENABLED
#define DBG_PRINT(str, ...)	printf("[%s:%d] " str, 		\
				__FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define DBG_PRINT(str, ...) ((void) 0)
#endif // #ifdef DEBUG_PRINT_ENABLED

//
// Asserts on !x and prints errorstr
//
#define assert(x, errorStr, ...)

//
// Asserts on !x and prints errorstr
//
#define assert_persistent(x, errorStr, ...)				\
	{													\
		if (!(x)) {										\
			printf("[%s:%d]** Asserting with error: "	\
					errorStr, __FUNCTION__, __LINE__, 	\
					##__VA_ARGS__);						\
			while (1) {}								\
		}												\
	}

//
// Basic true false definitions, be careful of checking for something == true,
// value of something may be not 0 but also not one
//

#ifndef false
#define false (0)
#endif // #ifndef false

#ifndef true
#define true  (1)
#endif // #ifndef true

#endif /* UTILS_H_ */
