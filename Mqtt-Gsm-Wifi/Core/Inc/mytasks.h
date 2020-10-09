/*
 * mytasks.h
 *
 *  Created on: Jul 20, 2020
 *      Author: fedepacher
 */

#ifndef INC_MYTASKS_H_
#define INC_MYTASKS_H_

#include "uart.h"

/*
 * @brief Initialization tasks, semaphores and queues.
 * @param argument	task argument
 * @return void
 */
void initTasks();

/*
 * @brief Print message to the console
 * @param argument	task argument
 * @return void
 */
void printConsoleTask(void *argument);

/*
 * @brief Connect to gsm network.
 * @param argument	task argument
 * @return void
 */
void connectGSMTask(void *argument);

/*
 * @brief Connect to wifi network.
 * @param argument	task argument
 * @return void
 */
void connectWifiTask(void *argument);

/*
 * @brief Led status.
 * @param argument	task argument
 * @return void
 */
void ledTask(void *argument);

/*
 * @brief Button task.
 * @param argument	task argument
 * @return void
 */
//void buttonsTask(void *argument);

/*
 * @brief Receive message to the mqtt topic and retransmit it to the ME1040.
 * @param argument	task argument
 * @return void
 */
void subscribeTask(void *argument);

/*
 * @brief Publish message to the mqtt topic.
 * @param argument	task argument
 * @return void
 */
void publishTask(void *argument);

/*
 * @brief Receive command from UART from Microelect device and excecute desire task.
 * @param argument	task argument
 * @return void
 */
void controlTask(void *argument);

/*
 * @brief Search wifi available network.
 * @param argument	task argument
 * @return void
 */
void searchWifiTask(void *argument);

/*
 * @brief Send ping packet to the broker mqtt to keep alive connection.
 * @param argument	task argument
 * @return void
 */
void keepAliveTask(void *argument);


#endif /* INC_MYTASKS_H_ */
