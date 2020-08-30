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


typedef enum
{
	BG96_OK                            = 0,
	BG96_ERROR                         = 1,
	BG96_BUSY                          = 2,
	BG96_ALREADY_CONNECTED             = 3,
	BG96_CONNECTION_CLOSED             = 4,
	BG96_TIMEOUT                       = 5,
	BG96_IO_ERROR                      = 6,
	BG96_EOA		                    = 7,
} BG96_StatusTypeDef;

/* Exported types ------------------------------------------------------------*/
typedef enum
{
	BG96_FALSE         = 0,
	BG96_TRUE          = 1
} BG96_Boolean;
/* Public variables ---------------------------------------------------------*/


/*
 * @brief Creates non-blocking delay.
 * @param delay_ms Delay time in ms.
 * @return SUCCESS, BUSY.
 */
//BG96_StatusTypeDef Delay(const uint16_t delay_ms);




/*
 * @brief TCP Connect.
 * @param cmd:		Commando to send to gsm module.
 * @param length	Command's Length.
 * @param Token		Response expencted.
 * @param timeout	Command timeout.
 * @return SUCCESS, BUSY or ERROR.
 */
BG96_StatusTypeDef BG96_ConnectTCP(const uint8_t * cmd, uint32_t length, const uint8_t* Token, uint32_t timeout) ;



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
		const uint16_t keepalive, const uint8_t sslenable);

/*
 * @brief Connect to MQTT broker.
 * @param clientId The client identifier string.
 * @param userName User name of the client. It can be used for authentication.
 * @param password Password corresponding to the user name of the client. It can be used for authentication.
 * @param tcpconnectID MQTT socket identifier. The range is 0-5.
 * @return SUCCESS, BUSY or ERROR.
 */
BG96_StatusTypeDef BG96_MQTTConnect(const uint8_t * clientId, const uint8_t * userName, const uint8_t * password, const uint8_t tcpconnectID);

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
BG96_StatusTypeDef BG96_PublishTopic(uint8_t tcpconnectID, uint32_t msgID, uint8_t qos, uint8_t retain, uint8_t* topic,
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
 * @retval Returns BG96_OK on success and BG96_ERROR otherwise.
 */
BG96_StatusTypeDef BG96_SubscribeTopic(uint8_t tcpconnectID, uint32_t msgID, uint8_t* topic, uint8_t qos);

/**
 * @brief  Unsubscribe topic over GSM connection.
 * @param  tcpconnectID:MQTT socket identifier. The range is 0-5.
 * @param  msgID: 		Message identifier of packet. The range is 0-65535. It will be 0 only when <qos>=0.
 * @param  topic:		Topic that needs to be published
 * @retval Returns BG96_OK on success and BG96_ERROR otherwise.
 */
BG96_StatusTypeDef BG96_UnsubscribeTopic(uint8_t tcpconnectID, uint32_t msgID, uint8_t* topic);

/**
 * @brief  Disconect a client from MQTT Server.
 * @param  tcpconnectID:MQTT identifier. The range is 0-5.
 * @retval Returns BG96_OK on success and BG96_ERROR otherwise.
 */
BG96_StatusTypeDef BG96_Disconnect(uint8_t tcpconnectID);

/**
 * @brief  Close network for MQTT Client.
 * @param  tcpconnectID:MQTT identifier. The range is 0-5.
 * @retval Returns BG96_OK on success and BG96_ERROR otherwise.
 */
BG96_StatusTypeDef BG96_Close(uint8_t tcpconnectID);

/*
 * @brief Reset Module ESP.
 * @return SUCCESS, BUSY or ERROR.
 */
BG96_StatusTypeDef BG96_Reset();

#endif /* INC_CONNECT_CLIENTBG96_H_ */
