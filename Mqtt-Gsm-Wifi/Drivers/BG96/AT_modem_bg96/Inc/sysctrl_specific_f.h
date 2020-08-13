/*
 * sysctrl_specific_f.h
 *
 *  Created on: Jul 22, 2020
 *      Author: fedepacher
 */

#ifndef BG96_AT_MODEM_BG96_INC_SYSCTRL_SPECIFIC_F_H_
#define BG96_AT_MODEM_BG96_INC_SYSCTRL_SPECIFIC_F_H_

/* Includes ------------------------------------------------------------------*/
#include "sysctrl_f.h"

/* Exported constants --------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
//sysctrl_status_t SysCtrl_BG96_getDeviceDescriptor(sysctrl_device_type_t type, sysctrl_info_t *p_devices_list);
//sysctrl_status_t SysCtrl_BG96_open_channel(sysctrl_device_type_t type);
//sysctrl_status_t SysCtrl_BG96_close_channel(sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_BG96_power_on(void);
sysctrl_status_t SysCtrl_BG96_power_off(void);
sysctrl_status_t SysCtrl_BG96_reset(void);
sysctrl_status_t SysCtrl_BG96_sim_select(sysctrl_sim_slot_t sim_slot);

#ifdef __cplusplus
}
#endif

#endif /* BG96_AT_MODEM_BG96_INC_SYSCTRL_SPECIFIC_F_H_ */
