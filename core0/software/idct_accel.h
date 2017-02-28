/*
 * idct_accel.h
 *
 *  Created on: Feb 7, 2017
 *      Author: ggowripa
 */

#ifndef IDCT_ACCEL_H_
#define IDCT_ACCEL_H_

#include <stdint.h>

int init_idct_accel (void);
void idct_accel_calculate_buffer (uint32_t* inputBuffer, uint32_t* outputBuffer, uint32_t sizeOfInputBuffers, uint32_t sizeOfOutputBuffers);
void wait_for_idct_finsh (void);
void test_idct_accel (void);

#endif /* IDCT_ACCEL_H_ */