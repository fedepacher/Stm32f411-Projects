/*
 * Client.c
 *
 *  Created on: May 25, 2020
 *      Author: fedepacher
 */

#include "stdio.h"
#include <string.h>
#include <assert.h>
#include <connect_ClientBG96.h>
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


extern UART_HandleTypeDef huart2;	//connected to bg96

// Internal states.
typedef enum {
	BG96_State0 = 0,
	BG96_State1,
	BG96_State2,
	BG96_State3,
	BG96_State4,
	BG96_State5,
	BG96_State6,
	BG96_State7,
	BG96_State8,
	BG96_State9,
	BG96_State10,
	BG96_State11,
	BG96_StateMAX,
} State_t;


uint8_t RxBuffer[BUFFERSIZE_RESPONSE];
static uint8_t cmdBuffer[BUFFERSIZE_CMD];


/*
 * @brief Receive data over the wifi connection.
 * @param  Buffer: the buffer to receive
 * @param  Length: the Buffer's data size.
 * @param  RetLength: the Buffer's data length received.
 * @retval Returns BG96_OK on success and BG96_ERROR otherwise.
 */
//static BG96_StatusTypeDef BG96_ReceiveData(uint8_t* Buffer, uint32_t Length,
//		uint32_t* RetLength);


static BG96_StatusTypeDef BG96_atCommand(uint8_t* cmd, uint32_t Length, const uint8_t* Token, uint32_t timeout);
static BG96_StatusTypeDef BG96_executeAtCmd(uint8_t* cmd, uint32_t Length);
static BG96_StatusTypeDef BG96_responseAtCmd(const uint8_t* Token, uint32_t timeout);

static int32_t BG96_receiveInternal(uint8_t *Buffer, uint32_t Length);
//static BG96_StatusTypeDef BG96_getData(uint8_t* Buffer, uint32_t Length, uint32_t* RetLength);


/*
 * @brief Reset Module ESP.
 * @return SUCCESS, BUSY or ERROR.
 */
