/*
 * ESP_Client.c
 *
 *  Created on: May 25, 2020
 *      Author: fedepacher
 */

#include "stdio.h"
#include <string.h>
#include <assert.h>
#include "connect_ClientESP.h"
#include "uart.h"
#include "stdbool.h"
#include "cmsis_os.h"
#include "data_manage.h"



static uint8_t ESP82_cmdBuffer[ESP_BUFFERSIZE_CMD];
//static TickType_t t0;
static const char * ESP_SSLSIZE_str = "AT+CIPSSLSIZE=4096\r\n";///< ESP8266 module memory (2048 to 4096) reserved for SSL.

/*
 * @brief INTERNAL Timeout setup.
 */
//static void timeoutBegin(void){
//	// Get entry time.
//	t0 = xTaskGetTickCount();
//}


/*
 * @brief INTERNAL Timeout checker.
 * @param interval_ms Interval time in ms.
 * @return True if timeout expired.
 */
//static bool timeoutIsExpired(const uint16_t interval_ms) {
//	// Check if the given interval is in the past.
//	return (interval_ms < (xTaskGetTickCount() - t0));
//
//}





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
 * @brief Connect to AP.
 * @param resetToDefault If true, reset the module to default settings before connecting.
 * @param ssid AP name.
 * @param pass AP password.
 * @return SUCCESS, BUSY or ERROR.
 */
ESP8266_StatusTypeDef ESP_ConnectWifi(const bool resetToDefault, const char * ssid, const char * pass) {
	static uint8_t internalState = State0;
	ESP8266_StatusTypeDef result;

	// State machine.
	switch (internalState) {
	case State0:
		//if(!resetToDefault || (ESP8266_OK == (result = atCommand((uint8_t*)"AT+RST\r\n", 8, (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_3000)))) {
			// Wait for startup phase to finish.
			osDelay(ESP_TIMEOUT_MS_RESTART/portTICK_PERIOD_MS);
			// To the next state.
			internalState = State1;
		//}
	//nobreak;
	case State1:
		// AT+RESTORE (if requested).
		if(!resetToDefault || (ESP8266_OK == (result = atCommand((uint8_t*)"AT\r\n", 4, (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_3000)))) {
			// To the next state.
			internalState = State2;
		}
		//nobreak;
	case State2:
		// If resetted, wait for restart to finish.
		if(!resetToDefault){
			osDelay(ESP_TIMEOUT_MS_RESTART/portTICK_PERIOD_MS);
			// To the next state.
			internalState = State3;
		}
		//nobreak;
	case State3:
		// AT+CWMODE (client mode)
		if((ESP8266_OK == (result = atCommand((uint8_t*)"AT+CWMODE=1\r\n", 13, (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_3000))) && (ssid != NULL)){
			// To the next state.
			internalState = State4;
		}
		// nobreak;
	case State4:
			// AT+CWMODE (client mode)
			if((ESP8266_OK == (result = atCommand((uint8_t*)"AT+CIPMUX=0\r\n", 13, (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_3000))) && (ssid != NULL)){
				// To the next state.
				internalState = State5;
			}
			// nobreak;
	case State5:
		// Size check.
		if ((strlen(ssid) + strlen(pass)) > (ESP_BUFFERSIZE_CMD - 17)) {
			return false;
		}

		// AT+CWJAP prepare.
		sprintf((char *)ESP82_cmdBuffer, "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, pass);

		// To the next state.
		internalState = State6;

		//nobreak;
	case State6:
		// AT+CWJAP
		if(ESP8266_OK == (result = atCommand(ESP82_cmdBuffer, strlen((char*)ESP82_cmdBuffer), (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_5000))){
			internalState = State0;
		}

		//nobreak;
	//default:
		// To the first state.
		//internalState = State0;
	}
	return result;
}

/*
 * @brief Connection test.
 * @return SUCCESS, INPROGRESS or ERROR.
 */
ESP8266_StatusTypeDef ESP_IsConnectedWifi(void) {
	return atCommand((uint8_t*)"AT+CIPSTATUS\r\n", 14, (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_3000);
}



/*
 * @brief Connect to server via TCP.
 * @param host Hostname or IP address.
 * @param port Remote port.
 * @param keepalive Keep-alive time between 0 to 7200 seconds.
 * @param ssl Starts SSL connection.
 * @return SUCCESS, BUSY or ERROR.
 */
ESP8266_StatusTypeDef ESP_StartTCP(const uint8_t * host, const uint16_t port, const uint16_t keepalive, const uint8_t ssl) {
	static uint8_t internalState = State0;
	ESP8266_StatusTypeDef result;

	// State machine.
	switch (internalState) {
	case State0:
		// Size check.
		if(strlen((char*)host) > (ESP_BUFFERSIZE_CMD - 34)){
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
		internalState = State1;

		//nobreak;
	case State1:
		// AT+CIPSSLSIZE (or skip)
		if(!ssl || (ESP8266_OK == (result = atCommand((uint8_t*)ESP_SSLSIZE_str, 20, (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_3000)))){
			// To the next state.
			internalState = State2;
		}else{
			// Exit on ERROR or INPROGRESS.
			return result;
		}
		//nobreak;
	case State2:
		// AT+CIPSTART
		if(ESP8266_OK == (result = atCommand(ESP82_cmdBuffer, strlen((char*)ESP82_cmdBuffer), (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_5000))){
			internalState = State0;
		}
	}
	return result;
}

/**
 * @brief  Send data over the wifi connection.
 * @param  Buffer: the buffer to send
 * @param  Length: the Buffer's data size.
 * @retval Returns ESP8266_OK on success and ESP8266_ERROR otherwise.
 */
ESP8266_StatusTypeDef ESP_SendData(uint8_t* Buffer, uint32_t Length) {
	ESP8266_StatusTypeDef Ret = ESP8266_OK;
	//static uint8_t internalState = State0;
	if (Buffer != NULL) {
		//uint32_t tickStart;
		//TickType_t tickStart;

		//switch (internalState) {
			//case State0:
				/* Construct the CIPSEND command */
				memset(ESP82_cmdBuffer, '\0', ESP_BUFFERSIZE_CMD);
				sprintf((char *) ESP82_cmdBuffer, "AT+CIPSEND=%lu%c%c", Length  , '\r', '\n');

				/* The CIPSEND command doesn't have a return command
				 until the data is actually sent. Thus we check here whether
				 we got the '>' prompt or not. */
				Ret = atCommand(ESP82_cmdBuffer, strlen((char *) ESP82_cmdBuffer),
						(uint8_t*) AT_SEND_PROMPT_STRING, CMD_TIMEOUT_5000);//AT_SEND_PROMPT_STRING

				/* Return Error */
				if (Ret != ESP8266_OK) {
					//return ESP8266_ERROR;
				}
				//internalState++;
				//break;
			//case State1:
				/* Wait before sending data. */
				osDelay(1000); //not blocking delay
				//internalState++;
				//break;
			//case State2:
				/* Send the data */
				Ret = atCommand(Buffer, Length, (uint8_t*) AT_OK_STRING, CMD_TIMEOUT_5000);//AT_IPD_STRING);//
//				if(Ret == ESP8266_OK){
//					internalState = State0;
//				}
//				break;
//		}
	}

	return ESP8266_OK;
}
