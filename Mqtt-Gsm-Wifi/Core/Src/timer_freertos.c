/*
 * timer_freertos.c
 *
 *  Created on: Jul 25, 2020
 *      Author: fedepacher
 */

#include "timer_freertos.h"

Timer_StatusTypedef * cmd_timeout_status_ptr;
//TimerHandle_t * timer_timeout_cmd_ptr;

static void timer_CallbackFunction(TimerHandle_t timerID);
/*
 * @brief INTERNAL Timeout at command.
 * @param interval_ms Interval time in ms.
 * @return none
 */
static void timer_CallbackFunction(TimerHandle_t timerID)
{
	Timer_StatusTypedef * param = (Timer_StatusTypedef *)pvTimerGetTimerID(timerID);
	*param = TIMER_TIMEOUT;
}


/*
 * @brief Create FreeRTOS Timer
 * @param timer	handler
 * @param xTimerPeriodInTicks Period of time
 * @param uxAutoReload Periodic pdTRUE otherwise pdFALSE
 * @param flag_status flag tha indicate status of the timer
 * @return status
 */
Timer_StatusTypedef Timer_Create(TimerHandle_t * timer, const TickType_t xTimerPeriodInTicks, const UBaseType_t uxAutoReload, Timer_StatusTypedef* flag_status){
	cmd_timeout_status_ptr = flag_status;
	*cmd_timeout_status_ptr = TIMER_STOPPED;
	*timer = xTimerCreate("timeout_cmd", xTimerPeriodInTicks, uxAutoReload, (void*)cmd_timeout_status_ptr, timer_CallbackFunction);
	if(timer != NULL)
		return TIMER_OK;
	return TIMER_ERROR;

}

/*
 * @brief Start timer
 * @param timer	handler
 * @param newPeriod
 * @return status
 */
Timer_StatusTypedef Timer_Start(TimerHandle_t * timer){
	*cmd_timeout_status_ptr = TIMER_STARTED;
	if( xTimerStart(*timer, 100 ) == pdPASS )
	{
			return TIMER_OK;/* The command was successfully sent. */
	}
	else
	{
		return TIMER_ERROR;/* The command could not be sent, even after waiting for 100 ticks
	            to pass.  Take appropriate action here. */
	}
}

/*
 * @brief change period and start timer
 * @param timer	handler
 * @param newPeriod
 * @return status
 */
Timer_StatusTypedef Timer_Change_Period(TimerHandle_t * timer, uint32_t newPeriod){
	*cmd_timeout_status_ptr = TIMER_STARTED;
	if( xTimerChangePeriod( *timer, newPeriod, 100 ) == pdPASS )
	{
			return TIMER_OK;/* The command was successfully sent. */
	}
	else
	{
		return TIMER_ERROR;/* The command could not be sent, even after waiting for 100 ticks
	            to pass.  Take appropriate action here. */
	}
}

/*
 * @brief Stop timer
 * @param timer	handler
 * @return status
 */
Timer_StatusTypedef Timer_Stop(TimerHandle_t * timer){
	*cmd_timeout_status_ptr = TIMER_STOPPED;
	if(xTimerStop(*timer, 100) == pdPASS  )
	{
		return TIMER_OK;/* The command was successfully sent. */
	}
	else
	{
		return TIMER_ERROR;/* The command could not be sent, even after waiting for 100 ticks
		            to pass.  Take appropriate action here. */
	}
}
