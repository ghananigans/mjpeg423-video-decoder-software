/*
 * timer.c
 *
 *  Created on: Jan 24, 2017
 *      Author: ggowripa
 */

#include "timer.h"
#include "common/utils.h"
#include <system.h>
#include <sys/alt_irq.h>
#include "altera_avalon_timer_regs.h"
#include <stdio.h>

static void (*timerTickIsrFunc)(void) = 0;

static void timerIrq (void* isr_context) {
	IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_0_1_BASE, 0x00);
	timerTickIsrFunc();
}

int initTimer (unsigned int microseconds, void (*timerTickFunc)(void)) {
	alt_u64 timerPeriod;
	int retVal;

	timerPeriod = (alt_u64)microseconds * (alt_u64)TIMER_0_1_FREQ / 1000000;

	// Initialize timer interrupt vector
	timerTickIsrFunc = timerTickFunc;

	// Initalize timer period
	IOWR_ALTERA_AVALON_TIMER_PERIOD_0(TIMER_0_1_BASE, (timerPeriod & ALTERA_AVALON_TIMER_PERIOD_0_MSK));
	IOWR_ALTERA_AVALON_TIMER_PERIOD_1(TIMER_0_1_BASE, ((timerPeriod >> 16) & ALTERA_AVALON_TIMER_PERIOD_1_MSK));
	IOWR_ALTERA_AVALON_TIMER_PERIOD_2(TIMER_0_1_BASE, ((timerPeriod >> 32) & ALTERA_AVALON_TIMER_PERIOD_2_MSK));
	IOWR_ALTERA_AVALON_TIMER_PERIOD_3(TIMER_0_1_BASE, ((timerPeriod >> 48) & ALTERA_AVALON_TIMER_PERIOD_3_MSK));

	// clear interrupt bit in status register
	IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_0_1_BASE, 0x00);

	IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER_0_1_BASE, ALTERA_AVALON_TIMER_CONTROL_ITO_MSK
			| ALTERA_AVALON_TIMER_CONTROL_CONT_MSK);

	retVal = alt_ic_isr_register(TIMER_0_1_IRQ_INTERRUPT_CONTROLLER_ID, TIMER_0_1_IRQ, &timerIrq, NULL, NULL) ? 0 : 1;
	return retVal;
}

void startTimer (void) {
	DBG_PRINT("Starting Timer\n");

	IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER_0_1_BASE, ALTERA_AVALON_TIMER_CONTROL_ITO_MSK
			| ALTERA_AVALON_TIMER_CONTROL_CONT_MSK | ALTERA_AVALON_TIMER_CONTROL_START_MSK);
}

void stopTimer (void) {
	DBG_PRINT("Stopping Timer\n");

	IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER_0_1_BASE, ALTERA_AVALON_TIMER_CONTROL_STOP_MSK);
}
