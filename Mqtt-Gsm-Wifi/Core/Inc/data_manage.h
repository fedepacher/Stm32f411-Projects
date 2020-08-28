/*
 * data_manage.h
 *
 *  Created on: Aug 27, 2020
 *      Author: fedepacher
 */

#ifndef INC_DATA_MANAGE_H_
#define INC_DATA_MANAGE_H_

#include "general_defs.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include "cmsis_os.h"

//static bool inProgress = false;///< State flag for non-blocking functions.

// Internal states.
typedef enum {
	State0 = 0,
	State1,
	State2,
	State3,
	State4,
	State5,
	State6,
	State7,
	State8,
	State9,
	State10,
	State11,
	StateMAX,
} State_t;

/*
 * @brief Creates non-blocking delay.
 * @param delay_ms Delay time in ms.
 * @return SUCCESS, BUSY.
 */
//ESP8266_StatusTypeDef Delay(const uint16_t delay_ms);

/*
 * @brief Receive data over the wifi connection.
 * @param  Buffer: the buffer to receive
 * @param  Length: the Buffer's data size.
 * @param  RetLength: the Buffer's data length received.
 * @retval Returns ESP8266_OK on success and ESP8266_ERROR otherwise.
 */
ESP8266_StatusTypeDef ReceiveData(uint8_t* Buffer, uint32_t Length,
		uint32_t* RetLength);


 ESP8266_StatusTypeDef atCommand(uint8_t* cmd, uint32_t Length, const uint8_t* Token, uint32_t timeout);
 ESP8266_StatusTypeDef executeAtCmd(uint8_t* cmd, uint32_t Length);
 ESP8266_StatusTypeDef responseAtCmd(const uint8_t* Token, uint32_t timeout);

 int32_t receiveInternal(uint8_t *Buffer, uint32_t Length);
 ESP8266_StatusTypeDef getData(uint8_t* Buffer, uint32_t Length, uint32_t* RetLength);


#endif /* INC_DATA_MANAGE_H_ */
