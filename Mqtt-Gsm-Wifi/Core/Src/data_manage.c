/*
 * data_manage.c
 *
 *  Created on: Aug 27, 2020
 *      Author: fedepacher
 */

#include "data_manage.h"
#include "uart.h"
#include "task.h"


uint8_t RxBuffer[BUFFERSIZE_RESPONSE];






/*
 * @brief Receive data over mqtt.
 * @param Buffer data buffer.
 * @param Length data buffer length.
 * @param RetLength data length received.
 * @return SUCCESS or ERROR.
 */
ESP8266_StatusTypeDef ReceiveData(uint8_t* Buffer, uint32_t Length,
		uint32_t* RetLength) {
	ESP8266_StatusTypeDef Ret;

	/* Receive the data from the host */
	Ret = getData(Buffer, Length, RetLength);

	return Ret;
}


/**
 * @brief  MEF to send and recieve AT command
 * @param  cmd the buffer to fill will the received data.
 * @param  Length the maximum data size to receive.
 * @param  Token the expected output if command runs successfully
 * @retval Returns ESP8266_OK on success and ESP8266_ERROR otherwise.
 */
ESP8266_StatusTypeDef atCommand(uint8_t* cmd, uint32_t Length, const uint8_t* Token, uint32_t timeout) {
	uint8_t internalState = State0;
	ESP8266_StatusTypeDef result;

	// State machine.
	switch (internalState) {
	case State0:

		result = executeAtCmd(cmd, Length);

		osDelay(250 / portTICK_PERIOD_MS);
		// To the next state.
		if(result == ESP8266_OK)
			internalState = State1;
	case State1:
		result = responseAtCmd(Token, timeout);
		//if(result == ESP8266_OK)
		//	internalState = State0;

	}
	return result;
}


/**
 * @brief  Run the AT command
 * @param  cmd the buffer to fill will the received data.
 * @param  Length the maximum data size to receive.
 * @retval Returns ESP8266_OK on success and ESP8266_ERROR otherwise.
 */
ESP8266_StatusTypeDef executeAtCmd(uint8_t* cmd, uint32_t Length) {
	taskENTER_CRITICAL();
	/* Send the command */
	if (HAL_UART_F_Send((char*)cmd, Length) < 0) {
		return ESP8266_ERROR;
	}
	taskEXIT_CRITICAL();
	return ESP8266_OK;
}




/**
 * @brief  Wait response to the AT command
 * @param  Token the expected output if command runs successfully
 * @retval Returns ESP8266_OK on success and ESP8266_ERROR otherwise.
 */

ESP8266_StatusTypeDef responseAtCmd(const uint8_t* Token, uint32_t timeout) {
	uint32_t idx = 0;
	uint8_t RxChar;
	ESP8266_StatusTypeDef status = ESP8266_OK;

	/* Reset the Rx buffer to make sure no previous data exist */
	memset(RxBuffer, '\0', BUFFERSIZE_RESPONSE);

	TickType_t tickStart = xTaskGetTickCount();
	TickType_t tickLapsed = 0;
	/* Wait for reception */
	do {
	//while(1){
		/* Wait to recieve data */
		if (receiveInternal(&RxChar, 1) != 0) {	//si es 0 salio por timeout
			RxBuffer[idx++] = RxChar;
		}else {
			status = ESP8266_ERROR;
			break;
		}

		/* Check that max buffer size has not been reached */
		if (idx == BUFFERSIZE_RESPONSE) {
			status = ESP8266_ERROR;
			break;
		}

		/* Extract the Token */
		if (strstr((char *) RxBuffer, (char *) Token) != NULL) {
			status = ESP8266_OK;
			break;
		}

		/* Check if the message contains error code */
		if (strstr((char *) RxBuffer, AT_ERROR_STRING) != NULL) {
			status = ESP8266_ERROR;
			break;
		}
		osDelay(1);
		tickLapsed = xTaskGetTickCount();
	} while((tickLapsed - tickStart) < timeout);//DEFAULT_TIME_OUT);

	if((tickLapsed - tickStart) >= timeout)
		status = ESP8266_TIMEOUT;

	return status;
}






/**
 * @brief  Extract info to the circular bufer
 * @param  Buffer The buffer where to fill the received data
 * @param  Length the maximum data size to receive.
 * @retval Returns ESP8266_OK on success and ESP8266_ERROR otherwise.
 */
