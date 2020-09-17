/*
 * ESP_Client.c
 *
 *  Created on: May 25, 2020
 *      Author: fedepacher
 */

#include "ESP_Client.h"
#include "stdio.h"
#include <string.h>
#include <assert.h>
#include "uart.h"
#include "stdbool.h"
#include "cmsis_os.h"
#include "general_defs.h"
#include "task.h"

extern UART_HandleTypeDef huart6;	//connected to esp8266
static uint8_t ESP82_cmdBuffer[ESP_BUFFERSIZE_CMD];
uint8_t RxBuffer[ESP_BUFFERSIZE_RESPONSE];
static const char * ESP_SSLSIZE_str = "AT+CIPSSLSIZE=4096\r\n";///< ESP8266 module memory (2048 to 4096) reserved for SSL.

// Internal states.
typedef enum {
	ESP82_State0 = 0,
	ESP82_State1,
	ESP82_State2,
	ESP82_State3,
	ESP82_State4,
	ESP82_State5,
	ESP82_State6,
	ESP82_State7,
	ESP82_State8,
	ESP82_State9,
	ESP82_StateMAX,
} tESP82_State;


//static ESP8266_StatusTypeDef runAtCmd(uint8_t* cmd, uint32_t Length, const uint8_t* Token);
static ESP8266_StatusTypeDef ESP_atCommand(uint8_t* cmd, uint32_t Length, const uint8_t* Token);
static ESP8266_StatusTypeDef ESP_executeAtCmd(uint8_t* cmd, uint32_t Length);
static ESP8266_StatusTypeDef ESP_responseAtCmd(const uint8_t* Token);

static int32_t ESP_Receive(uint8_t *Buffer, uint32_t Length);
static ESP8266_StatusTypeDef ESP_getData(uint8_t* Buffer, uint32_t Length, uint32_t* RetLength);


/*
 * @brief Creates non-blocking delay.
 * @param delay_ms Delay time in ms.
 * @return SUCCESS, INPROGRESS.
 */
ESP8266_StatusTypeDef ESP_Delay(const uint16_t delay_ms){
	// Function entry.

	osDelay(delay_ms);
	return ESP8266_OK;

}

ESP8266_StatusTypeDef ESP8266_ConnectionClose(void) {
	return ESP_atCommand((uint8_t*) "AT+CIPCLOSE\r\n", 13, (uint8_t*) AT_OK_STRING);
}

/*
 * @brief Reset Module ESP.
 * @return SUCCESS, BUSY or ERROR.
 */
ESP8266_StatusTypeDef ESP_Reset(){
	ESP8266_StatusTypeDef result = ESP8266_OK;
	//result = runAtCmd((uint8_t*)"AT+RST\r\n", 8, (uint8_t*) AT_OK_STRING);
	return result;
}

/*
 * @brief Search Wifi access point.
 * @return SUCCESS, BUSY or ERROR.
 */
ESP8266_StatusTypeDef ESP_SearchWifi() {
	ESP8266_StatusTypeDef result;
	result = ESP_atCommand((uint8_t*)"AT+CWLAP\r\n", 10, (uint8_t*) AT_OK_STRING);
	return result;
}


/*
 * @brief Connect to AP.
 * @param resetToDefault If true, reset the module to default settings before connecting.
 * @param ssid AP name.
 * @param pass AP password.
 * @return SUCCESS, BUSY or ERROR.
 */
