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
extern UART_HandleTypeDef huart1;	//--
extern UART_HandleTypeDef huart2;	//connected to bg96
extern UART_HandleTypeDef huart6;	//connected to esp8266
extern QueueHandle_t xQueuePrintConsole;
data_print_console_t data;
static uint8_t indice = 0;
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
		/*if(xSemaphoreSub != NULL){
			if(dato == '\n')
				xSemaphoreGiveFromISR(xSemaphoreSub, &xHigherPriorityTaskWoken);
		}*/
#if DEBUG
	#if WRITE_CHAR
		xQueueSendFromISR(xQueuePrintConsole, &dato, &xHigherPriorityTaskWoken);
	#else
		if(indice < DATA_LENGTH)
			data.data_cmd[indice++] = dato;
		else
			indice = 0;
		if(dato == '\n'){
			data.data_cmd[indice] = '\0';
			xQueueSendFromISR(xQueuePrintConsole, &data, &xHigherPriorityTaskWoken);
			memset((char*)data.data_cmd, '\0', indice);
			indice = 0;
		}

	#endif
#endif
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

void HAL_UART_F_Init() {

		WiFiRxBuffer.head = 0;
		WiFiRxBuffer.tail = 0;

#if ACTIVATE_WIFI
		HAL_UART_Receive_IT(&huart6,
			(uint8_t*) &WiFiRxBuffer.data[WiFiRxBuffer.tail], 1);
#else
		HAL_UART_Receive_IT(&huart2,
					(uint8_t*) &WiFiRxBuffer.data[WiFiRxBuffer.tail], 1);
#endif

}



void HAL_UART_F_DeInit(void) {
	/* Reset USART configuration to default */
	HAL_UART_DeInit(&huart1);
	HAL_UART_DeInit(&huart2);
	HAL_UART_DeInit(&huart6);
}

int8_t HAL_UART_F_Send(const char* Buffer, const uint8_t Length) {
	/* It is using a blocking call to ensure that the AT commands were correctly sent. */

#if ACTIVATE_WIFI
	if (HAL_UART_Transmit_IT(&huart6, (uint8_t*) Buffer, Length) != HAL_OK){
		return -1;
	}
#else
	if (HAL_UART_Transmit_IT(&huart2, (uint8_t*) Buffer, Length) != HAL_OK){
		return -1;
	}
#endif
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
