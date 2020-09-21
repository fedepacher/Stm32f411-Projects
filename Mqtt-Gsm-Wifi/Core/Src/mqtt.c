/*
 * mqtt.c
 *
 *  Created on: May 26, 2020
 *      Author: fedepacher
 */
#include "mqtt.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "MQTTPacket.h"


#define TRIAL_CONNECTION_TIME	5

//int32_t transport_socket;

ESP8266_StatusTypeDef mqtt_Connect( uint8_t * clientId,  uint8_t * userName,  uint8_t * password) {
	int32_t length;
	unsigned char buffer[BUFFERSIZE_CMD];
	uint8_t sessionPresent, connack_rc;
	ESP8266_StatusTypeDef Status = ESP8266_OK;
	int32_t internalState = 0;
	int32_t trial = 0;
	MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

	while (trial < TRIAL_CONNECTION_TIME) {
		switch (internalState) {
		case 0:
			connectData.MQTTVersion = 3;//4;
			connectData.clientID.cstring = (char*)clientId;
			connectData.username.cstring = (char*)userName;
			connectData.password.cstring = (char*)password;
			connectData.keepAliveInterval = CONNECTION_KEEPALIVE_S * 2;
			//connectData.cleansession = 1;
			//connectData.willFlag = 1;
			//connectData.will.qos = 2;
			memset((char*) buffer, '\0', BUFFERSIZE_CMD);
			length = MQTTSerialize_connect(buffer, sizeof(buffer),
					&connectData);

			// Send CONNECT to the mqtt broker.

			Status = ESP_SendData(buffer, length);

			if (Status == ESP8266_OK) {

				internalState++;
			} else {
				internalState = 0;
				if(Status == ESP8266_ERROR)
					trial++;
			}
			break;
		case 1:
			memset((char*) buffer, '\0', BUFFERSIZE_CMD);
			uint32_t Retlength;
			Status = ESP_ReceiveData(buffer, BUFFERSIZE_CMD, &Retlength);

			if (Status == ESP8266_OK) {
				if (MQTTDeserialize_connack(&sessionPresent, &connack_rc,
						buffer, strlen((char*) buffer)) != 1) {
					internalState++;
				} else {
					Status = ESP8266_OK;
					trial = TRIAL_CONNECTION_TIME;
					break;
				}
			} else {
				internalState++;
			}
			break;

		case 2:
			memset((char*) buffer, '\0', BUFFERSIZE_CMD);
			length = MQTTSerialize_disconnect(buffer, sizeof(buffer));
			Status = ESP_SendData(buffer, length);
			internalState++;
			break;
		case 3:
			Status = ESP8266_ConnectionClose();
			trial = TRIAL_CONNECTION_TIME;//aca deberia devolver para reconectarse
			break;
		}

	}


	return Status;
}

ESP8266_StatusTypeDef mqtt_Publisher(uint8_t *topic, uint8_t *data,
		uint32_t dataLength) {
	unsigned char buffer[BUFFERSIZE_CMD];
	int32_t length;
	int32_t trial = 0;
	int32_t internalState = 0;
	ESP8266_StatusTypeDef Status = ESP8266_OK;

	// Populate the publish message.
	MQTTString topicString = MQTTString_initializer;
	topicString.cstring = (char*) topic;
	int qos = 0;

	if(MQTTSerialize_pingreq(buffer, BUFFERSIZE_CMD) != 0){
		memset((char*) buffer, '\0', BUFFERSIZE_CMD);
		//strcat((char*)data->data, "\r\n");// OJO QUE PUEDE QUE ALGUNOS ENVIOS NECESITEN ESTE \R\N
		length = MQTTSerialize_publish(buffer, BUFFERSIZE_CMD, 0, qos, 0, 0,
				topicString, data, dataLength);

		// Send PUBLISH to the mqtt broker.
		while (trial < TRIAL_CONNECTION_TIME) {
			switch (internalState) {
			case 0:
				Status = ESP_SendData(buffer, length);

				if (Status == ESP8266_OK) {	//(result = transport_sendPacketBuffer(transport_socket, buffer, length)) == length) {
					internalState++;
				} else {
					internalState = 0;
					trial++;
				}
				break;
			case 1:
				Status = ESP8266_OK;
				trial = TRIAL_CONNECTION_TIME;
				break;
			}
		}
	}
	else{
		Status = ESP8266_ERROR;
	}
	return Status;
}

