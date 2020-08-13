/*
 * sysctrl_f.h
 *
 *  Created on: Jul 22, 2020
 *      Author: fedepacher
 */

#ifndef BG96_AT_MODEM_BG96_INC_SYSCTRL_F_H_
#define BG96_AT_MODEM_BG96_INC_SYSCTRL_F_H_

#include "stdio.h"

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  SCSTATUS_OK = 0,
  SCSTATUS_ERROR,
} sysctrl_status_t;

typedef enum
{
  DEVTYPE_MODEM_CELLULAR = 0,
  /*  DEVTYPE_WIFI, */
  /*  DEVTYPE_GPS, */
  /* etc... all modules using AT commands */

  /* --- */
  DEVTYPE_INVALID,      /* keep it last */
} sysctrl_device_type_t;

typedef enum
{
  SC_MODEM_SIM_SOCKET_0 = 0,    /* to select SIM card placed in SIM socket */
  SC_MODEM_SIM_ESIM_1,          /* to select integrated SIM in modem module */
  SC_STM32_SIM_2,               /* to select SIM in STM32 side (various implementations) */
} sysctrl_sim_slot_t;


void SysCtrl_delay(uint32_t timeMs);

#endif /* BG96_AT_MODEM_BG96_INC_SYSCTRL_F_H_ */
