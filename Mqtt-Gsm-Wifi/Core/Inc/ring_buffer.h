/*
 * ring_buffer.h
 *
 *  Created on: Jul 20, 2020
 *      Author: fedepacher
 */

#ifndef INC_RING_BUFFER_H_
#define INC_RING_BUFFER_H_

#include <stdint.h>

/* Public macro -------------------------------------------------------------*/
#define BUFFERSIZE_CIRCULAR 	2048UL


/* Public typedef -----------------------------------------------------------*/
typedef struct {
	uint8_t data[BUFFERSIZE_CIRCULAR];
	uint16_t tail;
	uint16_t head;
} RingBuffer_t;

/* Private variables ---------------------------------------------------------*/



#endif /* INC_RING_BUFFER_H_ */
