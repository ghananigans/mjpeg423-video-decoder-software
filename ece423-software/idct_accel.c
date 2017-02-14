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

#define DESC_CONTROL_FROM_IDCT_ACCEL      (ALTERA_MSGDMA_DESCRIPTOR_CONTROL_TRANSFER_COMPLETE_IRQ_MASK | ALTERA_MSGDMA_DESCRIPTOR_CONTROL_GENERATE_EOP_MASK | ALTERA_MSGDMA_DESCRIPTOR_CONTROL_GO_MASK)
#define DESC_CONTROL_TO_IDCT_ACCEL        (ALTERA_MSGDMA_DESCRIPTOR_CONTROL_GENERATE_EOP_MASK | ALTERA_MSGDMA_DESCRIPTOR_CONTROL_GO_MASK)



typedef struct mdma {
	alt_msgdma_standard_descriptor desc;
	alt_msgdma_dev* dev;
} mdma_t;


static mdma_t from_accel;
static mdma_t to_accel;

static bool done1 = 0;
static bool done2 = 0;

static void dmaToIrq (void* isr_context){
	//IOWR_ALTERA_MSGDMA_CSR_STATUS(MDMA_TO_IDCT_ACCEL_CSR_BASE, 0x1);
	done1 = 0;
}

static void dmaFromIrq (void* isr_context){
	//IOWR_ALTERA_MSGDMA_CSR_STATUS(MDMA_FROM_IDCT_ACCEL_CSR_BASE, 0x1);
	done2 = 0;
}


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

	//alt_msgdma_register_callback(from_accel.dev, &dmaFromIrq, ALTERA_MSGDMA_CSR_GLOBAL_INTERRUPT_MASK, NULL);
	//alt_msgdma_register_callback(to_accel.dev, &dmaToIrq, ALTERA_MSGDMA_CSR_GLOBAL_INTERRUPT_MASK, NULL);

	return retVal;
}

void idct_accel_calculate_buffer (uint32_t* inputBuffer, uint32_t* outputBuffer, uint32_t sizeOfBuffers){
	int retVal;

	//
	// Flush input buffer cache
	//
	alt_dcache_flush(inputBuffer, sizeOfBuffers);

	retVal = alt_msgdma_construct_standard_mm_to_st_descriptor(to_accel.dev,
					&to_accel.desc,(alt_u32 *)inputBuffer, sizeOfBuffers, 0);
	if (retVal) {
		DBG_PRINT("ERROR %d\n", retVal);
	}

	retVal = alt_msgdma_construct_standard_st_to_mm_descriptor(from_accel.dev,
					&from_accel.desc,(alt_u32 *)outputBuffer, sizeOfBuffers, 0);
	if (retVal) {
		DBG_PRINT("ERROR %d\n", retVal);
	}

	//printf("Attempting DMA transfer...\n");

	retVal = alt_msgdma_standard_descriptor_sync_transfer(to_accel.dev, &to_accel.desc);
	if (retVal) {
		DBG_PRINT("ERROR %d\n", retVal);
	}else{
		//DBG_PRINT("Successful write\n");
	}

	retVal = alt_msgdma_standard_descriptor_sync_transfer(from_accel.dev, &from_accel.desc);
	if (retVal) {
		DBG_PRINT("ERROR %d\n", retVal);
	}else{
		//DBG_PRINT("Successful read\n");
	}
}