BG96_StatusTypeDef BG96_Reset(){
	BG96_StatusTypeDef result = BG96_OK;
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
BG96_StatusTypeDef BG96_ConnectTCP(const uint8_t * cmd, uint32_t length, const uint8_t* Token, uint32_t timeout) {
	BG96_StatusTypeDef result;

	if(strcmp((char*)cmd, (char*)END_OF_ARRAY) != 0) {
		// To the next state.
		result = BG96_atCommand((uint8_t*)cmd, length, Token, timeout);
	} else {
		result = BG96_EOA;
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
BG96_StatusTypeDef BG96_MQTTOpen(const uint8_t * host, const uint16_t port, const uint8_t tcpconnectID, const uint8_t vsn, const uint8_t cid, const uint8_t clean_session,
		const uint16_t keepalive, const uint8_t sslenable) {
	static uint8_t internalState = BG96_State0;
	BG96_StatusTypeDef result;


	// State machine.
	switch (internalState) {
	case BG96_State0:
		// Keepalive check.
		if(keepalive > KEEPALIVE_MAX){
			return BG96_ERROR;
		}
		if(strlen((char*)host) > (BUFFERSIZE_CMD - 34)){
			return BG96_ERROR;
		}
		sprintf((char *)cmdBuffer, "AT+QMTCFG=\"%s\",%u,%u%c%c%c", VERSION, tcpconnectID, vsn, '\r', '\n', '\0');
		internalState = BG96_State1;
	case BG96_State1:
		if(BG96_OK == (result = BG96_atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_300))){
			// To the next state.
			internalState = BG96_State2;
		}else{
			// Exit on ERROR or INPROGRESS.
			return result;
		}
	case BG96_State2:
		sprintf((char *)cmdBuffer, "AT+QMTCFG=\"%s\",%u,%u%c%c%c", PDPCID, tcpconnectID, cid, '\r', '\n', '\0');
		if(BG96_OK == (result = BG96_atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_300))){
			// To the next state.
			internalState = BG96_State3;
		}else{
			// Exit on ERROR or INPROGRESS.
			return result;
		}
	case BG96_State3:
		sprintf((char *)cmdBuffer, "AT+QMTCFG=\"%s\",%u%c%c%c", WILL, tcpconnectID, '\r', '\n', '\0');
		if(BG96_OK == (result = BG96_atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_300))){
			// To the next state.
			internalState = BG96_State4;
		}else{
			// Exit on ERROR or INPROGRESS.
			return result;
		}
	case BG96_State4:
		sprintf((char *)cmdBuffer, "AT+QMTCFG=\"%s\",%u%c%c%c", TIMEOUT, tcpconnectID, '\r', '\n', '\0');
		if(BG96_OK == (result = BG96_atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_300))){
			// To the next state.
			internalState = BG96_State5;
		}else{
			// Exit on ERROR or INPROGRESS.
			return result;
		}
	case BG96_State5:
		sprintf((char *)cmdBuffer, "AT+QMTCFG=\"%s\",%u,%u%c%c%c", SESSION, tcpconnectID, clean_session, '\r', '\n', '\0');
		if(BG96_OK == (result = BG96_atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_300))){
			// To the next state.
			internalState = BG96_State6;
		}else{
			// Exit on ERROR or INPROGRESS.
			return result;
		}
	case BG96_State6:
		sprintf((char *)cmdBuffer, "AT+QMTCFG=\"%s\",%u,%u%c%c%c", KEEPALIVE, tcpconnectID, keepalive, '\r', '\n', '\0');
		if(BG96_OK == (result = BG96_atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_300))){
			// To the next state.
			internalState = BG96_State7;
		}else{
			// Exit on ERROR or INPROGRESS.
			return result;
		}
	case BG96_State7:
		sprintf((char *)cmdBuffer, "AT+QMTCFG=\"%s\",%u,%u%c%c%c", SSL, tcpconnectID, sslenable, '\r', '\n', '\0');
		if(BG96_OK == (result = BG96_atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_300))){
			// To the next state.
			internalState = BG96_State8;
		}else{
				// Exit on ERROR or INPROGRESS.
				return result;
			}
	case BG96_State8:
		sprintf((char *)cmdBuffer, "AT+QMTOPEN=%u,\"%s\",%d%c%c%c", tcpconnectID, host,  port, '\r', '\n', '\0');
		if(BG96_OK == (result = BG96_atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) QMTOPEN_STRING, CMD_TIMEOUT_75000))){
			// To the next state.
			internalState = BG96_State9;
		}else{
			// Exit on ERROR or INPROGRESS.
			return result;
		}
	case BG96_State9:
		sprintf((char *)cmdBuffer, "AT+QMTOPEN?%c%c%c", '\r', '\n', '\0');
		if(BG96_OK == (result = BG96_atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_75000))){
			internalState = BG96_State0;
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
BG96_StatusTypeDef BG96_MQTTConnect(const uint8_t * clientId, const uint8_t * userName, const uint8_t * password, const uint8_t tcpconnectID){
	static uint8_t internalState = BG96_State0;
	BG96_StatusTypeDef result;

	switch (internalState) {
		case BG96_State0:

			sprintf((char *)cmdBuffer, "AT+QMTCONN=?%c%c%c",'\r', '\n', '\0');

			internalState = BG96_State1;

		case BG96_State1:
			if(BG96_OK == (result = BG96_atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) QMTCONN_STRING, CMD_TIMEOUT_15000))){
				internalState = BG96_State2;
			}
		case BG96_State2:

			sprintf((char *)cmdBuffer, "AT+QMTCONN=%u,\"%s\",\"%s\",\"%s\"%c%c%c",tcpconnectID, clientId, userName, password,'\r', '\n', '\0');

			internalState = BG96_State3;

		case BG96_State3:
			if(BG96_OK == (result = BG96_atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) QMTCONN_STRING, CMD_TIMEOUT_15000))){
				internalState = BG96_State0;
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
 * @retval Returns BG96_OK on success and BG96_ERROR otherwise.
 */
BG96_StatusTypeDef BG96_PublishTopic(uint8_t tcpconnectID, uint32_t msgID, uint8_t qos, uint8_t retain, uint8_t* topic, uint8_t* dataBuffer, uint32_t Length) {
	BG96_StatusTypeDef Ret = BG96_ERROR;

	if (dataBuffer != NULL) {
		memset(cmdBuffer, '\0', BUFFERSIZE_CMD);
		sprintf((char *) cmdBuffer, "AT+QMTPUB=%u,%lu,%u,%u,\"%s\"%c%c", tcpconnectID, (unsigned long)msgID, qos, retain, topic, '\r', '\n');

		/* The QMTPUB command doesn't have a return command
		 until the data is actually sent. Thus we check here whether
		 we got the '>' prompt or not. */
		Ret = BG96_atCommand(cmdBuffer, strlen((char *) cmdBuffer), (uint8_t*) AT_SEND_PROMPT_STRING, 15000U);

		/* Return Error */
		if (Ret != BG96_OK) {
			return BG96_ERROR;
		}

		/* Wait before sending data. */
		//osDelay(1000); //not blocking delay

		/* Send the data */
		Ret = BG96_atCommand(dataBuffer, Length, (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_5000);//AT_IPD_STRING);//
	}

	return Ret;
}

/**
 * @brief  Subscribe topic over GSM connection.
 * @param  tcpconnectID:MQTT socket identifier. The range is 0-5.
 * @param  msgID: 		Message identifier of packet. The range is 0-65535. It will be 0 only when <qos>=0.
 * @param  topic:		Topic that needs to be published
 * @param  qos:			The QoS level at which the client wants to publish the messages.
 * 							0 At most once
 * 							1 At least once
 * 							2 Exactly once
 * @retval Returns BG96_OK on success and BG96_ERROR otherwise.
 */
BG96_StatusTypeDef BG96_SubscribeTopic(uint8_t tcpconnectID, uint32_t msgID, uint8_t* topic, uint8_t qos) {
	static uint8_t internalState = BG96_State0;
	BG96_StatusTypeDef result;

		switch (internalState) {
			case BG96_State0:

				sprintf((char *) cmdBuffer, "AT+QMTSUB=%u,%lu,\"%s\",%u%c%c", tcpconnectID, (unsigned long)msgID, topic, qos, '\r', '\n');
				internalState = BG96_State1;

			case BG96_State1:
				if(BG96_OK == (result = BG96_atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) QMTSUB_STRING, CMD_TIMEOUT_5000))){
					internalState = BG96_State0;
					return result;
				}
		}
		return result;
}

