/*
 * sysctrl_specific_f.c
 *
 *  Created on: Jul 22, 2020
 *      Author: fedepacher
 */

/* Includes ------------------------------------------------------------------*/
#include <defs.h>
#include "sysctrl_f.h"
#include "sysctrl_specific_f.h"

#define PRINT_FORCE(format, args...) (void) printf(format "\n\r", ## args)
#define PRINT_INFO(format, args...)  (void) printf("SysCtrl_BG96:" format "\n\r", ## args)
#define PRINT_ERR(format, args...)   (void) printf("SysCtrl_BG96 ERROR:" format "\n\r", ## args)

/* Private defines -----------------------------------------------------------*/
#define BG96_BOOT_TIME (5500U) /* Time in ms allowed to complete the modem boot procedure
                                *  according to spec, time = 13 sec
                                *  but pratically, it seems that about 5 sec is acceptable
                                */

/* Private variables ---------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static sysctrl_status_t SysCtrl_BG96_setup(void);

/* Functions Definition ------------------------------------------------------*/
sysctrl_status_t SysCtrl_BG96_power_on(void)
{

  sysctrl_status_t retval = SCSTATUS_OK;
  SysCtrl_BG96_setup();
  SysCtrl_delay(3000U);

  /* Reference: Quectel BG96&EG95&UG96&M95 R2.0_Compatible_Design_V1.0
  *  PWRKEY   connected to MODEM_PWR_EN (inverse pulse)
  *  RESET_N  connected to MODEM_RST    (inverse)
  *
  * Turn ON module sequence (cf paragraph 4.2)
  *
  *          PWRKEY   modem_state
  * init       0        OFF
  * T=0        1        OFF
  * T1=100     0        BOOTING
  * T1+100     1        BOOTING
  * T1+13000   1        RUNNING
  */

  /* POWER ON sequence
  * PWR_EN to 0
  */
  /* Set PWR_EN to 1 during at least 30 ms */

  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_SET);
  SysCtrl_delay(50U);
  /* Set PWR_EN to 0 during at least 100ms (200ms) then PWR_EN = 0 */
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_RESET);
  SysCtrl_delay(200U);
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_SET);

  /* wait for Modem to complete its booting procedure */
  PRINT_INFO("Waiting %d millisec for modem running...", BG96_BOOT_TIME);
  SysCtrl_delay(BG96_BOOT_TIME);
  PRINT_INFO("...done");

  return (retval);
}

sysctrl_status_t SysCtrl_BG96_power_off(void)
{

  sysctrl_status_t retval = SCSTATUS_OK;

  /* Reference: Quectel BG96&EG95&UG96&M95 R2.0_Compatible_Design_V1.0
  *  PWRKEY   connected to MODEM_PWR_EN (inverse pulse)
  *  RESET_N  connected to MODEM_RST    (inverse)
  *
  * Turn OFF module sequence (cf paragraph 4.3)
  *
  * Need to use AT+QPOWD command
  * reset GPIO pins to initial state only after completion of previous command (between 2s and 40s)
  */

  /* POWER OFF sequence
  * set PWR_EN and wait at least 650ms (800ms) then PWR_EN = 0
  */
  /* No HW power off for BG96 (done by AT command) */
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_RESET);
  SysCtrl_delay(800U);

  /* wait at least 2000ms but, in pratice, it can exceed 4000ms */
  SysCtrl_delay(5000U);

  return (retval);
}

sysctrl_status_t SysCtrl_BG96_reset(void)
{

  sysctrl_status_t retval = SCSTATUS_OK;

  /* Reference: Quectel BG96&EG95&UG96&M95 R2.0_Compatible_Design_V1.0
  *  PWRKEY   connected to MODEM_PWR_EN (inverse pulse)
  *  RESET_N  connected to MODEM_RST    (inverse)
  *
  * Reset module sequence (cf paragraph 4.4)
  *
  * Can be done using RESET_N pin to low voltage for 100ms minimum
  *
  *          RESET_N  modem_state
  * init       1        RUNNING
  * T=0        0        OFF
  * T=150      1        BOOTING
  * T>=460     1        RUNNING
  */
  PRINT_INFO("!!! Hardware Reset triggered !!!");
  /* set RST to 1 for a time between 150ms and 460ms (200) */
  HAL_GPIO_WritePin(MODEM_RST_GPIO_PORT, MODEM_RST_PIN, GPIO_PIN_SET);
  SysCtrl_delay(200U);
  HAL_GPIO_WritePin(MODEM_RST_GPIO_PORT, MODEM_RST_PIN, GPIO_PIN_RESET);

  /* wait for Modem to complete its restart procedure */
  PRINT_INFO("Waiting %d millisec for modem running...", BG96_BOOT_TIME);
  SysCtrl_delay(BG96_BOOT_TIME);
  PRINT_INFO("...done");

  return (retval);
}