ESP8266_StatusTypeDef mqtt_Subscriber() {

	int length;
	unsigned char buffer[BUFFERSIZE_CMD];
	ESP8266_StatusTypeDef Status = ESP8266_OK;

	// Populate the subscribe message.
	MQTTString topicFilters[1] = { MQTTString_initializer };
	topicFilters[0].cstring = "test/rgb";
	int requestedQoSs[1] = { 0 };
	length = MQTTSerialize_subscribe(buffer, BUFFERSIZE_CMD, 0, 1, 1,
			topicFilters, requestedQoSs);

	// Send SUBSCRIBE to the mqtt broker.
	Status = ESP_SendData(buffer, length);

	//envio a otro topic
	/*memset(buffer, '\0', sizeof(buffer));
	 topicFilters[0].cstring = "test/rgb1";
	 length = MQTTSerialize_subscribe(buffer, sizeof(buffer), 0, 1, 1,
	 topicFilters, requestedQoSs);
	 Status = ESP8266_SendData(buffer, length);*/

	uint32_t RetLength;
	uint8_t *dato;
	if (Status == ESP8266_OK) {
		while (1) {
			//alocate memory bor the receiving buffer
			dato = (uint8_t*) malloc(128 * sizeof(uint8_t));
			memset(dato, '\0', 128);
			ESP_ReceiveData(dato, 128, &RetLength);
			free(dato);
		}
	} else {

	}
	return Status;
}

ESP8266_StatusTypeDef mqtt_SubscriberPacket(uint8_t *topic, uint8_t topic_length) {
	int length;
	unsigned char buffer[BUFFERSIZE_CMD];
	ESP8266_StatusTypeDef Status = ESP8266_OK;
	int32_t trial = 0;
	int32_t internalState = 0;
	uint16_t subcribe_MsgID;
	int32_t requestQoS, subcribeCount, granted_QoS;

	// Populate the subscribe message.
	MQTTString topicFilters[1] = { MQTTString_initializer };
	//strncpy((char*)topicFilters[0].cstring, (char*)topic, topic_length);
	topicFilters[0].cstring = topic;			//"test/rgb";
	int requestedQoSs[1] = { 0 };
	memset((char*) buffer, '\0', BUFFERSIZE_CMD);
	length = MQTTSerialize_subscribe(buffer, BUFFERSIZE_CMD, 0, 1, 1,
			topicFilters, requestedQoSs);

	// Send SUBSCRIBE to the mqtt broker.
	while (trial < TRIAL_CONNECTION_TIME) {
		switch (internalState) {
		case 0:
			Status = ESP_SendData(buffer, length);

			if (Status == ESP8266_OK) {	//(result = transport_sendPacketBuffer(transport_socket, buffer, length)) == length) {
				internalState++;
			} else {
				internalState = 0;
				trial++;
			}
			break;
		case 1://por ahora lo dejamos sin suback

			memset((char*) buffer, '\0', BUFFERSIZE_CMD);//chequeamos respuesta del broker si se pudo subscribir
			uint32_t Retlength;
			Status = ESP_ReceiveData(buffer, BUFFERSIZE_CMD, &Retlength);

			if (Status == ESP8266_OK) {
				if (MQTTDeserialize_suback(&subcribe_MsgID, 1,
						(int*) &subcribeCount, (int*) &granted_QoS, buffer,
						strlen((char*) buffer)) != 1) {
					internalState++;

				} else {
					Status = ESP8266_OK;
					trial = TRIAL_CONNECTION_TIME;
					break;

				}
			} else
				//si no fue bien desconectamos
				internalState++;
			break;
		case 2:
			memset((char*) buffer, '\0', BUFFERSIZE_CMD);
			length = MQTTSerialize_disconnect(buffer, sizeof(buffer));
			Status = ESP_SendData(buffer, length);
			internalState++;
			break;
		case 3:
			Status = ESP8266_ConnectionClose();
			internalState = 0;
			trial = TRIAL_CONNECTION_TIME;
			break;
		}
	}
	return Status;
}


ESP8266_StatusTypeDef mqtt_SubscriberReceive(uint8_t *data,
		uint32_t data_length, uint32_t *Retlength) {//char topic[], char *pData, uint32_t *length) {
	ESP8266_StatusTypeDef Status = ESP8266_OK;

	memset(data, '\0', data_length);
	ESP_ReceiveData(data, data_length, Retlength);

	return Status;
}