/**
 * @brief  Unsubscribe topic over GSM connection.
 * @param  tcpconnectID:MQTT socket identifier. The range is 0-5.
 * @param  msgID: 		Message identifier of packet. The range is 0-65535. It will be 0 only when <qos>=0.
 * @param  topic:		Topic that needs to be published
 * @retval Returns BG96_OK on success and BG96_ERROR otherwise.
 */
BG96_StatusTypeDef BG96_UnsubscribeTopic(uint8_t tcpconnectID, uint32_t msgID, uint8_t* topic) {
	static uint8_t internalState = BG96_State0;
	BG96_StatusTypeDef result;

		switch (internalState) {
			case BG96_State0:

				sprintf((char *) cmdBuffer, "AT+QMTUNS=%u,%lu,\"%s\"%c%c", tcpconnectID, (unsigned long)msgID, topic, '\r', '\n');
				internalState = BG96_State1;

			case BG96_State1:
				if(BG96_OK == (result = BG96_atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) QMTUNS_STRING, CMD_TIMEOUT_15000))){
					internalState = BG96_State0;
					return result;
				}
		}
		return result;
}

/**
 * @brief  Disconect a client from MQTT Server.
 * @param  tcpconnectID:MQTT identifier. The range is 0-5.
 * @retval Returns BG96_OK on success and BG96_ERROR otherwise.
 */
BG96_StatusTypeDef BG96_Disconnect(uint8_t tcpconnectID) {
	static uint8_t internalState = BG96_State0;
	BG96_StatusTypeDef result;

		switch (internalState) {
			case BG96_State0:

				sprintf((char *) cmdBuffer, "AT+QMTDISC=%u%c%c", tcpconnectID, '\r', '\n');
				internalState = BG96_State1;

			case BG96_State1:
				if(BG96_OK == (result = BG96_atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) QMTDISC_STRING, CMD_TIMEOUT_300))){
					internalState = BG96_State0;
					return result;
				}
		}
		return result;
}

/**
 * @brief  Close network for MQTT Client.
 * @param  tcpconnectID:MQTT identifier. The range is 0-5.
 * @retval Returns BG96_OK on success and BG96_ERROR otherwise.
 */
BG96_StatusTypeDef BG96_Close(uint8_t tcpconnectID) {
	static uint8_t internalState = BG96_State0;
	BG96_StatusTypeDef result;

		switch (internalState) {
			case BG96_State0:

				sprintf((char *) cmdBuffer, "AT+QMTCLOSE=%u%c%c", tcpconnectID, '\r', '\n');
				internalState = BG96_State1;

			case BG96_State1:
				if(BG96_OK == (result = BG96_atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) QMTCLOSE_STRING, CMD_TIMEOUT_300))){
					internalState = BG96_State0;
					return result;
				}
		}
		return result;
}