ESP8266_StatusTypeDef ESP_ConnectWifi(const bool resetToDefault, const char * ssid, const char * pass) {
	static uint8_t internalState;
	ESP8266_StatusTypeDef result;

	// State machine.
	switch (internalState = ESP82_State0) {
	case ESP82_State0:
			// Wait for startup phase to finish.
			if(ESP8266_OK == (result = ESP_Delay(ESP_TIMEOUT_MS_RESTART))) {
				// To the next state.
				internalState = ESP82_State1;
			} else {
				// INPROGRESS or SUCCESS if no reset is requested.
				return result;
			}
	//nobreak;
	case ESP82_State1:
		// AT+RESTORE (if requested).
		if(!resetToDefault || (ESP8266_OK == (result = ESP_atCommand((uint8_t*)"AT\r\n", 4, (uint8_t*) AT_OK_STRING)))) {
			// To the next state.
			internalState = ESP82_State2;
		} else {
			// Exit on ERROR or INPROGRESS.
			return result;
		}

		//nobreak;
	case ESP82_State2:
		// If resetted, wait for restart to finish.
		if(!resetToDefault || (ESP8266_OK == (result = ESP_Delay(ESP_TIMEOUT_MS_RESTART)))){
				// To the next state.
				internalState = ESP82_State3;
		}else{
			// INPROGRESS or SUCCESS if no reset is requested.
			return result;
		}

		//nobreak;
	case ESP82_State3:
		// AT+CWMODE (client mode)
		if((ESP8266_OK == (result = ESP_atCommand((uint8_t*)"AT+CWMODE=1\r\n", 13, (uint8_t*) AT_OK_STRING))) && (ssid != NULL)){
			// To the next state.
			internalState = ESP82_State4;
		} else{
			// Exit on ERROR, INPROGRESS or SUCCESS (if no SSID is provided).
			return result;
		}

		// nobreak;
	case ESP82_State4:
		// Size check.
		if ((strlen(ssid) + strlen(pass)) > (ESP_BUFFERSIZE_CMD - 17)) {
			return false;
		}

		// AT+CWJAP prepare.
		sprintf((char *)ESP82_cmdBuffer, "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, pass);

		// To the next state.
		internalState = ESP82_State5;

		//nobreak;
	case ESP82_State5:
		// AT+CWJAP
		return ESP_atCommand(ESP82_cmdBuffer, strlen((char*)ESP82_cmdBuffer), (uint8_t*) AT_OK_STRING);

		//nobreak;
	default:
		// To the first state.
		internalState = ESP82_State0;
	}
}

/*
 * @brief Connection test.
 * @return SUCCESS, INPROGRESS or ERROR.
 */
ESP8266_StatusTypeDef ESP_IsConnectedWifi(void) {
	return ESP_atCommand((uint8_t*)"AT+CIPSTATUS\r\n", 14, (uint8_t*) AT_OK_STRING);
}



/*
 * @brief Connect to server via TCP.
 * @param host Hostname or IP address.
 * @param port Remote port.
 * @param keepalive Keep-alive time between 0 to 7200 seconds.
 * @param ssl Starts SSL connection.
 * @return SUCCESS, BUSY or ERROR.
 */
ESP8266_StatusTypeDef ESP_StartTCP(const char * host, const uint16_t port, const uint16_t keepalive, const bool ssl) {
	static uint8_t internalState;
	ESP8266_StatusTypeDef result;

	// State machine.
	switch (internalState = ESP82_State0) {
	case ESP82_State0:
		// Size check.
		if(strlen(host) > (ESP_BUFFERSIZE_CMD - 34)){
			return false;
		}

		// Keepalive check.
		if(keepalive > 7200){
			return false;
		}

		// prepare AT+CIPSTART
		//sprintf((char *)ESP82_cmdBuffer, "AT+CIPSTART=\"%s\",\"%s\",%i,%i\r\n", (ssl ? "SSL" : "TCP"), host, port, keepalive);
		sprintf((char *)ESP82_cmdBuffer, "AT+CIPSTART=\"%s\",\"%s\",%i\r\n", (ssl ? "SSL" : "TCP"), host, port);


		// To the next state.
		internalState = ESP82_State1;

		//nobreak;
	case ESP82_State1:
		// AT+CIPSSLSIZE (or skip)
		if(!ssl || (ESP8266_OK == (result = ESP_atCommand((uint8_t*)ESP_SSLSIZE_str, 20, (uint8_t*) AT_OK_STRING)))){
			// To the next state.
			internalState = ESP82_State2;
		}else{
			// Exit on ERROR or INPROGRESS.
			return result;
		}
		//nobreak;
	case ESP82_State2:
		// AT+CIPSTART
		return ESP_atCommand((uint8_t*)ESP82_cmdBuffer, strlen((char*)ESP82_cmdBuffer), (uint8_t*) AT_OK_STRING);
	}
}

/**
 * @brief  Send data over the wifi connection.
 * @param  Buffer: the buffer to send
 * @param  Length: the Buffer's data size.
 * @retval Returns ESP8266_OK on success and ESP8266_ERROR otherwise.
 */
ESP8266_StatusTypeDef ESP_SendData(uint8_t* Buffer, uint32_t Length) {
	ESP8266_StatusTypeDef Ret = ESP8266_OK;

	if (Buffer != NULL) {
		//uint32_t tickStart;
		//TickType_t tickStart;

		/* Construct the CIPSEND command */
		memset(ESP82_cmdBuffer, '\0', ESP_BUFFERSIZE_CMD);
		sprintf((char *) ESP82_cmdBuffer, "AT+CIPSEND=%lu%c%c", Length  , '\r', '\n');

		/* The CIPSEND command doesn't have a return command
		 until the data is actually sent. Thus we check here whether
		 we got the '>' prompt or not. */
		Ret = ESP_atCommand(ESP82_cmdBuffer, strlen((char *) ESP82_cmdBuffer),
				(uint8_t*) AT_SEND_PROMPT_STRING);

		/* Return Error */
		if (Ret != ESP8266_OK) {
			return ESP8266_ERROR;
		}

		/* Wait before sending data. */
		ESP_Delay(250); //not blocking delay

		/* Send the data */
		Ret = ESP_atCommand(Buffer, Length, (uint8_t*) AT_SEND_OK_STRING);//AT_IPD_STRING);//
	}

	return Ret;
}

/*
 * @brief Receive data over mqtt.
 * @param Buffer data buffer.
 * @param Length data buffer length.
 * @param RetLength data length received.
 * @return SUCCESS or ERROR.
 */
ESP8266_StatusTypeDef ESP_ReceiveData(uint8_t* Buffer, uint32_t Length,
		uint32_t* RetLength) {
	ESP8266_StatusTypeDef Ret;

	/* Receive the data from the host */
	Ret = ESP_getData(Buffer, Length, RetLength);

	return Ret;
}


/**
 * @brief  MEF to send and recieve AT command
 * @param  cmd the buffer to fill will the received data.
 * @param  Length the maximum data size to receive.
 * @param  Token the expected output if command runs successfully
 * @retval Returns ESP8266_OK on success and ESP8266_ERROR otherwise.
 */
ESP8266_StatusTypeDef ESP_atCommand(uint8_t* cmd, uint32_t Length, const uint8_t* Token) {
	static uint8_t internalState;
	ESP8266_StatusTypeDef result;

	// State machine.
	switch (internalState = ESP82_State0) {
	case ESP82_State0:

		result = ESP_executeAtCmd(cmd, Length);

		// To the next state.
		if(result == ESP8266_OK)
			internalState = ESP82_State1;
	case ESP82_State1:
		return ESP_responseAtCmd(Token);

	}
}


/**
 * @brief  Run the AT command
 * @param  cmd the buffer to fill will the received data.
 * @param  Length the maximum data size to receive.
 * @retval Returns ESP8266_OK on success and ESP8266_ERROR otherwise.
 */
static ESP8266_StatusTypeDef ESP_executeAtCmd(uint8_t* cmd, uint32_t Length) {

	/* Send the command */
	if (HAL_UART_F_Send(&huart6, (char*)cmd, Length) < 0) {
		return ESP8266_ERROR;
	}
	return ESP8266_OK;
}


/**
 * @brief  Wait response to the AT command
 * @param  Token the expected output if command runs successfully
 * @retval Returns ESP8266_OK on success and ESP8266_ERROR otherwise.
 */
static ESP8266_StatusTypeDef ESP_responseAtCmd(const uint8_t* Token) {
	uint32_t idx = 0;
	uint8_t RxChar;
	uint8_t status_io = 0;//0 is ok

	/* Reset the Rx buffer to make sure no previous data exist */
	memset(RxBuffer, '\0', ESP_BUFFERSIZE_RESPONSE);


	//uint32_t currentTime = 0;
	/* Wait for reception */
	//do {
	while(1){
		/* Wait to recieve data */
		if (ESP_Receive(&RxChar, 1) != 0) {
			RxBuffer[idx++] = RxChar;
		} else {
			status_io = 1;
			break;
		}

		/* Check that max buffer size has not been reached */
		if (idx == ESP_BUFFERSIZE_RESPONSE) {
			status_io = 1;
			break;
		}

		/* Extract the Token */
		if (strstr((char *) RxBuffer, (char *) Token) != NULL) {
			status_io = 0;
			break;
			//return ESP8266_OK;
		}

		/* Check if the message contains error code */
		if (strstr((char *) RxBuffer, AT_ERROR_STRING) != NULL) {
			status_io = 1;
			break;
			//return ESP8266_ERROR;
		}
		//currentTime++;
		//osDelay(1);
	}//while(currentTime < ESP_LONG_TIME_OUT);

	if(status_io == 1)
		return ESP8266_ERROR;
	return ESP8266_OK;
}



/**
 * @brief  Extract info to the circular bufer
 * @param  Buffer The buffer where to fill the received data
 * @param  Length the maximum data size to receive.
 * @retval Returns ESP8266_OK on success and ESP8266_ERROR otherwise.
 */
static int32_t ESP_Receive(uint8_t *Buffer, uint32_t Length) {
	uint32_t ReadData = 0;
	/* Loop until data received */
	while (Length--) {
		//uint32_t tickStart = HAL_GetTick();
		TickType_t tickStart = xTaskGetTickCount();
		//uint32_t currentTime = 0;
		do {
			if (WiFiRxBuffer.head != WiFiRxBuffer.tail) {
				/* serial data available, so return data to user */
				*Buffer++ = WiFiRxBuffer.data[WiFiRxBuffer.head++];

				ReadData++;

				/* check for ring buffer wrap */
				if (WiFiRxBuffer.head >= ESP_BUFFERSIZE_CIRCULAR) {
					/* Ring buffer wrap, so reset head pointer to start of buffer */
					WiFiRxBuffer.head = 0;
				}
				break;
			}
		} while((xTaskGetTickCount() - tickStart) < ESP_DEFAULT_TIME_OUT);
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
static ESP8266_StatusTypeDef ESP_getData(uint8_t* Buffer, uint32_t Length, uint32_t* RetLength) {
	uint8_t RxChar;
	uint32_t idx = 0;
	uint8_t LengthString[4];
	uint32_t LengthValue;
	uint8_t i = 0;
	ESP8266_Boolean newChunk = ESP8266_FALSE;

	/* Reset the reception data length */
	*RetLength = 0;

	/* Reset the reception buffer */
	memset(RxBuffer, '\0', ESP_BUFFERSIZE_RESPONSE);

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
		if (ESP_Receive(&RxChar, 1) != 0) {
			/* The data chunk starts with +IPD,<chunk length>: */
			if (newChunk == ESP8266_TRUE) {
				/* Read the next lendthValue bytes as part from the actual data. */
				if (LengthValue--) {
					*Buffer++ = RxChar;
					(*RetLength)++;
				} else {
					/* Clear the buffer as the new chunk has ended. */
					newChunk = ESP8266_FALSE;
					memset(RxBuffer, '\0', ESP_BUFFERSIZE_RESPONSE);
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

		if (idx == ESP_BUFFERSIZE_RESPONSE) {
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
				ESP_Receive(&RxChar, 1);
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
	}while(currentTime < ESP_LONG_TIME_OUT);

	if(currentTime > ESP_LONG_TIME_OUT)
		return ESP8266_ERROR;

	return ESP8266_OK;
}
