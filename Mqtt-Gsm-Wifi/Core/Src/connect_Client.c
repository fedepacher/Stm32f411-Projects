/*
 * Client.c
 *
 *  Created on: May 25, 2020
 *      Author: fedepacher
 */

#include "connect_Client.h"
#include "stdio.h"
#include <string.h>
#include <assert.h>
#include "uart.h"
#include "stdbool.h"
#include "cmsis_os.h"
#include "task.h"




#define VERSION					"version"
#define PDPCID					"pdpcid"
#define WILL					"will"
#define TIMEOUT					"timeout"
#define SESSION					"session"
#define KEEPALIVE				"keepalive"
#define SSL						"ssl"


//static unsigned long int (* getTime_ms)(void);///< Used to hold handler for time provider.
//static unsigned long int t0;///< Keeps entry time for timeout detection.
static TickType_t t0;
static bool inProgress = false;///< State flag for non-blocking functions.
static uint8_t cmdBuffer[BUFFERSIZE_CMD];
uint8_t RxBuffer[BUFFERSIZE_RESPONSE];


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


static ESP8266_StatusTypeDef atCommand(uint8_t* cmd, uint32_t Length, const uint8_t* Token, uint32_t timeout);
static ESP8266_StatusTypeDef executeAtCmd(uint8_t* cmd, uint32_t Length);
static ESP8266_StatusTypeDef responseAtCmd(const uint8_t* Token, uint32_t timeout);

static int32_t receiveInternal(uint8_t *Buffer, uint32_t Length, uint32_t timeout);
static ESP8266_StatusTypeDef getData(uint8_t* Buffer, uint32_t Length, uint32_t* RetLength);

/*
 * @brief INTERNAL Timeout setup.
 */
static void timeoutBegin(void){
	// Get entry time.
	t0 = xTaskGetTickCount();
}


/*
 * @brief INTERNAL Timeout checker.
 * @param interval_ms Interval time in ms.
 * @return True if timeout expired.
 */
static bool timeoutIsExpired(const uint16_t interval_ms) {
	// Check if the given interval is in the past.
	return (interval_ms < (xTaskGetTickCount() - t0));

}

/*
 * @brief Creates non-blocking delay.
 * @param delay_ms Delay time in ms.
 * @return SUCCESS, INPROGRESS.
 */
ESP8266_StatusTypeDef Delay_t(const uint16_t delay_ms){
	// Function entry.
	if(!inProgress){
		// Start timeout.
		timeoutBegin();
	}

	inProgress = !timeoutIsExpired(delay_ms);

	if(inProgress)
		return ESP8266_BUSY;
	else
		return ESP8266_OK;

}
/*
 * @brief Reset Module ESP.
 * @return SUCCESS, BUSY or ERROR.
 */
ESP8266_StatusTypeDef Reset(){
	ESP8266_StatusTypeDef result = ESP8266_OK;
	//result = runAtCmd((uint8_t*)"AT+RST\r\n", 8, (uint8_t*) AT_OK_STRING);
	return result;
}

/*
 * @brief TCP Connect.
 * @param cmd:		Commando to send to gsm module.
 * @param length	Command's Length.
 * @param timeout	Command timeout
 * @return SUCCESS, BUSY or ERROR.
 */
ESP8266_StatusTypeDef ConnectTCP(const uint8_t * cmd, uint32_t length, uint32_t timeout) {
	ESP8266_StatusTypeDef result;

	if(strcmp((char*)cmd, (char*)END_OF_ARRAY) != 0) {
		// To the next state.
		result = atCommand((uint8_t*)cmd, length, (uint8_t*) AT_OK_STRING, timeout);
	} else {
		result = ESP8266_EOA;
	}
	return result;
}



/*
 * @brief Open MQTT broker.
 * @param host Hostname or IP address.
 * @param port Remote port.
 * @param tcpconnectID MQTT socket identifier. The range is 0-5.
 * @param vsn MQTT protocol version
 *							3 MQTT protocol v3
 *							4 MQTT protocol v4
 * @param cid The PDP to be used by the MQTT client. The range is 1-16. The default value is 1.
 * @param clean_session Configure the session type
 *							0 The server must store the subscriptions of the client after it disconnects.
 *							1 The server must discard any previously maintained information about the client and treat the connection as “clean”.
 * @param keepalive Keep-alive time. The range is 0-3600. The default value is 120. Unit: second. It defines the maximum time interval between messages received from a client. If
 * 							the server does not receive a message from the client within 1.5 times of the keep alive time period,
 * 							it disconnects the client as if the client has sent a DISCONNECT message.
 * @param sslnable Configure the MQTT SSL mode
 *							0 Use normal TCP connection for MQTT
 *							1 Use SSL TCP secure connection for MQTT
 * @return SUCCESS, BUSY or ERROR.
 */
