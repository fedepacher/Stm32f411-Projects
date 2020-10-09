/*
 * defs.h
 *
 *  Created on: Aug 27, 2020
 *      Author: fedepacher
 */

#ifndef INC_GENERAL_DEFS_H_
#define INC_GENERAL_DEFS_H_


#define DEBUG					1
#define WRITE_CHAR				0
#define ME_CONSOLE				1

#define END_OF_ARRAY			"EOA"

// Strings definitions.
#define AT_OK_STRING            "OK\r\n"
#define AT_IPD_OK_STRING        "OK\r\n\r\n"
#define AT_SEND_OK_STRING       "SEND OK\r\n"
#define AT_SEND_PROMPT_STRING   ">"
#define AT_ERROR_STRING         "ERROR\r\n"
#define AT_IPD_STRING           "+IPD,"
#define QMTSUB_STRING   		"+QMTSUB:"
#define QMTUNS_STRING			"+QMTUNS:"
#define QMTCONN_STRING   		"+QMTCONN:"
#define QMTOPEN_STRING			"+QMTOPEN:"
#define QMTDISC_STRING			"+QMTDISC:"
#define QMTCLOSE_STRING			"+QMTCLOSE:"

//Microelect commands
#define AT_MEWIFI_STRING		"MEWIFI"
#define AT_MEGSM_STRING			"MEGSM"
#define AT_MEDISCWIFI_STRING	"MEDISCWIFI"
#define AT_MEDISCGSM_STRING		"MEDISCGSM"
#define AT_MECONNOK_STRING		"AT+MECONNOK\r\n"
#define AT_MECWLAP_STRING		"MECWLAP"
#define AT_MECWJAP_STRING		"MECWJAP"
//#define AT_MERECOK_STRING		"AT+MERECOK\r\n"
#define AT_MEREC_STRING			"MEREC"

// Buffer settings.
#define BUFFERSIZE_RESPONSE 	1500UL
#define BUFFERSIZE_CMD 			256UL

// Timing settings.
#define KEEPALIVE_MAX			3600

//CMD maximun timeout
#define CMD_TIMEOUT_300			300U
#define CMD_TIMEOUT_3000		3000U
#define CMD_TIMEOUT_5000		5000U
#define CMD_TIMEOUT_15000		15000U
#define CMD_TIMEOUT_75000		75000U
#define CMD_TIMEOUT_140000		140000U
#define CMD_TIMEOUT_150000		150000U
#define CMD_TIMEOUT_180000		180000U



#endif /* INC_GENERAL_DEFS_H_ */
