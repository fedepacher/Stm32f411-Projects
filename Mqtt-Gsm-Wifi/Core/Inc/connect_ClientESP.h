/*
 * ESP_Client.h
 *
 *  Created on: May 26, 2020
 *      Author: fedepacher
 */

#ifndef INC_CONNECT_CLIENTESP_H_
#define INC_CONNECT_CLIENTESP_H_

// Includes.
#include <general_defs.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>




// Buffer settings.
#define ESP_BUFFERSIZE_CIRCULAR 	2048UL
#define ESP_BUFFERSIZE_RESPONSE 	1500UL
#define ESP_BUFFERSIZE_CMD 			128UL

// Timing settings.
#define ESP_TIMEOUT_MS_RESTART       2000UL///< Module restart timeout.
#define ESP_DEFAULT_TIME_OUT         3000UL /* in ms */
#define ESP_LONG_TIME_OUT         	20000UL /* in ms */




/*
 * @brief Connect to AP.
 * @param resetToDefault If true, reset the module to default settings before connecting.
 * @param ssid AP name.
 * @param pass AP password.
 * @return SUCCESS, BUSY or ERROR.
 */
ESP8266_StatusTypeDef ESP_ConnectWifi(const bool resetToDefault, const char * ssid, const char * pass);


/*
 * @brief Connection test.
 * @return SUCCESS, BUSY or ERROR.
 */
ESP8266_StatusTypeDef ESP_IsConnectedWifi(void);

/*
 * @brief Connect to server via TCP.
 * @param host Hostname or IP address.
 * @param port Remote port.
 * @param keepalive Keep-alive time between 0 to 7200 seconds.
 * @param ssl Starts SSL connection.
 * @return SUCCESS, BUSY or ERROR.
 */
ESP8266_StatusTypeDef ESP_StartTCP(const uint8_t * host, const uint16_t port, const uint16_t keepalive, const uint8_t ssl);

/**
 * @brief  Send data over the wifi connection.
 * @param  Buffer: the buffer to send
 * @param  Length: the Buffer's data size.
 * @retval Returns ESP8266_OK on success and ESP8266_ERROR otherwise.
 */
ESP8266_StatusTypeDef ESP_SendData(uint8_t* pData, uint32_t Length);


/*
 * @brief Reset Module ESP.
 * @return SUCCESS, BUSY or ERROR.
 */
ESP8266_StatusTypeDef ESP_Reset();

#endif /* INC_CONNECT_CLIENTESP_H_ */