ESP8266_StatusTypeDef MQTTOpen(const uint8_t * host, const uint16_t port, const uint8_t tcpconnectID, const uint8_t vsn, const uint8_t cid, const uint8_t clean_session,
		const uint16_t keepalive, const uint8_t sslenable) {
	static uint8_t internalState;
	ESP8266_StatusTypeDef result;


	// State machine.
	switch (internalState = (inProgress ? internalState : State0)) {
	case State0:
		// Keepalive check.
		if(keepalive > KEEPALIVE_MAX){
			return ESP8266_ERROR;
		}
		if(strlen((char*)host) > (BUFFERSIZE_CMD - 34)){
			return ESP8266_ERROR;
		}
		sprintf((char *)cmdBuffer, "AT+QMTCFG=\"%s\",%u,%u%c%c%c", VERSION, tcpconnectID, vsn, '\r', '\n', '\0');
		internalState = State1;
	case State1:
		if(ESP8266_OK == (result = atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_300))){
			// To the next state.
			internalState = State2;
		}else{
			// Exit on ERROR or INPROGRESS.
			return result;
		}
	case State2:
		sprintf((char *)cmdBuffer, "AT+QMTCFG=\"%s\",%u,%u%c%c%c", PDPCID, tcpconnectID, cid, '\r', '\n', '\0');
		if(ESP8266_OK == (result = atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_300))){
			// To the next state.
			internalState = State3;
		}else{
			// Exit on ERROR or INPROGRESS.
			return result;
		}
	case State3:
		sprintf((char *)cmdBuffer, "AT+QMTCFG=\"%s\",%u%c%c%c", WILL, tcpconnectID, '\r', '\n', '\0');
		if(ESP8266_OK == (result = atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_300))){
			// To the next state.
			internalState = State4;
		}else{
			// Exit on ERROR or INPROGRESS.
			return result;
		}
	case State4:
		sprintf((char *)cmdBuffer, "AT+QMTCFG=\"%s\",%u%c%c%c", TIMEOUT, tcpconnectID, '\r', '\n', '\0');
		if(ESP8266_OK == (result = atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_300))){
			// To the next state.
			internalState = State5;
		}else{
			// Exit on ERROR or INPROGRESS.
			return result;
		}
	case State5:
		sprintf((char *)cmdBuffer, "AT+QMTCFG=\"%s\",%u,%u%c%c%c", SESSION, tcpconnectID, clean_session, '\r', '\n', '\0');
		if(ESP8266_OK == (result = atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_300))){
			// To the next state.
			internalState = State6;
		}else{
			// Exit on ERROR or INPROGRESS.
			return result;
		}
	case State6:
		sprintf((char *)cmdBuffer, "AT+QMTCFG=\"%s\",%u,%u%c%c%c", KEEPALIVE, tcpconnectID, keepalive, '\r', '\n', '\0');
		if(ESP8266_OK == (result = atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_300))){
			// To the next state.
			internalState = State7;
		}else{
			// Exit on ERROR or INPROGRESS.
			return result;
		}
	case State7:
		sprintf((char *)cmdBuffer, "AT+QMTCFG=\"%s\",%u,%u%c%c%c", SSL, tcpconnectID, sslenable, '\r', '\n', '\0');
		if(ESP8266_OK == (result = atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_300))){
			// To the next state.
			internalState = State8;
		}else{
				// Exit on ERROR or INPROGRESS.
				return result;
			}
	case State8:
		sprintf((char *)cmdBuffer, "AT+QMTOPEN=%u,\"%s\",%d%c%c%c", tcpconnectID, host,  port, '\r', '\n', '\0');
		if(ESP8266_OK == (result = atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_75000))){
			// To the next state.
			internalState = State9;
		}else{
			// Exit on ERROR or INPROGRESS.
			return result;
		}
	case State9:
		sprintf((char *)cmdBuffer, "AT+QMTOPEN?%c%c%c", '\r', '\n', '\0');
		if(ESP8266_OK == (result = atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_75000))){
			return result;
		}
	}
	return result;
}

/*
 * @brief Connect to MQTT broker.
 * @param clientId The client identifier string.
 * @param userName User name of the client. It can be used for authentication.
 * @param password Password corresponding to the user name of the client. It can be used for authentication.
 * @param tcpconnectID MQTT socket identifier. The range is 0-5.
 * @return SUCCESS, BUSY or ERROR.
 */
