/*
 * utils.h
 *
 *  Created on: Jan 17, 2017
 *      Author: ggowripa
 */

#ifndef UTILS_H_
#define UTILS_H_

//
// DBG_PRINT used to print debug statements
// Use this so we can easily remove debug statements
// with minimal code changes
//
#define DBG_PRINT(str, ...)	printf("[%s:%d] " str, 		\
				__FUNCTION__, __LINE__, ##__VA_ARGS__);	\

//
// Asserts on !x and prints errorstr
//
#define assert(x, errorStr, ...)						\
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
#define false (0)
#define true  (1)

#endif /* UTILS_H_ */