///*
// * @brief Receive data over mqtt.
// * @param Buffer data buffer.
// * @param Length data buffer length.
// * @param RetLength data length received.
// * @return SUCCESS or ERROR.
// */
//static BG96_StatusTypeDef BG96_ReceiveData(uint8_t* Buffer, uint32_t Length,
//		uint32_t* RetLength) {
//	BG96_StatusTypeDef Ret;
//
//	/* Receive the data from the host */
//	Ret = BG96_getData(Buffer, Length, RetLength);
//
//	return Ret;
//}


/**
 * @brief  MEF to send and recieve AT command
 * @param  cmd the buffer to fill will the received data.
 * @param  Length the maximum data size to receive.
 * @param  Token the expected output if command runs successfully
 * @retval Returns BG96_OK on success and BG96_ERROR otherwise.
 */
static BG96_StatusTypeDef BG96_atCommand(uint8_t* cmd, uint32_t Length, const uint8_t* Token, uint32_t timeout) {
	static uint8_t internalState = BG96_State0;
	BG96_StatusTypeDef result;

	// State machine.
	switch (internalState) {
	case BG96_State0:

		result = BG96_executeAtCmd(cmd, Length);

		osDelay(250 / portTICK_PERIOD_MS);
		// To the next state.
		if(result == BG96_OK)
			internalState = BG96_State1;
	case BG96_State1:
		result = BG96_responseAtCmd(Token, timeout);
		if(result == BG96_OK)
			internalState = BG96_State0;

	}
	return result;
}


/**
 * @brief  Run the AT command
 * @param  cmd the buffer to fill will the received data.
 * @param  Length the maximum data size to receive.
 * @retval Returns BG96_OK on success and BG96_ERROR otherwise.
 */
static BG96_StatusTypeDef BG96_executeAtCmd(uint8_t* cmd, uint32_t Length) {
	taskENTER_CRITICAL();
	/* Send the command */
	if (HAL_UART_F_Send(&huart2, (char*)cmd, Length) < 0) {
		return BG96_ERROR;
	}
	taskEXIT_CRITICAL();
	return BG96_OK;
}




/**
 * @brief  Wait response to the AT command
 * @param  Token the expected output if command runs successfully
 * @retval Returns BG96_OK on success and BG96_ERROR otherwise.
 */

static BG96_StatusTypeDef BG96_responseAtCmd(const uint8_t* Token, uint32_t timeout) {
	static uint32_t idx;
	uint8_t RxChar;
	static BG96_StatusTypeDef status = BG96_OK;

	/* Reset the Rx buffer to make sure no previous data exist */

	if(status != BG96_BUSY){
		memset(RxBuffer, '\0', BUFFERSIZE_RESPONSE);
		idx = 0;
	}

	uint32_t tickLapsed = 0;
	/* Wait for reception */
	do {
	//while(1){
		/* Wait to recieve data */
		if (BG96_receiveInternal(&RxChar, 1) != 0) {
			RxBuffer[idx++] = RxChar;
		}else {
			status = BG96_BUSY;
			break;
		}

		/* Check that max buffer size has not been reached */
		if (idx == BUFFERSIZE_RESPONSE) {
			status = BG96_ERROR;
			break;
		}

		/* Extract the Token */
		if (strstr((char *) RxBuffer, (char *) Token) != NULL) {
			status = BG96_OK;
			break;
		}

		/* Check if the message contains error code */
		if (strstr((char *) RxBuffer, AT_ERROR_STRING) != NULL) {
			status = BG96_ERROR;
			break;
		}
		osDelay(1);
		//tickLapsed = xTaskGetTickCount();
		tickLapsed++;
	} while(tickLapsed < timeout);

	if(tickLapsed >= timeout)
		status = BG96_BUSY;


	return status;
}






/**
 * @brief  Extract info to the circular bufer
 * @param  Buffer The buffer where to fill the received data
 * @param  Length the maximum data size to receive.
 * @retval Returns BG96_OK on success and BG96_ERROR otherwise.
 */
