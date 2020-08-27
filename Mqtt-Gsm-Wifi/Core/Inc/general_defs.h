/*
 * defs.h
 *
 *  Created on: Aug 27, 2020
 *      Author: fedepacher
 */

#ifndef INC_GENERAL_DEFS_H_
#define INC_GENERAL_DEFS_H_

#define	ACTIVATE_WIFI			1
#define DEBUG					1
#define WRITE_CHAR				1

#define END_OF_ARRAY			"EOA"

// Strings definitions.
#define AT_OK_STRING            "OK\r\n"
#define AT_IPD_OK_STRING        "OK\r\n\r\n"
#define AT_SEND_OK_STRING       "SEND OK\r\n"
#define AT_SEND_PROMPT_STRING   ">"
#define AT_ERROR_STRING         "ERROR\r\n"
#define AT_IPD_STRING           "+IPD,"
#define QMTSUB_STRING   		"+QMTSUB:"
#define QMTCONN_STRING   		"+QMTCONN:"
#define QMTOPEN_STRING			"+QMTOPEN:"
// Buffer settings.
#define BUFFERSIZE_RESPONSE 	1500UL
#define BUFFERSIZE_CMD 			128UL

// Timing settings.
//#define TIMEOUT_MS_RESTART       2000UL///< Module restart timeout.
//#define DEFAULT_TIME_OUT         5000UL /* in ms */
//#define LONG_TIME_OUT         	20000UL /* in ms */

#define KEEPALIVE_MAX			3600

//CMD maximun timeout
#define CMD_TIMEOUT_300			300U
#define CMD_TIMEOUT_5000		5000U
#define CMD_TIMEOUT_15000		15000U
#define CMD_TIMEOUT_75000		75000U
#define CMD_TIMEOUT_140000		140000U
#define CMD_TIMEOUT_150000		150000U
#define CMD_TIMEOUT_180000		180000U


typedef enum
{
  ESP8266_OK                            = 0,
  ESP8266_ERROR                         = 1,
  ESP8266_BUSY                          = 2,
  ESP8266_ALREADY_CONNECTED             = 3,
  ESP8266_CONNECTION_CLOSED             = 4,
  ESP8266_TIMEOUT                       = 5,
  ESP8266_IO_ERROR                      = 6,
  ESP8266_EOA		                    = 7,
} ESP8266_StatusTypeDef;

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  ESP8266_FALSE         = 0,
  ESP8266_TRUE          = 1
} ESP8266_Boolean;

#endif /* INC_GENERAL_DEFS_H_ */
