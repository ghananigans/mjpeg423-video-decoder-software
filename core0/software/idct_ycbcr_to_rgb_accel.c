/*
 * ycbr_to_rgb_accel.c
 *
 *  Created on: Feb 16, 2017
 *      Author: ggowripa
 */

#include "idct_ycbcr_to_rgb_accel.h"
#include "common/utils.h"
#include <system.h>

#include <altera_msgdma.h>
#include <altera_msgdma_descriptor_regs.h>
#include <altera_msgdma_csr_regs.h>
#include <stdbool.h>

#include "mdma.h"

static mdma_t from_ycbcr_to_rgb_accel;
static mdma_t to_idct_accel_y;
static mdma_t to_idct_accel_cb;
static mdma_t to_idct_accel_cr;

static void dmaFromIrq (void* isr_context){
	IOWR_ALTERA_MSGDMA_CSR_STATUS(MDMA_FROM_YCBCR_TO_RGB_ACCEL_CSR_NAME, 0x1);
}

static void idct_accel_calculate_buffer (mdma_t * to_idct_accel, void * inputBuffer, uint32_t sizeOfInputBuffer) {
	int retVal;

	retVal = alt_msgdma_construct_standard_mm_to_st_descriptor(to_idct_accel->dev,
					&to_idct_accel->desc, (alt_u32 *)inputBuffer, sizeOfInputBuffer, 0);
	assert(retVal == 0, "ERROR: %d\n", retVal);

	retVal = alt_msgdma_standard_descriptor_async_transfer(to_idct_accel->dev, &to_idct_accel->desc);
	assert(retVal == 0, "ERROR: %d\n", retVal);
}

int init_idct_ycbcr_to_rgb_accel (void) {

	from_ycbcr_to_rgb_accel.dev = alt_msgdma_open(MDMA_FROM_YCBCR_TO_RGB_ACCEL_CSR_NAME);
	assert(from_ycbcr_to_rgb_accel.dev, "MDMA from failed to open\n");

	to_idct_accel_y.dev   = alt_msgdma_open(MDMA_TO_IDCT_ACCEL_Y_CSR_NAME);
	assert(to_idct_accel_y.dev, "MDMA to failed to open\n");

	to_idct_accel_cb.dev   = alt_msgdma_open(MDMA_TO_IDCT_ACCEL_CB_CSR_NAME);
	assert(to_idct_accel_cb.dev, "MDMA to failed to open\n");

	to_idct_accel_cr.dev   = alt_msgdma_open(MDMA_TO_IDCT_ACCEL_CR_CSR_NAME);
	assert(to_idct_accel_cr.dev, "MDMA to failed to open\n");

	//
	// Interrupt setup
	//
	//alt_msgdma_register_callback(from_ycbcr_to_rgb_accel.dev, &dmaFromIrq, ALTERA_MSGDMA_CSR_GLOBAL_INTERRUPT_MASK, NULL);

	return 1;
}

void idct_accel_calculate_buffer_y (void * inputBuffer, uint32_t sizeOfInputBuffer) {
	idct_accel_calculate_buffer(&to_idct_accel_y, inputBuffer, sizeOfInputBuffer);
}

void idct_accel_calculate_buffer_cb (void * inputBuffer, uint32_t sizeOfInputBuffer) {
	idct_accel_calculate_buffer(&to_idct_accel_cb, inputBuffer, sizeOfInputBuffer);
}

void idct_accel_calculate_buffer_cr (void * inputBuffer, uint32_t sizeOfInputBuffer) {
	idct_accel_calculate_buffer(&to_idct_accel_cr, inputBuffer, sizeOfInputBuffer);
}

void ycbcr_to_rgb_accel_get_results (void * outputBuffer, uint32_t sizeOfOutputBuffer) {
	int retVal;

	retVal = alt_msgdma_construct_standard_st_to_mm_descriptor(from_ycbcr_to_rgb_accel.dev,
			&from_ycbcr_to_rgb_accel.desc, (alt_u32 *)outputBuffer, sizeOfOutputBuffer, 0);
	assert(retVal == 0, "ERROR: %d\n", retVal);

	retVal = alt_msgdma_standard_descriptor_async_transfer(from_ycbcr_to_rgb_accel.dev, &from_ycbcr_to_rgb_accel.desc);
	assert(retVal == 0, "ERROR: %d\n", retVal);
}

void wait_for_ycbcr_to_rgb_finsh (void) {
	alt_u32 csr_status = 1;

	while(csr_status & ALTERA_MSGDMA_CSR_BUSY_MASK){
		csr_status = IORD_ALTERA_MSGDMA_CSR_STATUS(from_ycbcr_to_rgb_accel.dev->csr_base);
	}
}
