/*
 * defs.h
 *
 *  Created on: Jul 22, 2020
 *      Author: fedepacher
 */

#ifndef BG96_AT_MODEM_BG96_INC_DEFS_H_
#define BG96_AT_MODEM_BG96_INC_DEFS_H_

#include "cmsis_os.h"
#include "main.h"
#include "stm32f411xe.h"

#define	RTOS_USED	1
#define UNUSED(X) (void)X      /* To avoid gcc/g++ warnings */

#define CONFIG_MODEM_UART_BAUDRATE (115200U)

/* UART interface */
#define MODEM_UART_BAUDRATE     (CONFIG_MODEM_UART_BAUDRATE)
#define MODEM_UART_WORDLENGTH   UART_WORDLENGTH_8B
#define MODEM_UART_STOPBITS     UART_STOPBITS_1
#define MODEM_UART_PARITY       UART_PARITY_NONE
#define MODEM_UART_MODE         UART_MODE_TX_RX

#define MODEM_UART_HWFLOWCTRL   UART_HWCONTROL_RTS_CTS
#define MODEM_TX_GPIO_PORT      ((GPIO_TypeDef *)USART1_TX_GPIO_Port)   /* for DiscoL496: GPIOB       */
#define MODEM_TX_PIN            USART1_TX_Pin                           /* for DiscoL496: GPIO_PIN_6  */
#define MODEM_RX_GPIO_PORT      ((GPIO_TypeDef *)UART1_RX_GPIO_Port)    /* for DiscoL496: GPIOG       */
#define MODEM_RX_PIN            UART1_RX_Pin                            /* for DiscoL496: GPIO_PIN_10 */
#define MODEM_CTS_GPIO_PORT     ((GPIO_TypeDef *)UART1_CTS_GPIO_Port)   /* for DiscoL496: GPIOG       */
#define MODEM_CTS_PIN           UART1_CTS_Pin                           /* for DiscoL496: GPIO_PIN_11 */
#define MODEM_RTS_GPIO_PORT     ((GPIO_TypeDef *)UART1_RTS_GPIO_Port)   /* for DiscoL496: GPIOG       */
#define MODEM_RTS_PIN           UART1_RTS_Pin                           /* for DiscoL496: GPIO_PIN_12 */

/* output */
#define MODEM_RST_GPIO_PORT     ((GPIO_TypeDef *)MDM_RST_GPIO_Port)    /* for DiscoL496: GPIOB      */
#define MODEM_RST_PIN           MDM_RST_Pin                            /* for DiscoL496: GPIO_PIN_2 */
#define MODEM_PWR_EN_GPIO_PORT  ((GPIO_TypeDef *)MDM_PWR_EN_GPIO_Port) /* for DiscoL496: GPIOD      */
#define MODEM_PWR_EN_PIN        MDM_PWR_EN_Pin                         /* for DiscoL496: GPIO_PIN_3 */
#define MODEM_DTR_GPIO_PORT     ((GPIO_TypeDef *)MDM_DTR_GPIO_Port)    /* for DiscoL496: GPIOA      */
#define MODEM_DTR_PIN           MDM_DTR_Pin                            /* for DiscoL496: GPIO_PIN_0 */
/* input */
#define MODEM_RING_GPIO_PORT    ((GPIO_TypeDef *)STMOD_INT_GPIO_Port)  /* for DiscoL496: GPIOH      */
#define MODEM_RING_PIN          STMOD_INT_Pin                          /* for DiscoL496: GPIO_PIN_2 */
#define MODEM_RING_IRQN         EXTI2_IRQn

/* ---- MODEM SIM SELECTION pins ---- */
#define MODEM_SIM_SELECT_0_GPIO_PORT  MDM_SIM_SELECT_0_GPIO_Port
#define MODEM_SIM_SELECT_0_PIN        MDM_SIM_SELECT_0_Pin
#define MODEM_SIM_SELECT_1_GPIO_PORT  MDM_SIM_SELECT_1_GPIO_Port
#define MODEM_SIM_SELECT_1_PIN        MDM_SIM_SELECT_1_Pin


#endif /* BG96_AT_MODEM_BG96_INC_DEFS_H_ */
