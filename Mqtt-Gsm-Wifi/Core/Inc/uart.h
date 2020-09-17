/*
 * uart.h
 *
 *  Created on: Jul 20, 2020
 *      Author: fedepacher
 */

#ifndef INC_UART_H_
#define INC_UART_H_

#include "main.h"
#include "stdint.h"
#include <string.h>
#include "cmsis_os.h"
#include "semphr.h"
#include "ring_buffer.h"
#include "general_defs.h"

/* Private types ------------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/

/* Private constants --------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/


/* Private functions ------------------------------------------------------- */

 /* Public struct ---------------------------------------------------------*/
	RingBuffer_t WiFiRxBuffer;
	RingBuffer_t ControlBuffer;

typedef struct{
	UART_HandleTypeDef uart;
	QueueHandle_t xQueuePrintConsole;
}driver_uart_t;

typedef struct{
	uint8_t data_cmd[BUFFERSIZE_CMD];
}data_print_console_t;

typedef struct{
	uint8_t data[BUFFERSIZE_CMD];
	uint32_t length;
}data_publish_t;


 /**
  * @brief  Uart Initalization.
  *         This function inits the UART interface to deal with the esp8266,
  *         then starts asynchronous listening on the RX port.
  * @param huart	uart to be initialized
  * @param circularBuffer	circular buffer to be initialized
  * @retval None.
  */
void HAL_UART_F_Init(UART_HandleTypeDef *huart);

/**
 * @brief  Uart Initalization.
 *         This function inits the UART interface to deal with the esp8266,
 *         then starts asynchronous listening on the RX port.
 * @param huart	uart to be initialized
 * @param circularBuffer	circular buffer to be initialized
 * @retval None.
 */
void HAL_UART1_F_Init(UART_HandleTypeDef *huart);

/**
 * @brief  Uart Deinitialization.
 *         This function Deinits the UART interface of the ESP8266. When called
 *         the esp8266 commands can't be executed anymore.
 * @param  None.
 * @retval None.
 */
void HAL_UART_F_DeInit(void);

/**
 * @brief  Send Data to the ESP8266 module over the UART interface.
 *         This function allows sending data to the  ESP8266 WiFi Module, the
 *          data can be either an AT command or raw data to send over
 *			a pre-established WiFi connection.
 * @param huart		uart selected to send msg
 * @param Buffer: data to send.
 * @param Length: the data length.
 * @retval 0 on success, -1 otherwise.
 */
int8_t HAL_UART_F_Send(UART_HandleTypeDef *huart, const char* Buffer, const uint8_t Length);

/**
 * @brief  Extract info to the circular bufer
 * @param  Buffer The buffer where to fill the received data
 * @param  Length the maximum data size to receive.
 * @retval Returns ESP8266_OK on success and ESP8266_ERROR otherwise.
 */
int32_t receiveInternal(uint8_t *Buffer, uint32_t Length);

#endif /* INC_UART_H_ */
