/*
 * key_controls.h
 *
 *  Created on: Jan 18, 2017
 *      Author: ggowripa
 */

#ifndef KEY_CONTROLS_H_
#define KEY_CONTROLS_H_

#include <stdint.h>

int initKeyIrq();
int buttonHasBeenPressed();
int waitForButtonPress();

#endif /* KEY_CONTROLS_H_ */
