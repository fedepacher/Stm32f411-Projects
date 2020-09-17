/*
 * mqtt.h
 *
 *  Created on: May 26, 2020
 *      Author: fedepacher
 */

#ifndef INC_MQTT_H_
#define INC_MQTT_H_

#include "general_defs.h"
#include "stdio.h"
#include "ESP_Client.h"


#define CONNECTION_KEEPALIVE_S	60UL
#define MQTT_BUFFERSIZE			128UL

typedef struct{
	char topic[20];
	char data[MQTT_BUFFERSIZE];
	uint32_t length;
}dataMqtt_t;

/*
 * @brief Send connect packet to the broker mqtt.
 * @param clientId The client identifier string.
 * @param userName 		User name of the client. It can be used for authentication.
 * @param password 		Password corresponding to the user name of the client. It can be used for authentication.
 * @return ESP8266_OK if it is ok otherwise ESP8266_ERROR
 */
ESP8266_StatusTypeDef mqtt_Connect(uint8_t * clientId, uint8_t * userName, uint8_t * password);

/*
 * @brief Send publish packet to the broker mqtt.
 * @param topic		topic to publish
 * @param data		data to be sent
 * @param length	data length to be published
 * @return ESP8266_OK if it is ok otherwise ESP8266_ERROR
 */
ESP8266_StatusTypeDef mqtt_Publisher(uint8_t *topic, uint8_t *data, uint32_t length);//char *topic, char *data);

/*
 * @brief Send subcribe packet to the broker mqtt.
 * @return ESP8266_OK if it is ok otherwise ESP8266_ERROR
 */
ESP8266_StatusTypeDef mqtt_Subscriber();

/*
 * @brief Send subcribe packet to the broker mqtt.
 * @param topic topic to be subcribed
 * @param topic_length	topic length
 * @return ESP8266_OK if it is ok otherwise ESP8266_ERROR
 */
ESP8266_StatusTypeDef mqtt_SubscriberPacket(uint8_t *topic, uint8_t topic_length);

/*
 * @brief Receive packets from the broker mqtt.
 * @param data			data received
 * @param data_length	data length buffer
 * @param Retlength		data length received
 * @return ESP8266_OK if it is ok otherwise ESP8266_ERROR
 */
ESP8266_StatusTypeDef mqtt_SubscriberReceive(uint8_t *data, uint32_t data_length, uint32_t* Retlength);//char topic[], char *pData, uint32_t *length);

#endif /* INC_MQTT_H_ */
