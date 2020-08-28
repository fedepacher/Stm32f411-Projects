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
#include "data_manage.h"




#define VERSION					"version"
#define PDPCID					"pdpcid"
#define WILL					"will"
#define TIMEOUT					"timeout"
#define SESSION					"session"
#define KEEPALIVE				"keepalive"
#define SSL						"ssl"


//static unsigned long int (* getTime_ms)(void);///< Used to hold handler for time provider.
//static unsigned long int t0;///< Keeps entry time for timeout detection.


static uint8_t cmdBuffer[BUFFERSIZE_CMD];


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
ESP8266_StatusTypeDef ConnectTCP(const uint8_t * cmd, uint32_t length, const uint8_t* Token, uint32_t timeout) {
	ESP8266_StatusTypeDef result;

	if(strcmp((char*)cmd, (char*)END_OF_ARRAY) != 0) {
		// To the next state.
		result = atCommand((uint8_t*)cmd, length, Token, timeout);
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
	static uint8_t internalState = State0;
	ESP8266_StatusTypeDef result;


	// State machine.
	switch (internalState) {
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
		if(ESP8266_OK == (result = atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) QMTOPEN_STRING, CMD_TIMEOUT_75000))){
			// To the next state.
			internalState = State9;
		}else{
			// Exit on ERROR or INPROGRESS.
			return result;
		}
	case State9:
		sprintf((char *)cmdBuffer, "AT+QMTOPEN?%c%c%c", '\r', '\n', '\0');
		if(ESP8266_OK == (result = atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_75000))){
			internalState = State0;
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
	static uint8_t internalState = State0;
	ESP8266_StatusTypeDef result;

	switch (internalState) {
		case State0:

			sprintf((char *)cmdBuffer, "AT+QMTCONN=%u,\"%s\",\"%s\",\"%s\"%c%c%c",tcpconnectID, clientId, userName, password,'\r', '\n', '\0');

			internalState = State1;

		case State1:
			if(ESP8266_OK == (result = atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) QMTCONN_STRING, CMD_TIMEOUT_15000))){
				internalState = State0;
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

/**
 * @brief  Subscribe topic over GSM connection.
 * @param  tcpconnectID:MQTT socket identifier. The range is 0-5.
 * @param  msgID: 		Message identifier of packet. The range is 0-65535. It will be 0 only when <qos>=0.
 * @param  topic:		Topic that needs to be published
 * @param  qos:			The QoS level at which the client wants to publish the messages.
 * 							0 At most once
 * 							1 At least once
 * 							2 Exactly once
 * @retval Returns ESP8266_OK on success and ESP8266_ERROR otherwise.
 */
ESP8266_StatusTypeDef SubData(uint8_t tcpconnectID, uint32_t msgID, uint8_t* topic, uint8_t qos) {
	static uint8_t internalState = State0;
		ESP8266_StatusTypeDef result;

		switch (internalState) {
			case State0:

				sprintf((char *) cmdBuffer, "AT+QMTSUB=%u,%lu,\"%s\",%u%c%c", tcpconnectID, (unsigned long)msgID, topic, qos, '\r', '\n');
				internalState = State1;

			case State1:
				if(ESP8266_OK == (result = atCommand((uint8_t*)cmdBuffer, strlen((char*)cmdBuffer), (uint8_t*) QMTSUB_STRING, CMD_TIMEOUT_5000))){
					internalState = State0;
					return result;
				}
		}
		return result;
}

