/*
 * uart.c
 *
 *  Created on: Jul 20, 2020
 *      Author: fedepacher
 */

#include <general_defs.h>
#include "uart.h"
#include "stdio.h"



/* Private valirable ---------------------------------------------------------*/
extern QueueHandle_t xQueuePrintConsole;
extern QueueHandle_t xSemaphoreControl;
extern QueueHandle_t xQeuePubData;
extern QueueHandle_t xQeueSubData;


data_print_console_t data;
static uint8_t indice = 0;
data_publish_t data_publish_rx;
data_publish_t data_publish_tx;

data_publish_t data_subscribe_rx;
data_publish_t data_subscribe_tx;

uint8_t start_store_pub = 0;
uint8_t recep_ast_pub = 0;			//get asterisk
uint8_t count_crc_pub = 0;			//get crc

uint8_t start_store_sub = 0;
uint8_t recep_ast_sub = 0;			//get asterisk
uint8_t count_crc_sub = 0;			//get crc
/* Private function prototypes -----------------------------------------------*/
//static void WIFI_Handler(void);

/**
 * @brief  Rx Callback when new data is received on the UART.
 * @param  UartHandle: Uart handle receiving the data.
 * @retval None.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	// Set transmission flag: transfer complete
	static BaseType_t xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;

	if (huart->Instance == USART2 || huart->Instance == USART6) {
		uint8_t dato = WiFiRxBuffer.data[WiFiRxBuffer.tail];

		//printf("%c", dato);

		if (++WiFiRxBuffer.tail >= BUFFERSIZE_CIRCULAR) {
			WiFiRxBuffer.tail = 0;
		}
		// Receive one byte in interrupt mode
		HAL_UART_Receive_IT(huart, (uint8_t*) &WiFiRxBuffer.data[WiFiRxBuffer.tail], 1);


		if(start_store_sub == 1){
			if(data_subscribe_rx.length < sizeof(data_subscribe_rx.data)){
				data_subscribe_rx.data[data_subscribe_rx.length++] = dato;
			}
		}
		//the { char is because thingsboard accept json format
		if(dato == '{' || dato == '$'){
			start_store_sub = 1;
			data_subscribe_rx.length = 0;
			if(data_subscribe_rx.length < sizeof(data_subscribe_rx.data)){
				data_subscribe_rx.data[data_subscribe_rx.length++] = dato;
			}
		}
		if(dato == '*'){
			recep_ast_sub = 1;
		}
		if(recep_ast_sub == 1){
			count_crc_sub++;
		}
		//the { char is because thingsboard accept json format
		if(dato == '}' || count_crc_sub == 3){	//receive the end of tx so load a buffer and transmit
			start_store_sub = 0;
			recep_ast_sub = 0;
			count_crc_sub = 0;
			data_subscribe_rx.data[data_subscribe_rx.length++] = dato;
			if(xQeueSubData != NULL && data_subscribe_rx.length < sizeof(data_subscribe_rx.data)){
				strncpy((char*)data_subscribe_tx.data, (char*)data_subscribe_rx.data, data_subscribe_rx.length - 1);
				data_subscribe_tx.length = data_subscribe_rx.length - 1;
				xQueueSendFromISR(xQeueSubData, &data_subscribe_tx, &xHigherPriorityTaskWoken);
				memset((char*)data_subscribe_rx.data, '\0', sizeof(data_subscribe_rx.data));
				data_subscribe_rx.length = 0;
			}
		}

#if DEBUG
	#if WRITE_CHAR
		xQueueSendFromISR(xQueuePrintConsole, &dato, &xHigherPriorityTaskWoken);
	#else
		if(indice < BUFFERSIZE_CMD){		//MENOS 1 PORQUE QUIERO ARGREGARLE AL FINAL EL \0
			data.data_cmd[indice++] = dato;
		}
		else{
			indice = 0;
		}
		if(dato == '\n'){// || indice >= BUFFERSIZE_CMD - 1){
			data.data_cmd[indice] = '\0';
			xQueueSendFromISR(xQueuePrintConsole, &data, &xHigherPriorityTaskWoken);
			memset((char*)data.data_cmd, '\0', sizeof(data.data_cmd));
			indice = 0;
		}

	#endif
#endif
	}
	else{
			if(huart->Instance == USART1){
				uint8_t dato = ControlBuffer.data[ControlBuffer.tail];

				if (++ControlBuffer.tail >= BUFFERSIZE_CIRCULAR) {
					ControlBuffer.tail = 0;
				}
				// Receive one byte in interrupt mode
				HAL_UART_Receive_IT(huart, (uint8_t*) &ControlBuffer.data[ControlBuffer.tail], 1);
				if(start_store_pub == 1){
					if(data_publish_rx.length < sizeof(data_publish_rx.data)){
						data_publish_rx.data[data_publish_rx.length++] = dato;
					}
				}
				//the { char is because thingsboard accept json format
				if(dato == '{' || dato == '$'){
					start_store_pub = 1;
					data_publish_rx.length = 0;
					if(data_publish_rx.length < sizeof(data_publish_rx.data)){
							data_publish_rx.data[data_publish_rx.length++] = dato;
					}
				}
				if(dato == '*'){
					recep_ast_pub = 1;
				}
				if(recep_ast_pub == 1){
					count_crc_pub++;
				}
				//the { char is because thingsboard accept json format
				if(dato == '}' || count_crc_pub == 3){	//receive the end of tx so load a buffer and transmit
					start_store_pub = 0;
					recep_ast_pub = 0;
					count_crc_pub = 0;
					data_publish_rx.data[data_publish_rx.length++] = dato;
					if(xQeuePubData != NULL && data_publish_rx.length < sizeof(data_publish_rx.data)){
						strncpy((char*)data_publish_tx.data, (char*)data_publish_rx.data, data_publish_rx.length - 1);
						data_publish_tx.length = data_publish_rx.length - 1;
						xQueueSendFromISR(xQeuePubData, &data_publish_tx, &xHigherPriorityTaskWoken);
						memset((char*)data_publish_rx.data, '\0', sizeof(data_publish_rx.data));
						data_publish_rx.length = 0;
					}
				}
				if(xSemaphoreControl != NULL){//WAKEUP SUBSCRIBE TASK
					if(dato == '\n' && start_store_pub == 0)
						xSemaphoreGiveFromISR(xSemaphoreControl, &xHigherPriorityTaskWoken);
				}
			}
		}
	/* If xHigherPriorityTaskWoken was set to true you
	    we should yield.  The actual macro used here is
	    port specific. */
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );

}




