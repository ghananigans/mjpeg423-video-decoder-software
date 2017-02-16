/*
 * idct_accel.c
 *
 *  Created on: Feb 7, 2017
 *      Author: ggowripa
 */

#include "idct_accel.h"
#include "utils.h"

#include "system.h"
#include <sys/alt_cache.h>
#include <malloc.h>
#include <priv/alt_file.h>
#include <sys/alt_irq.h>

#include "altera_msgdma.h"
#include "altera_msgdma_descriptor_regs.h"
#include "altera_msgdma_csr_regs.h"
#include <stdbool.h>

#define DESC_CONTROL_FROM_IDCT_ACCEL      (ALTERA_MSGDMA_DESCRIPTOR_CONTROL_TRANSFER_COMPLETE_IRQ_MASK | ALTERA_MSGDMA_DESCRIPTOR_CONTROL_GO_MASK)
#define DESC_CONTROL_TO_IDCT_ACCEL        (ALTERA_MSGDMA_DESCRIPTOR_CONTROL_GO_MASK)



typedef struct mdma {
	alt_msgdma_standard_descriptor desc;
	alt_msgdma_dev* dev;
} mdma_t;


static mdma_t from_accel;
static mdma_t to_accel;

static int volatile working_count = 0;

static void dmaToIrq (void* isr_context){
	IOWR_ALTERA_MSGDMA_CSR_STATUS(MDMA_TO_IDCT_ACCEL_CSR_BASE, 0x1);
}

static void dmaFromIrq (void* isr_context){
	IOWR_ALTERA_MSGDMA_CSR_STATUS(MDMA_FROM_IDCT_ACCEL_CSR_BASE, 0x1);
	--working_count;
}

void print_buffer16(int16_t volatile (* buffer)[8]) {
	for (int i = 0; i < 8; ++i) {
		for (int j = 0; j < 8; ++j) {
			printf("%d ", (int16_t)IORD_16DIRECT(&buffer[i][j], 0));
		}

		printf("\n");
	}
}

void print_buffer8(uint8_t volatile (* buffer)[8]) {
	for (int i = 0; i < 8; ++i) {
		for (int j = 0; j < 8; ++j) {
			printf("%d ", (uint8_t)IORD_8DIRECT(&buffer[i][j], 0));
		}

		printf("\n");
	}
}

int16_t volatile testBuffer1[8][8] = {
		{100, 0, 0, 0, 0, 0, 0, 0},
		{0, 100, 0, 0, 0, 0, 0, 0},
		{0, 0, 100, 0, 0, 0, 0, 0},
		{0, 0, 0, 100, 0, 0, 0, 0},
		{0, 0, 0, 0, 100, 0, 0, 0},
		{0, 0, 0, 0, 0, 100, 0, 0},
		{0, 0, 0, 0, 0, 0, 100, 0},
		{0, 0, 0, 0, 0, 0, 0, 100}
};

int16_t volatile testBuffer2[8][8] = {
		{1240,   0, -10, 0, 0, 0, 0, 0},
		{ -24, -12,   0, 0, 0, 0, 0, 0},
		{ -14, -13,   0, 0, 0, 0, 0, 0},
		{  0,    0,   0, 0, 0, 0, 0, 0},
		{  0,    0,   0, 0, 0, 0, 0, 0},
		{  0,    0,   0, 0, 0, 0, 0, 0},
		{  0,    0,   0, 0, 0, 0, 0, 0},
		{  0,    0,   0, 0, 0, 0, 0, 0}
};

uint8_t volatile outputBuffer[8][8] = {
		{1, 2, 3, 4, 5, 6, 7, 8},
		{2, 2, 3, 4, 5, 6, 7, 8},
		{3, 2, 3, 4, 5, 6, 7, 8},
		{4, 2, 3, 4, 5, 6, 7, 8},
		{5, 2, 3, 4, 5, 6, 7, 8},
		{6, 2, 3, 4, 5, 6, 7, 8},
		{7, 2, 3, 4, 5, 6, 7, 8},
		{8, 2, 3, 4, 5, 6, 7, 8}
};

