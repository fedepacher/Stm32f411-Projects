/*
 * mytasks.h
 *
 *  Created on: Jul 20, 2020
 *      Author: fedepacher
 */

#ifndef INC_MYTASKS_H_
#define INC_MYTASKS_H_

#include "uart.h"

void initTasks();
void printConsoleTask(void *argument);
void connectTaskBG96(void *argument);
void buttonsTask(void *argument);
void timerHandlerTask(void *argument);

#endif /* INC_MYTASKS_H_ */
