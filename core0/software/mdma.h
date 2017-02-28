/*
 * mdma.h
 *
 *  Created on: Feb 16, 2017
 *      Author: ggowripa
 */

#ifndef MDMA_H_
#define MDMA_H_

#include "altera_msgdma.h"

typedef struct mdma {
	alt_msgdma_standard_descriptor desc;
	alt_msgdma_dev* dev;
} mdma_t;

#endif /* MDMA_H_ */