/**
 * @brief  Function called when error happens on the UART.
 * @param  UartHandle: Uart handle receiving the data.
 * @retval None.
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
	//WIFI_Handler();
}

void HAL_UART_F_Init(UART_HandleTypeDef *huart) {

		start_store_pub = 0;
		recep_ast_pub = 0;
		count_crc_pub = 0;
		start_store_sub = 0;
		recep_ast_sub = 0;
		count_crc_sub = 0;

		WiFiRxBuffer.head = 0;
		WiFiRxBuffer.tail = 0;

//		HAL_UART_Receive_IT(&huart6,
//			(uint8_t*) &WiFiRxBuffer.data[WiFiRxBuffer.tail], 1);
		HAL_UART_Receive_IT(huart,
					(uint8_t*) &WiFiRxBuffer.data[WiFiRxBuffer.tail], 1);


}

void HAL_UART1_F_Init(UART_HandleTypeDef *huart) {

	ControlBuffer.head = 0;
	ControlBuffer.tail = 0;

//		HAL_UART_Receive_IT(&huart6,
//			(uint8_t*) &WiFiRxBuffer.data[WiFiRxBuffer.tail], 1);
		HAL_UART_Receive_IT(huart,
					(uint8_t*) &ControlBuffer.data[ControlBuffer.tail], 1);


}


void HAL_UART_F_DeInit(void) {
	/* Reset USART configuration to default */
//	HAL_UART_DeInit(&huart1);
//	HAL_UART_DeInit(&huart2);
//	HAL_UART_DeInit(&huart6);
}

int8_t HAL_UART_F_Send(UART_HandleTypeDef *huart, const char* Buffer, const uint8_t Length) {
	/* It is using a blocking call to ensure that the AT commands were correctly sent. */

	if (HAL_UART_Transmit_IT(huart, (uint8_t*) Buffer, Length) != HAL_OK){
		return -1;
	}
//	if (HAL_UART_Transmit_IT(&huart2, (uint8_t*) Buffer, Length) != HAL_OK){
//		return -1;
//	}

	return 0;
}


/**
 * @brief  Handler to deinialize the ESP8266 UART interface in case of errors.
 * @param  None
 * @retval None.
 */
/*static void WIFI_Handler(void) {
	HAL_UART_DeInit(&huart2);

	while (1) {
	}
}*/