sysctrl_status_t SysCtrl_BG96_sim_select(sysctrl_sim_slot_t sim_slot)
{

  sysctrl_status_t retval = SCSTATUS_OK;

  switch (sim_slot)
  {
    case SC_MODEM_SIM_SOCKET_0:
      HAL_GPIO_WritePin(MODEM_SIM_SELECT_0_GPIO_PORT, MODEM_SIM_SELECT_0_PIN, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(MODEM_SIM_SELECT_1_GPIO_PORT, MODEM_SIM_SELECT_1_PIN, GPIO_PIN_RESET);
      PRINT_INFO("MODEM SIM SOCKET SELECTED");
      break;

    case SC_MODEM_SIM_ESIM_1:
      HAL_GPIO_WritePin(MODEM_SIM_SELECT_0_GPIO_PORT, MODEM_SIM_SELECT_0_PIN, GPIO_PIN_SET);
      HAL_GPIO_WritePin(MODEM_SIM_SELECT_1_GPIO_PORT, MODEM_SIM_SELECT_1_PIN, GPIO_PIN_RESET);
      PRINT_INFO("MODEM SIM ESIM SELECTED");
      break;

    case SC_STM32_SIM_2:
      HAL_GPIO_WritePin(MODEM_SIM_SELECT_0_GPIO_PORT, MODEM_SIM_SELECT_0_PIN, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(MODEM_SIM_SELECT_1_GPIO_PORT, MODEM_SIM_SELECT_1_PIN, GPIO_PIN_SET);
      PRINT_INFO("STM32 SIM SELECTED");
      break;

    default:
      PRINT_ERR("Invalid SIM %d selected", sim_slot);
      retval = SCSTATUS_ERROR;
      break;
  }

  return (retval);
}

/* Private function Definition -----------------------------------------------*/
static sysctrl_status_t SysCtrl_BG96_setup(void)
{
  sysctrl_status_t retval = SCSTATUS_OK;

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO config
   * Initial pins state:
   *  PWR_EN initial state = 1 : used to power-on/power-off the board
   *  RST initial state = 0 : used to reset the board
   *  DTR initial state = 0 ; not used
   */
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_PORT, MODEM_PWR_EN_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(MODEM_RST_GPIO_PORT, MODEM_RST_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(MODEM_DTR_GPIO_PORT, MODEM_DTR_PIN, GPIO_PIN_RESET);

  /* Init GPIOs - common parameters */
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  /* Init GPIOs - RESET pin */
  GPIO_InitStruct.Pin = MODEM_RST_PIN;
  HAL_GPIO_Init(MODEM_RST_GPIO_PORT, &GPIO_InitStruct);

  /* Init GPIOs - DTR pin */
  GPIO_InitStruct.Pin = MODEM_DTR_PIN;
 HAL_GPIO_Init(MODEM_DTR_GPIO_PORT, &GPIO_InitStruct);

  /* Init GPIOs - PWR_EN pin */
  GPIO_InitStruct.Pin = MODEM_PWR_EN_PIN;
  HAL_GPIO_Init(MODEM_PWR_EN_GPIO_PORT, &GPIO_InitStruct);

  PRINT_FORCE("BG96 UART config: BaudRate=%d / HW flow ctrl=%d", MODEM_UART_BAUDRATE,
              ((MODEM_UART_HWFLOWCTRL == UART_HWCONTROL_NONE) ? 0 : 1));

  return (retval);
}
