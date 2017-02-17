/*
 * ycbr_to_rgb_accel.c
 *
 *  Created on: Feb 16, 2017
 *      Author: ggowripa
 */

#include "ycbcr_to_rgb_accel.h"
#include "utils.h"

#include "system.h"
#include <sys/alt_cache.h>
#include <sys/alt_irq.h>

#include "altera_msgdma.h"
#include "altera_msgdma_descriptor_regs.h"
#include "altera_msgdma_csr_regs.h"
#include <stdbool.h>

#include "mdma.h"

static mdma_t from_accel;
static mdma_t to_accel_y;
static mdma_t to_accel_cb;
static mdma_t to_accel_cr;

#ifdef YCBCR_TO_RGB_HW_ACCEL
static bool volatile done = 0;

static void dmaFromIrq (void* isr_context){
	IOWR_ALTERA_MSGDMA_CSR_STATUS(MDMA_FROM_IDCT_ACCEL_CSR_BASE, 0x1);
	done = 1;
}
#else // #ifdef YCBCR_TO_RGB_HW_ACCEL
typedef union rgb_pixel_t_uint32_t_union {
	uint32_t data;
	rgb_pixel_t rgb;
} rgb_pixel_t_uint32_t_union_t;
#endif // #ifdef YCBCR_TO_RGB_HW_ACCEL

int init_ycbcr_to_rgb_accel (void) {
#ifdef YCBCR_TO_RGB_HW_ACCEL
	from_accel.dev = alt_msgdma_open(MDMA_FROM_YCBCR_TO_RGB_ACCEL_CSR_NAME);
	assert(from_accel.dev, "MDMA from failed to open\n");

	to_accel_y.dev   = alt_msgdma_open(MDMA_TO_YCBCR_TO_RGB_ACCEL_Y_CSR_NAME);
	assert(to_accel_y.dev, "MDMA to failed to open\n");

	to_accel_cb.dev   = alt_msgdma_open(MDMA_TO_YCBCR_TO_RGB_ACCEL_CB_CSR_NAME);
	assert(to_accel_cb.dev, "MDMA to failed to open\n");

	to_accel_cr.dev   = alt_msgdma_open(MDMA_TO_YCBCR_TO_RGB_ACCEL_CR_CSR_NAME);
	assert(to_accel_cr.dev, "MDMA to failed to open\n");

	alt_msgdma_register_callback(from_accel.dev, &dmaFromIrq, ALTERA_MSGDMA_CSR_GLOBAL_INTERRUPT_MASK, NULL);
#endif // #ifdef YCBCR_TO_RGB_HW_ACCEL

	return 1;
}

void ycbcr_to_rgb_accel_calculate_buffer(color_block_t* yBlock, color_block_t* crBlock, color_block_t* cbBlock,
		rgb_pixel_t* outputBuffer, int hCb_size, int wCb_size, int w_size) {

#ifdef YCBCR_TO_RGB_HW_ACCEL
	int retVal;

	retVal = alt_msgdma_construct_standard_mm_to_st_descriptor(to_accel_y.dev,
						&to_accel_y.desc,(alt_u32 *)yBlock, hCb_size*wCb_size*sizeof(color_block_t),
						0);
	assert(retVal == 0, "ERROR: %d\n", retVal);

	retVal = alt_msgdma_construct_standard_mm_to_st_descriptor(to_accel_cr.dev,
						&to_accel_cr.desc,(alt_u32 *)crBlock, hCb_size*wCb_size*sizeof(color_block_t),
						0);
	assert(retVal == 0, "ERROR: %d\n", retVal);

	retVal = alt_msgdma_construct_standard_mm_to_st_descriptor(to_accel_cb.dev,
						&to_accel_cb.desc,(alt_u32 *)cbBlock, hCb_size*wCb_size*sizeof(color_block_t),
						0);
	assert(retVal == 0, "ERROR: %d\n", retVal);

	retVal = alt_msgdma_construct_standard_st_to_mm_descriptor(from_accel.dev,
					&from_accel.desc,(alt_u32 *)outputBuffer, DISPLAY_HEIGHT*DISPLAY_WIDTH*sizeof(rgb_pixel_t),
					ALTERA_MSGDMA_DESCRIPTOR_CONTROL_TRANSFER_COMPLETE_IRQ_MASK);
	assert(retVal == 0, "ERROR: %d\n", retVal);

	retVal = alt_msgdma_standard_descriptor_async_transfer(to_accel_y.dev, &to_accel_y.desc);
	assert(retVal == 0, "ERROR: %d\n", retVal);

	retVal = alt_msgdma_standard_descriptor_async_transfer(to_accel_cr.dev, &to_accel_cr.desc);
	assert(retVal == 0, "ERROR: %d\n", retVal);

	retVal = alt_msgdma_standard_descriptor_async_transfer(to_accel_cb.dev, &to_accel_cb.desc);
	assert(retVal == 0, "ERROR: %d\n", retVal);

	retVal = alt_msgdma_standard_descriptor_async_transfer(from_accel.dev, &from_accel.desc);
	assert(retVal == 0, "ERROR: %d\n", retVal);

	//
	// Wait for module to finish
	//
	while(done == 0);
	done = 0;

	printf("%d %d %d %d\n", crBlock[0][0][2], yBlock[0][0][2], cbBlock[0][0][2], 0);
	printf("%d %d %d %d\n", outputBuffer[2].red, outputBuffer[2].green, outputBuffer[2].blue, outputBuffer[2].alpha);
#else // #ifdef YCBCR_TO_RGB_HW_ACCEL
    for (int h = 0; h < hCb_size; h++){
		for (int w = 0; w < wCb_size; w++) {
			int b = h * wCb_size + w;

			for (int y = 0; y < 8; y++){
				int index = ((h << 3) + y) * w_size + (w << 3);

				for(int x = 0; x < 8; x++){
					rgb_pixel_t_uint32_t_union_t t;
					//rgb_pixel_t pixel;
					//pixel.alpha = 0;
					//pixel.red = crBlock[b][y][x];
					//pixel.green = yBlock[b][y][x];
					//pixel.blue = cbBlock[b][y][x];
					//outputBuffer[index] = pixel;
					t.data = ((((uint32_t)crBlock[b][y][x] & 0xFF) << 16) | (((uint32_t)yBlock[b][y][x] & 0xFF) << 8) | (((uint32_t)cbBlock[b][y][x] & 0xFF)));
					outputBuffer[index] = t.rgb;
					index++;
				}
			}
		}
    }
#endif // #ifdef YCBCR_TO_RGB_HW_ACCEL
}
