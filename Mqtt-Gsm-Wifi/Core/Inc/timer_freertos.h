/*
 * timer_freertos.h
 *
 *  Created on: Jul 25, 2020
 *      Author: fedepacher
 */

#ifndef INC_TIMER_FREERTOS_H_
#define INC_TIMER_FREERTOS_H_

#include "cmsis_os.h"
#include "timers.h"
#include "stdio.h"

typedef enum{
	  TIMER_OFF                            = 0,
	  TIMER_ON		                      = 1,
}Timer_Status_t;


typedef enum{
	  TIMER_OK                            = 0,
	  TIMER_TIMEOUT                       = 1,
	  TIMER_ERROR                         = 2,
	  TIMER_STARTED						  = 3,
	  TIMER_STOPPED						  = 4,
}Timer_StatusTypedef;


typedef struct{
	Timer_StatusTypedef status;
	TimerHandle_t timer;
	uint32_t period;
}Timer_Typedef_t;

Timer_StatusTypedef Timer_Create(TimerHandle_t * timer, const TickType_t xTimerPeriodInTicks, const UBaseType_t uxAutoReload, Timer_StatusTypedef* flag_status);
Timer_StatusTypedef Timer_Change_Period(TimerHandle_t * timer, uint32_t newPeriod);
Timer_StatusTypedef Timer_Stop(TimerHandle_t * timer);
Timer_StatusTypedef Timer_Start(TimerHandle_t * timer);

#endif /* INC_TIMER_FREERTOS_H_ */