int init_idct_accel (void){
	int retVal = 1;

	from_accel.dev = alt_msgdma_open(MDMA_FROM_IDCT_ACCEL_CSR_NAME);
	assert(from_accel.dev, "MDMA from failed to open\n");

	to_accel.dev   = alt_msgdma_open(MDMA_TO_IDCT_ACCEL_CSR_NAME);
	assert(to_accel.dev, "MDMA to failed to open\n");

	//
	// Interrupt Setup
	// To match our 1 = success and 0 = failure convention
	//
	//retVal = alt_ic_isr_register(MDMA_FROM_IDCT_ACCEL_CSR_IRQ_INTERRUPT_CONTROLLER_ID, MDMA_FROM_IDCT_ACCEL_CSR_IRQ, \
	//		dmaIrq, NULL, NULL) ? 0 : 1;

	alt_msgdma_register_callback(from_accel.dev, &dmaFromIrq, ALTERA_MSGDMA_CSR_GLOBAL_INTERRUPT_MASK, NULL);
	//alt_msgdma_register_callback(to_accel.dev, &dmaToIrq, ALTERA_MSGDMA_CSR_GLOBAL_INTERRUPT_MASK, NULL);

	return retVal;
}

void idct_accel_calculate_buffer (uint32_t* inputBuffer, uint32_t* outputBuffer, uint32_t sizeOfInputBuffer, uint32_t sizeOfOutputBuffer){
	int retVal;

	++working_count;

	retVal = alt_msgdma_construct_standard_mm_to_st_descriptor(to_accel.dev,
					&to_accel.desc,(alt_u32 *)inputBuffer, sizeOfInputBuffer, DESC_CONTROL_TO_IDCT_ACCEL);
	assert(retVal == 0, "ERROR: %d\n", retVal);

	retVal = alt_msgdma_construct_standard_st_to_mm_descriptor(from_accel.dev,
					&from_accel.desc,(alt_u32 *)outputBuffer, sizeOfOutputBuffer, DESC_CONTROL_FROM_IDCT_ACCEL);
	assert(retVal == 0, "ERROR: %d\n", retVal);

	//printf("Attempting DMA transfer...\n");

	retVal = alt_msgdma_standard_descriptor_async_transfer(to_accel.dev, &to_accel.desc);
	assert(retVal == 0, "ERROR: %d\n", retVal);

	retVal = alt_msgdma_standard_descriptor_async_transfer(from_accel.dev, &from_accel.desc);
	assert(retVal == 0, "ERROR: %d\n", retVal);
}

void wait_for_idct_finsh (void) {
	while (working_count != 0);
}

void test_idct (void){
	int16_t volatile (* test1P)[8] = (int16_t volatile (*)[8]) &testBuffer1;
	int16_t volatile (* test2P)[8] = (int16_t volatile (*)[8]) &testBuffer2;
	uint8_t volatile (* outP)[8] = (uint8_t volatile (*)[8]) &outputBuffer;

	printf("Testing IDCT...\n");

	printf("\nPrint first buffer\n");
	print_buffer16(test1P);

	printf("\nPrint second buffer\n");
	print_buffer16(test2P);

	printf("\nPrint out buffer\n");
	print_buffer8(outP);

	printf("\nFirst Test\n");
	idct_accel_calculate_buffer((uint32_t *) test1P, (uint32_t *) outP, sizeof(testBuffer1), sizeof(outputBuffer));
	print_buffer16(test1P);
	print_buffer8(outP);
	printf("Done test 1\n");

	printf("\nSecond Test\n");
	idct_accel_calculate_buffer((uint32_t *) test2P, (uint32_t *) outP, sizeof(testBuffer2), sizeof(outputBuffer));
	print_buffer16(test2P);
	print_buffer8(outP);
	printf("Done test 2\n");

	while(1){}
}
