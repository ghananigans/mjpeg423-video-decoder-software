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

static void keyIrq (void* isr_context){

	// if the last button pressed hasnt been processed ignore this button
	if(lastButtonPressed == 0){
		lastButtonPressed = IORD_ALTERA_AVALON_PIO_EDGE_CAP(KEY_BASE);
	}

	// clear the edge capture register
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(KEY_BASE, 0x0);

	/* From the nios II handbook:
	 * Read the PIO to delay ISR exit. This is done to prevent a spurious interrupt in systems
	 * with high processor -> pio latency and fast interrupts.*/
	IORD_ALTERA_AVALON_PIO_EDGE_CAP(KEY_BASE);
}

int initKeyIrq(){
	int retVal;

	// enable interrupts
	IOWR_ALTERA_AVALON_PIO_IRQ_MASK(KEY_BASE, 0xF);
	// clear edge capture
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(KEY_BASE, 0x0);

	// to match our 1 = success and 0 = failure convention
	retVal = alt_ic_isr_register(KEY_IRQ_INTERRUPT_CONTROLLER_ID, KEY_IRQ, keyIrq, NULL, NULL) ? 0 : 1;

	return retVal;
}

int waitForButtonPress(){
	int tempLastButtonPressed;

	while(lastButtonPressed == 0){}
	tempLastButtonPressed = lastButtonPressed;
	lastButtonPressed = 0;

	return tempLastButtonPressed;
}

int buttonHasBeenPressed(){
	return lastButtonPressed;
}
