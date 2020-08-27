/*
 * mqtt.h
 *
 *  Created on: May 26, 2020
 *      Author: fedepacher
 */

#ifndef INC_MQTT_H_
#define INC_MQTT_H_

#include "connect_ClientESP.h"


#define CONNECTION_KEEPALIVE_S	60UL
#define MQTT_BUFFERSIZE			128UL

typedef struct{
	char topic[20];
	char data[MQTT_BUFFERSIZE];
	uint32_t length;
}dataMqtt_t;

/*
 * @brief Send connect packet to the broker mqtt.
 */
ESP8266_StatusTypeDef mqtt_Connect(void);

/*
 * @brief Send publish packet to the broker mqtt.
 */
ESP8266_StatusTypeDef mqtt_Publisher(dataMqtt_t *data);//char *topic, char *data);

/*
 * @brief Send subcribe packet to the broker mqtt.
 */
ESP8266_StatusTypeDef mqtt_Subscriber();

/*
 * @brief Send subcribe packet to the broker mqtt.
 */
ESP8266_StatusTypeDef mqtt_SubscriberPacket(char *topic);

/*
 * @brief Receive packets from the broker mqtt.
 */
ESP8266_StatusTypeDef mqtt_SubscriberReceive(dataMqtt_t *data);//char topic[], char *pData, uint32_t *length);

#endif /* INC_MQTT_H_ */