int32_t receiveInternal(uint8_t *Buffer, uint32_t Length) {
	uint32_t ReadData = 0;
	/* Loop until data received */
	while (Length--) {
		//TickType_t tickStart = xTaskGetTickCount();
		//uint32_t currentTime = 0;
		//do {
			//taskENTER_CRITICAL();
			if (WiFiRxBuffer.head != WiFiRxBuffer.tail) {
				/* serial data available, so return data to user */
				*Buffer++ = WiFiRxBuffer.data[WiFiRxBuffer.head++];

				ReadData++;

				/* check for ring buffer wrap */
				if (WiFiRxBuffer.head >= BUFFERSIZE_CIRCULAR) {
					/* Ring buffer wrap, so reset head pointer to start of buffer */
					WiFiRxBuffer.head = 0;
				}
				//break;
			}
			//taskEXIT_CRITICAL();
		//} while((xTaskGetTickCount() - tickStart) < timeout);//DEFAULT_TIME_OUT);
	}

	return ReadData;
}

/**
 * @brief  Receive data from the WiFi module
 * @param  Buffer The buffer where to fill the received data
 * @param  Length the maximum data size to receive.
 * @param  RetLength Length of received data
 * @retval Returns ESP8266_OK on success and ESP8266_ERROR otherwise.
 */
ESP8266_StatusTypeDef getData(uint8_t* Buffer, uint32_t Length, uint32_t* RetLength) {
	uint8_t RxChar;
	uint32_t idx = 0;
	uint8_t LengthString[4];
	uint32_t LengthValue;
	uint8_t i = 0;
	ESP8266_Boolean newChunk = ESP8266_FALSE;

	/* Reset the reception data length */
	*RetLength = 0;

	/* Reset the reception buffer */
	memset(RxBuffer, '\0', BUFFERSIZE_RESPONSE);

	/* When reading data over a wifi connection the esp8266
	 splits it into chunks of 1460 bytes maximum each, each chunk is preceded
	 by the string "+IPD,<chunk_size>:". Thus to get the actual data we need to:
	 - Receive data until getting the "+IPD," token, a new chunk is marked.
	 - Extract the 'chunk_size' then read the next 'chunk_size' bytes as actual data
	 - Mark end of the chunk.
	 - Repeat steps above until no more data is available. */
	uint32_t currentTime = 0;
	do{
	//while(1){
		if (receiveInternal(&RxChar, 1) != 0) {
			/* The data chunk starts with +IPD,<chunk length>: */
			if (newChunk == ESP8266_TRUE) {
				/* Read the next lendthValue bytes as part from the actual data. */
				if (LengthValue--) {
					*Buffer++ = RxChar;
					(*RetLength)++;
				} else {
					/* Clear the buffer as the new chunk has ended. */
					newChunk = ESP8266_FALSE;
					memset(RxBuffer, '\0', BUFFERSIZE_RESPONSE);
					idx = 0;
				}
			}
			RxBuffer[idx++] = RxChar;
		} else {
			/* Errors while reading return an error. */
			if ((newChunk == ESP8266_TRUE) && (LengthValue != 0)) {
				return ESP8266_ERROR;
			} else {
				break;
			}
		}

		if (idx == BUFFERSIZE_RESPONSE) {
			/* In case of Buffer overflow, return error */
			if ((newChunk == ESP8266_TRUE) && (LengthValue != 0)) {
				return ESP8266_ERROR;
			} else {
				break;
			}
		}

		/* When a new chunk is met, extact its size */
		if ((strstr((char *) RxBuffer, AT_IPD_STRING) != NULL)
				&& (newChunk == ESP8266_FALSE)) {
			i = 0;
			memset(LengthString, '\0', 4);
			do {
				receiveInternal(&RxChar, 1);
				LengthString[i++] = RxChar;
			} while (RxChar != ':');

			/* Get the buffer length */
			LengthValue = atoi((char *) LengthString);

			newChunk = ESP8266_TRUE;
		}

		/* Check if message contains error code */
		if (strstr((char *) RxBuffer, AT_ERROR_STRING) != NULL) {
			return ESP8266_ERROR;
		}

		/* Check for the chunk end */
		if (strstr((char *) RxBuffer, AT_IPD_OK_STRING) != NULL) {
			newChunk = ESP8266_FALSE;
		}
		currentTime++;
		osDelay(1);
	}while(currentTime < CMD_TIMEOUT_15000);

	if(currentTime > CMD_TIMEOUT_15000)
		return ESP8266_ERROR;

	return ESP8266_OK;
}