static int32_t BG96_receiveInternal(uint8_t *Buffer, uint32_t Length) {
	uint32_t ReadData = 0;
	/* Loop until data received */
	while (Length--) {

			if (WiFiRxBuffer.head != WiFiRxBuffer.tail) {
				/* serial data available, so return data to user */
				*Buffer++ = WiFiRxBuffer.data[WiFiRxBuffer.head++];

				ReadData++;

				/* check for ring buffer wrap */
				if (WiFiRxBuffer.head >= BUFFERSIZE_CIRCULAR) {
					/* Ring buffer wrap, so reset head pointer to start of buffer */
					WiFiRxBuffer.head = 0;
				}
			}
	}

	return ReadData;
}

///**
// * @brief  Receive data from the WiFi module
// * @param  Buffer The buffer where to fill the received data
// * @param  Length the maximum data size to receive.
// * @param  RetLength Length of received data
// * @retval Returns BG96_OK on success and BG96_ERROR otherwise.
// */
//static BG96_StatusTypeDef BG96_getData(uint8_t* Buffer, uint32_t Length, uint32_t* RetLength) {
//	uint8_t RxChar;
//	uint32_t idx = 0;
//	uint8_t LengthString[4];
//	uint32_t LengthValue;
//	uint8_t i = 0;
//	BG96_Boolean newChunk = BG96_FALSE;
//
//	/* Reset the reception data length */
//	*RetLength = 0;
//
//	/* Reset the reception buffer */
//	memset(RxBuffer, '\0', BUFFERSIZE_RESPONSE);
//
//	/* When reading data over a wifi connection the esp8266
//	 splits it into chunks of 1460 bytes maximum each, each chunk is preceded
//	 by the string "+IPD,<chunk_size>:". Thus to get the actual data we need to:
//	 - Receive data until getting the "+IPD," token, a new chunk is marked.
//	 - Extract the 'chunk_size' then read the next 'chunk_size' bytes as actual data
//	 - Mark end of the chunk.
//	 - Repeat steps above until no more data is available. */
//	uint32_t currentTime = 0;
//	do{
//	//while(1){
//		if (BG96_receiveInternal(&RxChar, 1) != 0) {
//			/* The data chunk starts with +IPD,<chunk length>: */
//			if (newChunk == BG96_TRUE) {
//				/* Read the next lendthValue bytes as part from the actual data. */
//				if (LengthValue--) {
//					*Buffer++ = RxChar;
//					(*RetLength)++;
//				} else {
//					/* Clear the buffer as the new chunk has ended. */
//					newChunk = BG96_FALSE;
//					memset(RxBuffer, '\0', BUFFERSIZE_RESPONSE);
//					idx = 0;
//				}
//			}
//			RxBuffer[idx++] = RxChar;
//		} else {
//			/* Errors while reading return an error. */
//			if ((newChunk == BG96_TRUE) && (LengthValue != 0)) {
//				return BG96_ERROR;
//			} else {
//				break;
//			}
//		}
//
//		if (idx == BUFFERSIZE_RESPONSE) {
//			/* In case of Buffer overflow, return error */
//			if ((newChunk == BG96_TRUE) && (LengthValue != 0)) {
//				return BG96_ERROR;
//			} else {
//				break;
//			}
//		}
//
//		/* When a new chunk is met, extact its size */
//		if ((strstr((char *) RxBuffer, AT_IPD_STRING) != NULL)
//				&& (newChunk == BG96_FALSE)) {
//			i = 0;
//			memset(LengthString, '\0', 4);
//			do {
//				BG96_receiveInternal(&RxChar, 1);
//				LengthString[i++] = RxChar;
//			} while (RxChar != ':');
//
//			/* Get the buffer length */
//			LengthValue = atoi((char *) LengthString);
//
//			newChunk = BG96_TRUE;
//		}
//
//		/* Check if message contains error code */
//		if (strstr((char *) RxBuffer, AT_ERROR_STRING) != NULL) {
//			return BG96_ERROR;
//		}
//
//		/* Check for the chunk end */
//		if (strstr((char *) RxBuffer, AT_IPD_OK_STRING) != NULL) {
//			newChunk = BG96_FALSE;
//		}
//		currentTime++;
//		osDelay(1);
//	}while(currentTime < CMD_TIMEOUT_15000);
//
//	if(currentTime > CMD_TIMEOUT_15000)
//		return BG96_TIMEOUT;
//
//	return BG96_OK;
//}
