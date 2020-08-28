/*
 * ESP_Client.h
 *
 *  Created on: May 26, 2020
 *      Author: fedepacher
 */

#ifndef INC_CONNECT_CLIENTBG96_H_
#define INC_CONNECT_CLIENTBG96_H_

// Includes.
#include <general_defs.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include "cmsis_os.h"


/* Private typedef -----------------------------------------------------------*/


/* Public variables ---------------------------------------------------------*/




/*
 * @brief TCP Connect.
 * @param cmd:		Commando to send to gsm module.
 * @param length	Command's Length.
 * @param Token		Response expencted.
 * @param timeout	Command timeout.
 * @return SUCCESS, BUSY or ERROR.
 */
ESP8266_StatusTypeDef ConnectTCP(const uint8_t * cmd, uint32_t length, const uint8_t* Token, uint32_t timeout) ;



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
ESP8266_StatusTypeDef SubData(uint8_t tcpconnectID, uint32_t msgID, uint8_t* topic, uint8_t qos);


/*
 * @brief Reset Module ESP.
 * @return SUCCESS, BUSY or ERROR.
 */
ESP8266_StatusTypeDef Reset();

#endif /* INC_CONNECT_CLIENTBG96_H_ */
