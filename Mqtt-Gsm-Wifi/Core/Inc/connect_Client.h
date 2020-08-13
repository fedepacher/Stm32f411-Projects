/*
 * ESP_Client.h
 *
 *  Created on: May 26, 2020
 *      Author: fedepacher
 */

#ifndef INC_CONNECT_CLIENT_H_
#define INC_CONNECT_CLIENT_H_

// Includes.
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include "cmsis_os.h"


#define END_OF_ARRAY			"EOA"

// Strings definitions.
#define AT_OK_STRING            "OK\r\n"
#define AT_IPD_OK_STRING        "OK\r\n\r\n"
#define AT_SEND_OK_STRING       "SEND OK\r\n"
#define AT_SEND_PROMPT_STRING   ">"
#define AT_ERROR_STRING         "ERROR\r\n"
#define AT_IPD_STRING           "+IPD,"


// Buffer settings.
#define BUFFERSIZE_RESPONSE 	1500UL
#define BUFFERSIZE_CMD 			128UL

// Timing settings.
//#define TIMEOUT_MS_RESTART       2000UL///< Module restart timeout.
//#define DEFAULT_TIME_OUT         5000UL /* in ms */
//#define LONG_TIME_OUT         	20000UL /* in ms */

#define KEEPALIVE_MAX			3600

//CMD maximun timeout
#define CMD_TIMEOUT_300			300U
#define CMD_TIMEOUT_5000		5000U
#define CMD_TIMEOUT_15000		15000U
#define CMD_TIMEOUT_75000		75000U
#define CMD_TIMEOUT_140000		140000U
#define CMD_TIMEOUT_150000		150000U
#define CMD_TIMEOUT_180000		180000U


typedef enum
{
  ESP8266_OK                            = 0,
  ESP8266_ERROR                         = 1,
  ESP8266_BUSY                          = 2,
  ESP8266_ALREADY_CONNECTED             = 3,
  ESP8266_CONNECTION_CLOSED             = 4,
  ESP8266_TIMEOUT                       = 5,
  ESP8266_IO_ERROR                      = 6,
  ESP8266_EOA		                    = 7,
} ESP8266_StatusTypeDef;

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  ESP8266_FALSE         = 0,
  ESP8266_TRUE          = 1
} ESP8266_Boolean;


/* Private typedef -----------------------------------------------------------*/


/* Public variables ---------------------------------------------------------*/


/*
 * @brief Creates non-blocking delay.
 * @param delay_ms Delay time in ms.
 * @return SUCCESS, BUSY.
 */
ESP8266_StatusTypeDef Delay(const uint16_t delay_ms);

/*
 * @brief TCP Connect.
 * @param cmd:		Commando to send to gsm module.
 * @param length	Command's Length.
 * @param timeout	Command timeout
 * @return SUCCESS, BUSY or ERROR.
 */
ESP8266_StatusTypeDef ConnectTCP(const uint8_t * cmd, uint32_t length, uint32_t timeout);



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
		const uint16_t keepalive, const uint8_t sslenable);

/*
 * @brief Connect to MQTT broker.
 * @param clientId The client identifier string.
 * @param userName User name of the client. It can be used for authentication.
 * @param password Password corresponding to the user name of the client. It can be used for authentication.
 * @param tcpconnectID MQTT socket identifier. The range is 0-5.
 * @return SUCCESS, BUSY or ERROR.
 */
ESP8266_StatusTypeDef MQTTConnect(const uint8_t * clientId, const uint8_t * userName, const uint8_t * password, const uint8_t tcpconnectID);

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
ESP8266_StatusTypeDef PubData(uint8_t tcpconnectID, uint32_t msgID, uint8_t qos, uint8_t retain, uint8_t* topic,
uint8_t* dataBuffer, uint32_t Length);


/*
 * @brief Receive data over the wifi connection.
 * @param  Buffer: the buffer to receive
 * @param  Length: the Buffer's data size.
 * @param  RetLength: the Buffer's data length received.
 * @retval Returns ESP8266_OK on success and ESP8266_ERROR otherwise.
 */
ESP8266_StatusTypeDef ReceiveData(uint8_t* Buffer, uint32_t Length,
		uint32_t* RetLength);

/*
 * @brief Reset Module ESP.
 * @return SUCCESS, BUSY or ERROR.
 */
ESP8266_StatusTypeDef Reset();

#endif /* INC_CONNECT_CLIENT_H_ */