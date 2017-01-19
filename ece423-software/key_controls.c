/*
 * key_controls.c
 *
 *  Created on: Jan 18, 2017
 *      Author: ggowripa
 */

#include "key_controls.h"
#include "system.h"
#include "sys/alt_irq.h"
#include "drivers/inc/altera_avalon_pio_regs.h"

static volatile int lastButtonPressed = 0;

static void keyIrq (void* isr_context) {
	//
	// If the last button pressed hasnt been processed ignore this button
	//
	if (lastButtonPressed == 0) {
		lastButtonPressed = IORD_ALTERA_AVALON_PIO_EDGE_CAP(KEY_BASE);
	}

	//
	// Clear the edge capture register
	//
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(KEY_BASE, 0x0);

	// From the nios II handbook:
	// Read the PIO to delay ISR exit. This is done to prevent a spurious interrupt in systems
	// with high processor -> pio latency and fast interrupts.
	//
	IORD_ALTERA_AVALON_PIO_EDGE_CAP(KEY_BASE);
}

int initKeyIrq (void) {
	int retVal;

	//
	// Enable interrupts
	//
	IOWR_ALTERA_AVALON_PIO_IRQ_MASK(KEY_BASE, 0xF);

	//
	// Clear edge capture
	//
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(KEY_BASE, 0x0);

	//
	// To match our 1 = success and 0 = failure convention
	//
	retVal = alt_ic_isr_register(KEY_IRQ_INTERRUPT_CONTROLLER_ID, KEY_IRQ, keyIrq, NULL, NULL) ? 0 : 1;

	return retVal;
}

int waitForButtonPress (void) {
	int tempLastButtonPressed;

	//
	// Wait for the next button to be pressed (blocking)
	//
	while (lastButtonPressed == 0) {}

	//
	// Reset last button pressed to nothing (0)
	//
	tempLastButtonPressed = lastButtonPressed;
	lastButtonPressed = 0;

	return tempLastButtonPressed;
}

int buttonHasBeenPressed (void) {
	return lastButtonPressed;
}
