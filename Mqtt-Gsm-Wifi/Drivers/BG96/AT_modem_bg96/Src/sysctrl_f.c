/*
 * sysctrl_f.c
 *
 *  Created on: Jul 22, 2020
 *      Author: fedepacher
 */

#include "sysctrl_f.h"
#include "defs.h"


/* Private variables ---------------------------------------------------------*/
/* AT custom functions ptrs table */
//static sysctrl_funcPtrs_t sysctrl_custom_func[DEVTYPE_INVALID] = {0U};

/**
  * @brief  SysCtrl_delay
  * @param  timeMs
  * @retval none
  */
void SysCtrl_delay(uint32_t timeMs)
{
#if (RTOS_USED == 1)
  (void) osDelay(timeMs / portTICK_PERIOD_MS);
#else
  HAL_Delay(timeMs);
#endif /* RTOS_USED */
}
