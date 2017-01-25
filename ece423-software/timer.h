/*
 * timer.h
 *
 *  Created on: Jan 24, 2017
 *      Author: ggowripa
 */

#ifndef TIMER_H_
#define TIMER_H_


int initTimer (unsigned int milliseconds, void (*timerTickFunc)(void));
void startTimer (void);
void stopTimer (void);

#endif /* TIMER_H_ */