ESP8266_StatusTypeDef MQTTConnect(const uint8_t * clientId, const uint8_t * userName, const uint8_t * password, const uint8_t tcpconnectID){
	static uint8_t internalState;
	ESP8266_StatusTypeDef result;

	switch (internalState = (inProgress ? internalState : State0)) {
		case State0:

			sprintf((char *)cmdBuffer, "AT+QMTCONN=%u,\"%s\",\"%s\",\"%s\"%c%c%c",tcpconnectID, clientId, userName, password,'\r', '\n', '\0');

			internalState = State1;

		case State1:
			if(ESP8266_OK == (result = atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_15000))){
				return result;
			}
	}
	return result;
}
/**
 * @brief  Publish data over GSM connection.
 * @param  tcpconnectID:MQTT socket identifier. The range is 0-5.
 * @param  msgID: 		Message identifier of packet. The range is 0-65535. It will be 0 only when <qos>=0.
 * @param  qos:			The QoS level at which the client wants to publish the messages.
 * 							0 At most once
 * 							1 At least once
 * 							2 Exactly once
 * @param  retain:		Whether or not the server will retain the message after it has been delivered to the current subscribers.
 *							0 The server will not retain the message after it has been delivered to the current subscribers
 *							1 The server will retain the message after it has been delivered to the current subscribers
 * @param  topic:		Topic that needs to be published
 * @param  dataBuffer:	Message to be published
 * @param  Length:		Data length to be published
 * @retval Returns ESP8266_OK on success and ESP8266_ERROR otherwise.
 */
ESP8266_StatusTypeDef PubData(uint8_t tcpconnectID, uint32_t msgID, uint8_t qos, uint8_t retain, uint8_t* topic, uint8_t* dataBuffer, uint32_t Length) {
	ESP8266_StatusTypeDef Ret = ESP8266_ERROR;

	if (dataBuffer != NULL) {
		memset(cmdBuffer, '\0', BUFFERSIZE_CMD);
		sprintf((char *) cmdBuffer, "AT+QMTPUB=%u,%lu,%u,%u,\"%s\"%c%c", tcpconnectID, (unsigned long)msgID, qos, retain, topic, '\r', '\n');

		/* The QMTPUB command doesn't have a return command
		 until the data is actually sent. Thus we check here whether
		 we got the '>' prompt or not. */
		Ret = atCommand(cmdBuffer, strlen((char *) cmdBuffer), (uint8_t*) AT_SEND_PROMPT_STRING, 5000U);

		/* Return Error */
		if (Ret != ESP8266_OK) {
			return ESP8266_ERROR;
		}

		/* Wait before sending data. */
		//osDelay(1000); //not blocking delay

		/* Send the data */
		Ret = atCommand(dataBuffer, Length, (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_5000);//AT_IPD_STRING);//
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
static ESP8266_StatusTypeDef atCommand(uint8_t* cmd, uint32_t Length, const uint8_t* Token, uint32_t timeout) {
	static uint8_t internalState;
	ESP8266_StatusTypeDef result;

	// State machine.
	switch (internalState = (inProgress ? internalState : State0)) {
	case State0:

		result = executeAtCmd(cmd, Length);

		// To the next state.
		if(result == ESP8266_OK)
			internalState = State1;
	case State1:
		result = responseAtCmd(Token, timeout);

	}
	return result;
}


/**
 * @brief  Run the AT command
 * @param  cmd the buffer to fill will the received data.
 * @param  Length the maximum data size to receive.
 * @retval Returns ESP8266_OK on success and ESP8266_ERROR otherwise.
 */
static ESP8266_StatusTypeDef executeAtCmd(uint8_t* cmd, uint32_t Length) {
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
static ESP8266_StatusTypeDef responseAtCmd(const uint8_t* Token, uint32_t timeout) {
	uint32_t idx = 0;
	uint8_t RxChar;
	ESP8266_StatusTypeDef status = ESP8266_OK;

	/* Reset the Rx buffer to make sure no previous data exist */
	memset(RxBuffer, '\0', BUFFERSIZE_RESPONSE);

	TickType_t tickStart = xTaskGetTickCount();
	/* Wait for reception */
	do {
	//while(1){
		/* Wait to recieve data */
		if (receiveInternal(&RxChar, 1, timeout) != 0) {	//si es 0 salio por timeout
			RxBuffer[idx++] = RxChar;
		} else {
			status = ESP8266_TIMEOUT;
			//break;
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
	} while((xTaskGetTickCount() - tickStart) < timeout);//DEFAULT_TIME_OUT);

	return status;
}



/**
 * @brief  Extract info to the circular bufer
 * @param  Buffer The buffer where to fill the received data
 * @param  Length the maximum data size to receive.
 * @retval Returns ESP8266_OK on success and ESP8266_ERROR otherwise.
 */
static int32_t receiveInternal(uint8_t *Buffer, uint32_t Length, uint32_t timeout) {
	uint32_t ReadData = 0;
	/* Loop until data received */
	while (Length--) {
		//TickType_t tickStart = xTaskGetTickCount();
		//uint32_t currentTime = 0;
		//do {
			taskENTER_CRITICAL();
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
			taskEXIT_CRITICAL();
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
static ESP8266_StatusTypeDef getData(uint8_t* Buffer, uint32_t Length, uint32_t* RetLength) {
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
		if (receiveInternal(&RxChar, 1, CMD_TIMEOUT_5000) != 0) {
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
				receiveInternal(&RxChar, 1, CMD_TIMEOUT_5000);
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
