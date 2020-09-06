/*
 * mytasks.c
 *
 *  Created on: Jul 20, 2020
 *      Author: fedepacher
 */

#include <connect_ClientBG96.h>
#include <general_defs.h>
#include "mytasks.h"
#include "uart.h"
#include "stdint.h"
#include <stdio.h>
#include <string.h>
#include "cmsis_os.h"
#include "semphr.h"
#include "credentials.h"
#include "sysctrl_specific_f.h"
#include "debounce.h"
#include "timer_freertos.h"
#include "mqtt.h"

/* Private define ------------------------------------------------------------*/

#define CTRL(x) (#x[0]-'a'+1)

#define STACK_SIZE				512UL
#define CONSOLE_QUEUE_LENGTH	50
#define CMD_LENGTH				128
#define TRIAL					1

//FSM maximun timeout
#define FSM_TIMEOUT_100			100U
#define FSM_TIMEOUT_20000		20000U	//20 seg
#define FSM_TIMEOUT_60000		60000U	//60 seg
#define FSM_TIMEOUT_90000		90000U	//90 seg
#define FSM_TIMEOUT_150000		150000U	//150 seg

//_A = ASSIGNATION
//_Q = QUERY

#define AT_IFC_A				"AT+IFC=2,2\r\n"				//Set TE-TA Local Data Flow Control = 2 RTS flow control, 2 CTS flow control
#define AT_CMD					"AT\r\n"
#define ATE1_A					"ATE1\r\n"						//Set Command Echo Mode = 1 echo on
#define AT_CMEE_A				"AT+CMEE=2\r\n"					//Error Message Format = 2 Enable result code and use verbose values
#define ATV1_A					"ATV1\r\n"						//TA Response Format = 1 Information response: <CR><LF><text><CR><LF>
#define AT_AND_D0_A				"AT&D0\r\n"						//Set DTR Function Mode = 0 TA ignores status on DTR.
#define AT_CGMR_CMD				"AT+CGMR\r\n"					//Request TA Revision Identification of Software Release
#define AT_CFUN_0_A				"AT+CFUN=0,0\r\n"				//Set Phone Functionality = 0 Minimum functionality, 0 Do not reset the ME before setting it to <fun> functionality level.
#define AT_QCFG_BAND_A			"AT+QCFG=\"band\"\r\n"			//Return Band Configuration
#define AT_QCFG_IOT_A			"AT+QCFG=\"iotopmode\"\r\n"		//Return Configure Network Category to be Searched under LTE RAT
#define AT_QCFG_NWSCANSEQ_A		"AT+QCFG=\"nwscanseq\"\r\n"		//Return Configure RAT Searching Sequence
#define AT_QCFG_NWSCANMODE_A	"AT+QCFG=\"nwscanmode\"\r\n"	//Return Configure RAT(s) to be Searched

#define AT_QICSGP1_A			"AT+QICSGP=1,1,\"\",\"\",\"\",0\r\n"		//Configure Parameters of a TCP/IP Context
#define AT_QICSGP2_A			"AT+QICSGP=1\r\n"
#define AT_CFUN_1_A				"AT+CFUN=1,0\r\n"						//Set Phone Functionality = 1 Full functionality, 0 Do not reset the ME before setting it to <fun> functionality level.
#define AT_QCCID_CMD			"AT+QCCID\r\n"							//Show ICCID
#define AT_QINITSTAT_CMD		"AT+QINISTAT\r\n"					//Query Initialization Status of (U)SIM Card

#define AT_CPIN_Q				"AT+CPIN?\r\n"
#define AT_CGDCONT_Q			"AT+CGDCONT?\r\n"						//Define PDP Context
#define AT_CREG_A				"AT+CREG=2\r\n"						//Network Registration Status
#define AT_CGREG_A				"AT+CGREG=2\r\n"
#define AT_CEREG_A				"AT+CEREG=2\r\n"
#define AT_GSN_CMD				"AT+GSN\r\n"						//Request International Mobile Equipment Identity (IMEI)
#define AT_CGMI_CMD				"AT+CGMI\r\n"						//Request Manufacturer Identification
#define AT_CGMM_CMD				"AT+CGMM\r\n"						//Request Model Identification
#define AT_QGMR_CMD				"AT+QGMR\r\n"						//Firmware Version
#define AT_CGSN_CMD				"AT+CGSN\r\n"						//Request Product Serial Number Identification

#define AT_CIMI_CMD				"AT+CIMI\r\n"						//Request International Mobile Subscriber Identity (IMSI)
#define AT_COPS_Q				"AT+COPS?\r\n"						//Operator Selection
#define AT_COPS_A				"AT+COPS=0\r\n"						//Operator Selection = 0 automatic mode
#define AT_CEREG_Q				"AT+CEREG?\r\n"						//EPS Network Registration Status
#define AT_CREG_Q				"AT+CREG?\r\n"						//Network Registration Status
#define AT_CGREG_Q				"AT+CGREG?\r\n"						//Network Registration Status
#define AT_CGATT_A				"AT+CGATT=1\r\n"					//Attachment or Detachment of PS = 1 Attached
#define AT_CSQ_CMD				"AT+CSQ\r\n"						//Signal Quality Report
#define AT_QCSQ_CMD				"AT+QCSQ\r\n"
#define AT_QNWINFO_CMD			"AT+QNWINFO\r\n"					//Query Network Information
#define AT_CGATT_Q				"AT+CGATT?\r\n"						//Attachment or Detachment of PS
#define AT_CGEREP_A				"AT+CGEREP=1,0\r\n"					//Packet Domain Event Report
#define AT_QIACT_Q				"AT+QIACT?\r\n"						//Activate a PDP Context
#define AT_QIACT_A				"AT+QIACT=1\r\n"					//Activate a PDP Context

//#define AT_QIOPEN_A				"AT+QIOPEN=1,0,\"TCP\",\"18.197.138.171\",1883,0,0\r\n"					//Activate a PDP Context
//#define AT_QISTATE_A			"AT+QISTATE=1,0\r\n"
//#define AT_QISEND_A				"AT+QISEND=0\r\n"

/*#define AT_QMTCFG_VERSION		"AT+QMTCFG=\"version\",0,3\r\n"
 #define AT_QMTCFG_PDPCID		"AT+QMTCFG=\"pdpcid\",0,1\r\n"
 #define AT_QMTCFG_WILL			"AT+QMTCFG=\"will\",0\r\n"
 #define AT_QMTCFG_TIMEOUT		"AT+QMTCFG=\"timeout\",0\r\n"
 #define AT_QMTCFG_SESSION		"AT+QMTCFG=\"session\",0,0,\r\n"
 #define AT_QMTCFG_KEEPALIVE		"AT+QMTCFG=\"keepalive\",0,120,\r\n"
 #define AT_QMTCFG_SSL 			"AT+QMTCFG=\"ssl\",0,0,0,\r\n"


 #define AT_QMTOPEN_Q			"AT+QMTOPEN=?\r\n"
 #define AT_QMTOPEN_A			"AT+QMTOPEN=0,\"broker.hivemq.com\",1883\r\n"
 #define AT_QMTOPEN1_Q			"AT+QMTOPEN?\r\n"
 #define AT_QMTCONN_Q			"AT+QMTCONN=?\r\n"
 #define AT_QMTCONN_A			"AT+QMTCONN=0,\"fedeID\",\"\",\"\"\r\n"*/
#define AT_QMTSUB_Q				"AT+QMTSUB=?\r\n"
#define AT_QMTSUB1_A			"AT+QMTSUB=0,1,\"topic/example\",0\r\n"
#define AT_QMTUNS_A				"AT+QMTUNS=0,2,\"topic/example\"\r\n"
#define AT_QMTPUB_Q				"AT+QMTPUB=?\r\n"
#define AT_QMTDISC_A			"AT+QMTDISC=0\r\n"

//#define MSG						"This is test data, hello my dear MQTT.\r\n"

//#define AT_QICSGP_A				"AT+QICSGP=1,1,\"datos.personal.com\",\"datos\",\"datos\",1\r\n"			//Configure Parameters of a TCP/IP Context
//#define AT_CGQREQ_Q				"AT+CGQREQ?\r\n"//VER ESTE COMANDO	//Quality of Service Profile (Requested)
//"AT+CGEQREQ"
//#define AT_CGQMIN_Q				"AT+CGQMIN?"//VER ESTE COMANDO
//"AT+CGEQMIN"

typedef struct {
	uint8_t cmd[CMD_LENGTH];
	uint8_t response[CMD_LENGTH];
	uint32_t cmd_timeout;
	uint32_t fsm_timeout;
} cmd_type_t;

static const cmd_type_t BG96_COMMAND_INIT[] = { { AT_IFC_A, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_100 }, { AT_CMD, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_100 }, { ATE1_A, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_100 }, { AT_CMEE_A, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_100 }, { ATV1_A, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_100 }, { AT_AND_D0_A, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_100 }, { AT_CGMR_CMD, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_100 }, { AT_CFUN_0_A, AT_OK_STRING,
CMD_TIMEOUT_15000, FSM_TIMEOUT_100 }, { AT_QCFG_BAND_A, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_100 }, { AT_QCFG_IOT_A, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_100 }, { AT_QCFG_NWSCANSEQ_A, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_100 }, { AT_QCFG_NWSCANMODE_A,
AT_OK_STRING, CMD_TIMEOUT_300, FSM_TIMEOUT_100 },

{ END_OF_ARRAY, AT_OK_STRING, CMD_TIMEOUT_300, FSM_TIMEOUT_100 }, };

static const cmd_type_t BG96_COMMAND_1[] = { { AT_QICSGP1_A, AT_OK_STRING,
CMD_TIMEOUT_5000, FSM_TIMEOUT_20000 }, { AT_QICSGP2_A, AT_OK_STRING,
CMD_TIMEOUT_5000, FSM_TIMEOUT_20000 }, { AT_CFUN_1_A, AT_OK_STRING,
CMD_TIMEOUT_15000, FSM_TIMEOUT_20000 }, { AT_QCCID_CMD, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_20000 },

{ END_OF_ARRAY, AT_OK_STRING, CMD_TIMEOUT_300, FSM_TIMEOUT_100 }, };

static const cmd_type_t BG96_COMMAND_2[] = {

{ AT_QINITSTAT_CMD, AT_ERROR_STRING, CMD_TIMEOUT_300,
FSM_TIMEOUT_20000 }, { AT_QINITSTAT_CMD,
AT_ERROR_STRING, CMD_TIMEOUT_300, FSM_TIMEOUT_20000 }, {
AT_QINITSTAT_CMD, AT_ERROR_STRING, CMD_TIMEOUT_300,
FSM_TIMEOUT_20000 },

{ END_OF_ARRAY, AT_OK_STRING, CMD_TIMEOUT_300, FSM_TIMEOUT_100 }, };

static const cmd_type_t BG96_COMMAND_3[] = {  { AT_CPIN_Q, AT_OK_STRING,
CMD_TIMEOUT_5000, FSM_TIMEOUT_20000 }, { AT_QGMR_CMD, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_60000 }, { AT_CPIN_Q, AT_OK_STRING,
CMD_TIMEOUT_5000, FSM_TIMEOUT_20000 }, { AT_CGDCONT_Q, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_20000 }, { AT_CREG_A, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_90000 }, { AT_CGREG_A, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_60000 }, { AT_CEREG_A, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_60000 }, { AT_GSN_CMD, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_60000 }, { AT_CGMI_CMD, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_60000 }, { AT_CGMM_CMD, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_60000 }, { AT_QGMR_CMD, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_60000 }, { AT_CGSN_CMD, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_60000 }, { AT_QCCID_CMD, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_20000 }, { AT_CIMI_CMD, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_20000 }, { AT_COPS_Q, AT_OK_STRING,
CMD_TIMEOUT_180000, FSM_TIMEOUT_20000 }, { AT_COPS_A, AT_OK_STRING,
CMD_TIMEOUT_180000, FSM_TIMEOUT_20000 }, { AT_CEREG_Q, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_60000 }, { AT_CREG_Q, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_90000 }, { AT_CGREG_Q, AT_OK_STRING,
CMD_TIMEOUT_300, FSM_TIMEOUT_60000 }, { AT_CGATT_A, AT_OK_STRING,
CMD_TIMEOUT_140000, FSM_TIMEOUT_60000 },
/*{AT_CSQ_CMD, 				CMD_TIMEOUT_300,		FSM_TIMEOUT_60000},
 {AT_QCSQ_CMD, 				CMD_TIMEOUT_5000,		FSM_TIMEOUT_60000},
 {AT_QNWINFO_CMD, 			CMD_TIMEOUT_300,		FSM_TIMEOUT_60000},
 {AT_CEREG_Q, 				CMD_TIMEOUT_300, 		FSM_TIMEOUT_60000},
 {AT_CREG_Q,					CMD_TIMEOUT_300, 		FSM_TIMEOUT_90000},
 {AT_CGREG_Q, 				CMD_TIMEOUT_300, 		FSM_TIMEOUT_60000},
 {AT_COPS_Q, 				CMD_TIMEOUT_180000,		FSM_TIMEOUT_20000},
 {AT_CEREG_Q, 				CMD_TIMEOUT_300, 		FSM_TIMEOUT_60000},
 {AT_CREG_Q,					CMD_TIMEOUT_300, 		FSM_TIMEOUT_90000},
 {AT_CGREG_Q, 				CMD_TIMEOUT_300, 		FSM_TIMEOUT_60000},
 {AT_COPS_Q, 				CMD_TIMEOUT_180000,		FSM_TIMEOUT_20000},
 {AT_CGATT_Q, 				CMD_TIMEOUT_140000,		FSM_TIMEOUT_60000},*/
{ AT_CGEREP_A, AT_OK_STRING, CMD_TIMEOUT_300, FSM_TIMEOUT_60000 }, { AT_QIACT_Q,
AT_OK_STRING, CMD_TIMEOUT_150000, FSM_TIMEOUT_150000 }, { AT_QIACT_A,
AT_OK_STRING, CMD_TIMEOUT_150000, FSM_TIMEOUT_150000 }, { AT_QIACT_Q,
AT_OK_STRING, CMD_TIMEOUT_150000, FSM_TIMEOUT_150000 },
		//{AT_QIACT_Q, 				CMD_TIMEOUT_150000,		FSM_TIMEOUT_60000},
		{ AT_CSQ_CMD, AT_OK_STRING, CMD_TIMEOUT_300, FSM_TIMEOUT_60000 }, {
		AT_QCSQ_CMD, AT_OK_STRING, CMD_TIMEOUT_300, FSM_TIMEOUT_60000 },
		{ AT_QNWINFO_CMD, AT_OK_STRING, CMD_TIMEOUT_300, FSM_TIMEOUT_60000 },
		/*{AT_CSQ_CMD, 				CMD_TIMEOUT_300,		FSM_TIMEOUT_60000},
		 {AT_QCSQ_CMD, 				CMD_TIMEOUT_5000,		FSM_TIMEOUT_60000},
		 {AT_QNWINFO_CMD, 			CMD_TIMEOUT_300,		FSM_TIMEOUT_60000},*/

		{ END_OF_ARRAY, AT_OK_STRING, CMD_TIMEOUT_300, FSM_TIMEOUT_100 }, };

/*static const cmd_type_t BG96_COMMAND_3 [] =
 {
 {AT_QMTCFG_VERSION, 		CMD_TIMEOUT_300, 		FSM_TIMEOUT_100},
 {AT_QMTCFG_PDPCID, 			CMD_TIMEOUT_300, 		FSM_TIMEOUT_100},
 {AT_QMTCFG_WILL, 			CMD_TIMEOUT_300, 		FSM_TIMEOUT_100},
 {AT_QMTCFG_TIMEOUT, 		CMD_TIMEOUT_300, 		FSM_TIMEOUT_100},
 {AT_QMTCFG_SESSION, 		CMD_TIMEOUT_300, 		FSM_TIMEOUT_100},
 {AT_QMTCFG_KEEPALIVE, 		CMD_TIMEOUT_300, 		FSM_TIMEOUT_100},
 {AT_QMTCFG_SSL, 			CMD_TIMEOUT_300, 		FSM_TIMEOUT_100},


 //{AT_QMTOPEN_Q, 				CMD_TIMEOUT_75000, 		FSM_TIMEOUT_100},
 {AT_QMTOPEN_A, 				CMD_TIMEOUT_75000, 		FSM_TIMEOUT_100},
 {AT_QMTOPEN1_Q,				CMD_TIMEOUT_75000, 		FSM_TIMEOUT_100},
 {AT_QMTCONN_Q,				CMD_TIMEOUT_15000, 		FSM_TIMEOUT_100},
 {AT_QMTCONN_A,				CMD_TIMEOUT_15000, 		FSM_TIMEOUT_100},
 //{AT_QMTSUB_Q,				CMD_TIMEOUT_15000, 		FSM_TIMEOUT_100},
 //{AT_QMTSUB1_A,				CMD_TIMEOUT_15000, 		FSM_TIMEOUT_100},
 {AT_QMTSUB1_A,				CMD_TIMEOUT_15000, 		FSM_TIMEOUT_100},
 //{AT_QMTUNS_A,				CMD_TIMEOUT_15000, 		FSM_TIMEOUT_100},
 //{AT_QMTPUB_Q,				CMD_TIMEOUT_15000, 		FSM_TIMEOUT_100},
 //{AT_QMTPUB_A,				CMD_TIMEOUT_15000, 		FSM_TIMEOUT_100},
 //{MSG,						CMD_TIMEOUT_15000, 		FSM_TIMEOUT_100},
 //{AT_QMTDISC_A,						CMD_TIMEOUT_15000, 		FSM_TIMEOUT_100},

 {END_OF_ARRAY,				CMD_TIMEOUT_300, 		FSM_TIMEOUT_100},
 };*/

extern UART_HandleTypeDef huart1;	//connected to bg96
extern UART_HandleTypeDef huart2;	//connected to bg96
extern UART_HandleTypeDef huart6;	//connected to esp8266

static uint8_t host[32] = "broker.hivemq.com"; ///< HostName i.e. "test.mosquitto.org"//"broker.mqttdashboard.com";//
static uint16_t port = 1883; ///< Remote port number.
static uint8_t clientId[] = "fedeID";
static uint8_t userName[] = "";
static uint8_t password[] = "";
static uint8_t tcpconnectID = 0;
static uint8_t vsn = 3;
static uint8_t cid = 1; //					//Atributes defined for connect wifi task
//					osThreadAttr_t publishTask_attributes = { .name =
//							"connectWifi", .stack_mem = &publishTaskBuffer[0],
//							.stack_size = sizeof(publishTaskBuffer), .cb_mem =
//									&publishTaskControlBlock, .cb_size =
//									sizeof(publishTaskControlBlock), .priority =
//									(osPriority_t) osPriorityAboveNormal, };
//
//					res = osThreadNew(publishTask, NULL,
//							&publishTask_attributes);
//					if (res == NULL) {
//						printf("error creacion de tarea\r\n");
//						while (1)
//							;
//					}
static uint8_t clean_session = 0;
static uint16_t keepalive = 120; ///< Default keepalive time in seconds.
static uint8_t sslenable = 0; ///< SSL is disabled by default.

static const uint32_t msgID = 0;
static const uint8_t qos = 0;
static const uint8_t retain = 0;
static uint8_t pub_topic[32] = "topic/pub";
static uint8_t sub_topic[32] = "topic/sub";
static uint8_t pub_data[BUFFERSIZE_CMD];
static uint8_t sub_data[BUFFERSIZE_CMD];

enum connection_t {
	WIFI = 0, GSM = 1
};

uint8_t connection_state;		//=0 gsm,

QueueHandle_t xSemaphoreSub;
QueueHandle_t xQueuePrintConsole;
SemaphoreHandle_t mutex;
button_t button_down;
SemaphoreHandle_t xSemaphoreButton;
SemaphoreHandle_t xSemaphoreMutexUart;
SemaphoreHandle_t xSemaphorePublish;
SemaphoreHandle_t xSemaphoreGSM;
SemaphoreHandle_t xSemaphoreWIFI;
SemaphoreHandle_t xSemaphoreControl;


osThreadId_t printTaskHandle;
uint32_t printTaskBuffer[256];
StaticTask_t printTaskControlBlock;

osThreadId_t buttonsTaskHandle;
uint32_t buttonsTaskBuffer[128];
StaticTask_t buttonsTaskControlBlock;

osThreadId_t connectTaskcHandle;
uint32_t connectTaskBuffer[512];
StaticTask_t connectTaskControlBlock;

osThreadId_t connectWifiTaskcHandle;
uint32_t connectWifiTaskBuffer[256];
StaticTask_t connectWifiTaskControlBlock;

osThreadId_t subscribeTaskcHandle;
uint32_t subscribeTaskBuffer[256];
StaticTask_t subscribeTaskControlBlock;

osThreadId_t publishTaskcHandle;
uint32_t publishTaskBuffer[256];
StaticTask_t publishTaskControlBlock;

osThreadId_t ledTaskcHandle;
uint32_t ledTaskBuffer[128];
StaticTask_t ledTaskControlBlock;

/**
 * @brief	Check flag and set error led, increment internal state if it is needed
 * @param	status			flag that indicate if led should be on or off
 * @param	internalstate	state on FSM
 */
//static int CheckFlag(ESP8266_StatusTypeDef status, int internalState);
static void clean_Timer(TimerHandle_t *timerT);

void initTasks() {

	connection_state = WIFI;
	//button set
	button_down.GPIOx = BTN_GPIO_Port;
	button_down.GPIO_Pin = BTN_Pin;

	//HAL_UART_F_Init(&huart1);

#if WRITE_CHAR
	xQueuePrintConsole = xQueueCreate(100, sizeof(uint8_t));
#else
	xQueuePrintConsole = xQueueCreate(CONSOLE_QUEUE_LENGTH,
			sizeof(data_print_console_t));
#endif

	mutex = xSemaphoreCreateMutex();							//mutex to handle print console
	xSemaphoreMutexUart = xSemaphoreCreateMutex();				//utex to handle uart
	xSemaphoreButton = xSemaphoreCreateCounting(10, 0);			//semaphore ccounting to catch button
	xSemaphorePublish = xSemaphoreCreateBinary();
	xSemaphoreGSM = xSemaphoreCreateCounting(2, 0);				//semaphore counting to activate gsm connection
	xSemaphoreWIFI = xSemaphoreCreateCounting(2, 0);			//semaphore counting to activate wifi connection
	xSemaphoreControl = xSemaphoreCreateCounting(5, 0);			//semaphore counting to get ME1040 commands


	if (xQueuePrintConsole == NULL) {
		printf("error queue creation\r\n");
		while (1)
			;
	}

	if (mutex == NULL || xSemaphoreMutexUart == NULL) {
		printf("error semaphore creation\r\n");
		while (1)
			;
	}

	if (xSemaphoreButton == NULL || xSemaphorePublish == NULL
				|| xSemaphoreGSM == NULL || xSemaphoreWIFI == NULL
				|| xSemaphoreControl == NULL) {
		printf("error buttons semaphore creation\r\n");
		while (1)
			;
	}

	//Atributes defined for print console task
	osThreadAttr_t printConsoleTask_attributes = { .name = "printConsole",
			.stack_mem = &printTaskBuffer[0], .stack_size =
					sizeof(printTaskBuffer), .cb_mem = &printTaskControlBlock,
			.cb_size = sizeof(printTaskControlBlock), .priority =
					(osPriority_t) osPriorityAboveNormal, };

	printTaskHandle = osThreadNew(printConsoleTask, NULL,
			&printConsoleTask_attributes);

	if (printTaskHandle == NULL) {
		printf("error print console task creation\r\n");
		while (1)
			;
	}

	//Atributes defined for buttons task
	osThreadAttr_t buttonsTask_attributes = { .name = "buttons", .stack_mem =
			&buttonsTaskBuffer[0], .stack_size = sizeof(buttonsTaskBuffer),
			.cb_mem = &buttonsTaskControlBlock, .cb_size =
					sizeof(buttonsTaskControlBlock), .priority =
					(osPriority_t) osPriorityAboveNormal, };

	buttonsTaskHandle = osThreadNew(buttonsTask, NULL, &buttonsTask_attributes);
	if (buttonsTaskHandle == NULL) {
		printf("error buttons task creation\r\n");
	}

	//Atributes defined for connect bg96 task
	osThreadAttr_t connectTask_attributes = { .name = "connectBG96",
					.stack_mem = &connectTaskBuffer[0], .stack_size =
							sizeof(connectTaskBuffer), .cb_mem =
							&connectTaskControlBlock, .cb_size =
							sizeof(connectTaskControlBlock), .priority =
							(osPriority_t) osPriorityAboveNormal, };
	switch (connection_state) {
	case GSM:
		connectTaskcHandle = osThreadNew(connectTask, NULL,
				&connectTask_attributes);
		if (connectTaskcHandle == NULL) {
			printf("error connect task creation\r\n");
			while (1)
				;
		}
		break;
	case WIFI:
		connectWifiTaskcHandle = osThreadNew(connectWifiTask, NULL,
				&connectTask_attributes);
		if (connectWifiTaskcHandle == NULL) {
			printf("error creacion de tarea\r\n");
			while (1)
				;
		}

		break;
	}

	//Atributes defined for connect bg96 task
	osThreadAttr_t ledTask_attributes = { .name = "led", .stack_mem =
			&ledTaskBuffer[0], .stack_size = sizeof(ledTaskBuffer), .cb_mem =
			&ledTaskControlBlock, .cb_size = sizeof(ledTaskControlBlock),
			.priority = (osPriority_t) osPriorityNormal, };

	ledTaskcHandle = osThreadNew(ledTask, NULL, &ledTask_attributes);
	if (ledTaskcHandle == NULL) {
		printf("error led task creation\r\n");
		while (1)
			;
	}

}

void ledTask(void *argument) {
	/* USER CODE BEGIN 5 */
	/* Infinite loop */
	for (;;) {
		switch (connection_state) {
		case GSM:
			HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin);
			break;
		case WIFI:
			HAL_GPIO_TogglePin(LED_ORANGE_GPIO_Port, LED_ORANGE_Pin);
			break;
		}

		osDelay(100);
	}
	/* USER CODE END 5 */
}

#if DEBUG
#if WRITE_CHAR
	void printConsoleTask(void *argument){
		int8_t dataQueuePrint;

		for(;;){
			xQueueReceive(xQueuePrintConsole, &dataQueuePrint, portMAX_DELAY);
			//taskENTER_CRITICAL();
			printf("%c", dataQueuePrint);
			//taskEXIT_CRITICAL();
			vTaskDelay(1 / portTICK_PERIOD_MS);
		}
	}
	#else
void printConsoleTask(void *argument) {
	data_print_console_t dataQueuePrint;
	for (;;) {
		xQueueReceive(xQueuePrintConsole, &dataQueuePrint, portMAX_DELAY);
		xSemaphoreTake(mutex, 300 / portTICK_PERIOD_MS); //taskENTER_CRITICAL(); //
		printf("%s", dataQueuePrint.data_cmd);
		xSemaphoreGive(mutex); //taskEXIT_CRITICAL(); //
		vTaskDelay(1 / portTICK_PERIOD_MS);
	}
}
#endif
#endif

static int CheckFlag(ESP8266_StatusTypeDef status, int internalState) {
	if (status == ESP8266_OK) {
		// To the next state.
		internalState++;
		HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
	} else {
		if (status == ESP8266_ERROR)
			HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
	}
	return internalState;
}

//void connectWifiTask(void *argument) {
//	ESP8266_StatusTypeDef Status;
//	static int internalState = 0;
////	TickType_t t = xTaskGetTickCount();
////	uint8_t contador = 0;
//	HAL_UART_F_Init();
//	printf("Conectando a wifi, Espere por favor.. \r\n");
//	for (;;) {
//		switch (internalState) {
//		case 0:
//
//			Status = ESP_ConnectWifi(true, WIFI_AP_SSID, WIFI_AP_PASS);
//
//
//			internalState = CheckFlag(Status, internalState);
//
//			break;
//		case 1:
//			// Wait 1sec.
//			Status = ESP_Delay(1000);
//			//osDelay(1000 / portTICK_PERIOD_MS);
//			internalState++;
//
//			break;
//		case 2:
//			// Check the wifi connection status.
//			//Status = ESP_IsConnectedWifi();
//
//			//internalState = CheckFlag(Status, internalState);
//			internalState++;
//			break;
//		case 3:
//			// Start TCP connection.
//
//
//			Status = ESP_StartTCP((char*)host, port, keepalive, sslenable);
//
//			//osDelay(500 / portTICK_PERIOD_MS);
//			internalState = CheckFlag(Status, internalState);
//			break;
//		case 4:
//			// Send the mqtt data.
//
//
//			Status = mqtt_Connect();
//
//
//			internalState = CheckFlag(Status, internalState);
//			//osDelay(500 / portTICK_PERIOD_MS);
//
//			break;
//		case 5:
//
//
//			Status = mqtt_SubscriberPacket(sub_topic, strlen((char*)sub_topic));
//
//			if (Status == ESP8266_OK) {
//				xSemaphoreSub = xSemaphoreCreateBinary();
//				if (xSemaphoreSub != NULL) {
//					osThreadId_t res;
//					//Atributes defined for connect wifi task
//					osThreadAttr_t subscribeTask_attributes = { .name =
//							"connectWifi", .stack_mem = &subscribeTaskBuffer[0],
//							.stack_size = sizeof(subscribeTaskBuffer), .cb_mem =
//									&subscribeTaskControlBlock, .cb_size =
//									sizeof(subscribeTaskControlBlock),
//							.priority = (osPriority_t) osPriorityAboveNormal1, };
//
//					res = osThreadNew(subscribeTask, NULL,
//							&subscribeTask_attributes);
//					if (res == NULL) {
//						printf("error creacion de tarea\r\n");
//						while (1)
//							;
//					}
//
//					//Atributes defined for connect wifi task
//					osThreadAttr_t publishTask_attributes = { .name =
//							"connectWifi", .stack_mem = &publishTaskBuffer[0],
//							.stack_size = sizeof(publishTaskBuffer), .cb_mem =
//									&publishTaskControlBlock, .cb_size =
//									sizeof(publishTaskControlBlock), .priority =
//									(osPriority_t) osPriorityAboveNormal, };
//
//					res = osThreadNew(publishTask, NULL,
//							&publishTask_attributes);
//					if (res == NULL) {
//						printf("error creacion de tarea\r\n");
//						while (1)
//							;
//					}
//				}
//			}
//
//			internalState = CheckFlag(Status, internalState);
//
//			//Atributes defined for connect wifi task
//					osThreadAttr_t publishTask_attributes = { .name =
//							"connectWifi", .stack_mem = &publishTaskBuffer[0],
//							.stack_size = sizeof(publishTaskBuffer), .cb_mem =
//									&publishTaskControlBlock, .cb_size =
//									sizeof(publishTaskControlBlock), .priority =
//									(osPriority_t) osPriorityAboveNormal, };
//
//					res = osThreadNew(publishTask, NULL,
//							&publishTask_attributes);
//					if (res == NULL) {
//						printf("error creacion de tarea\r\n");
//						while (1)
//							;
//	//vTaskDelayUntil(&t, pdMS_TO_TICKS(5000));
//			osDelay(5000 / portTICK_PERIOD_MS);
//
//			break;
//		case 6:
////			xSemaphoreTake(xSemaphoreButton, portMAX_DELAY);
////				memset(pub_data, '\0', BUFFERSIZE_CMD);
////				sprintf((char*)pub_data, "Contador:%d%c%c", contador,'\r', '\n');
////				Status = mqtt_Publisher(pub_topic, pub_data, strlen((char*)pub_data));
////				//vTaskDelayUntil(&t, pdMS_TO_TICKS(2000));
////				contador++;
////				vTaskDelay(2000 / portTICK_PERIOD_MS);
//			break;
//
//		default:
//			break;
//		}
//		osDelay(1 / portTICK_PERIOD_MS);
//	}
//
//}

//void publishTask(void *argument) {
//
//	uint8_t contador = 0;
//	for (;;) {
//		//xSemaphoreTake(xSemaphoreButton, portMAX_DELAY);
//
//		memset(pub_data, '\0', BUFFERSIZE_CMD);
//		sprintf((char*) pub_data, "Contador: %d%c%c", contador, '\r', '\n');
//		xSemaphoreTake(xSemaphoreMutexUart, 20000);
//		mqtt_Publisher(pub_topic, pub_data, strlen((char*) pub_data));
//		xSemaphoreGive(xSemaphoreMutexUart);
//		contador++;
//		vTaskDelay(1000 / portTICK_PERIOD_MS);
//	}
//}
//
//void subscribeTask(void *argument) {
//	ESP8266_StatusTypeDef Status;
//	uint32_t RetLength = 0;
//
//	for (;;) {
//		xSemaphoreTake(xSemaphoreSub, portMAX_DELAY);
//		xSemaphoreTake(xSemaphoreMutexUart, 20000);
//		Status = mqtt_SubscriberReceive(sub_data, BUFFERSIZE_CMD, &RetLength); //dataSub.topic, dataSub.data, &dataSub.length);
//		xSemaphoreGive(xSemaphoreMutexUart);
//		if (Status == ESP8266_OK) {
//			if (RetLength != 0) {
//				sub_data[RetLength] = '\0';
//				xQueueSend(xQueuePrintConsole, &sub_data, portMAX_DELAY);
//			}
//		}
//		vTaskDelay(500 / portTICK_PERIOD_MS);
//	}
//}

void connectTask(void *argument) {
	//driver_uart_t * uart = (driver_uart_t *)argument;
	BG96_StatusTypeDef Status;
	int internalState = 1;
	sysctrl_status_t modem_status;
	printf("Conectando a red GSM, Espere por favor.. \r\n");
	static uint8_t indice = 0;

	TimerHandle_t timer_timeout_cmd;
	Timer_StatusTypedef cmd_timeout_status = TIMER_STOPPED;

	if (Timer_Create(&timer_timeout_cmd, 1000, pdFALSE, &cmd_timeout_status)
			== TIMER_ERROR) {
		printf("error creacion de timer\r\n");
		while (1)
			;
	}

	for (;;) {

		if (cmd_timeout_status == TIMER_TIMEOUT) {
			cmd_timeout_status = TIMER_STOPPED;
			internalState = 0;
			indice = 0;
		}
		//xSemaphoreTake(xSemaphoreButton, portMAX_DELAY);
		switch (internalState) {
		case 0:
			modem_status = SysCtrl_BG96_power_off();
			//xSemaphoreGive(mutex);
			if (modem_status == SCSTATUS_OK) {
				internalState++;
			}

			break;
		case 1:
			//xSemaphoreTake(mutex, portMAX_DELAY);
			modem_status = SysCtrl_BG96_power_on();
			//xSemaphoreGive(mutex);
			if (modem_status == SCSTATUS_OK) {
				internalState++;
				HAL_UART_F_Init(&huart2);
			} else {
				modem_status = SysCtrl_BG96_power_off();
			}

			break;
		case 2:
			//xSemaphoreTake(mutex, portMAX_DELAY);
			Status = BG96_ConnectTCP(BG96_COMMAND_INIT[indice].cmd,
					strlen((char*) BG96_COMMAND_INIT[indice].cmd),
					BG96_COMMAND_INIT[indice].response,
					BG96_COMMAND_INIT[indice].cmd_timeout);
			//xSemaphoreGive(mutex);
			osDelay(100 / portTICK_PERIOD_MS);
			if (Status == BG96_OK) {
				indice++;
			} else {
				if (Status == BG96_EOA) {
					internalState++;
					indice = 0;
				}
			}
			break;
		case 3:
			//xSemaphoreTake(mutex, portMAX_DELAY);
			modem_status = SysCtrl_BG96_sim_select(SC_MODEM_SIM_SOCKET_0); //SC_STM32_SIM_2//SC_MODEM_SIM_SOCKET_0//SC_MODEM_SIM_ESIM_1
			//xSemaphoreGive(mutex);
			(void) osDelay(100);  // waiting for 10ms after sim selection
			if (modem_status == SCSTATUS_OK) {
				internalState++;
			}
			break;
		case 4:
			//xSemaphoreTake(mutex, portMAX_DELAY);
			Status = BG96_ConnectTCP(BG96_COMMAND_1[indice].cmd,
					strlen((char*) BG96_COMMAND_1[indice].cmd),
					BG96_COMMAND_1[indice].response,
					BG96_COMMAND_1[indice].cmd_timeout);
			//xSemaphoreGive(mutex);
			osDelay(100 / portTICK_PERIOD_MS);
			if (Status == BG96_OK) {
				indice++;
				clean_Timer(&timer_timeout_cmd);
			} else {
				if (Status == BG96_EOA) {
					clean_Timer(&timer_timeout_cmd);
					internalState++;
					indice = 0;
					break;
				} else {  //if there is an error, timer startup
					osDelay(3000 / portTICK_PERIOD_MS);
					if (cmd_timeout_status == TIMER_STOPPED) {
						Timer_Change_Period(&timer_timeout_cmd,
								BG96_COMMAND_1[indice].fsm_timeout);
					}
				}
			}
			break;
		case 5:
			//xSemaphoreTake(mutex, portMAX_DELAY);
			Status = BG96_ConnectTCP(BG96_COMMAND_2[indice].cmd,
					strlen((char*) BG96_COMMAND_2[indice].cmd),
					BG96_COMMAND_2[indice].response,
					BG96_COMMAND_2[indice].cmd_timeout);
			//xSemaphoreGive(mutex);
			osDelay(200 / portTICK_PERIOD_MS);
			if (Status == BG96_OK) { //get an error because the firmware is not up today but this command is needed
				indice++;
			} else {
				if (Status == BG96_EOA) {
					internalState++;
					indice = 0;
					break;
				}
			}
			break;
		case 6:
			///xSemaphoreTake(mutex, portMAX_DELAY);
			Status = BG96_ConnectTCP(BG96_COMMAND_3[indice].cmd,
					strlen((char*) BG96_COMMAND_3[indice].cmd),
					BG96_COMMAND_3[indice].response,
					BG96_COMMAND_3[indice].cmd_timeout);
			//xSemaphoreGive(mutex);
			osDelay(100 / portTICK_PERIOD_MS);
			if (Status == BG96_OK) {
				indice++;
				clean_Timer(&timer_timeout_cmd);
			} else {
				if (Status == BG96_EOA) {
					clean_Timer(&timer_timeout_cmd);
					internalState++;
					indice = 0;
					break;
				} else {	//if there is an error, timer startup
					osDelay(3000 / portTICK_PERIOD_MS);
					if (cmd_timeout_status == TIMER_STOPPED) {
						Timer_Change_Period(&timer_timeout_cmd,
								BG96_COMMAND_3[indice].fsm_timeout);
					}
				}
			}
			break;
		case 7:
			//xSemaphoreTake(mutex, portMAX_DELAY);
			Status = BG96_MQTTOpen(host, port, tcpconnectID, vsn, cid,
					clean_session, keepalive, sslenable);
			//xSemaphoreGive(mutex);
			osDelay(3000 / portTICK_PERIOD_MS);
			if (Status == BG96_OK) {
				internalState++;
			} else {
				osDelay(1000 / portTICK_PERIOD_MS);
			}
			break;
		case 8:
			//xSemaphoreTake(mutex, portMAX_DELAY);
			Status = BG96_MQTTConnect(clientId, userName, password,
					tcpconnectID);
			//xSemaphoreGive(mutex);
			osDelay(3000 / portTICK_PERIOD_MS);
			if (Status == BG96_OK) {
				internalState++;
				//internalState++;
			} else {
				osDelay(1000 / portTICK_PERIOD_MS);
			}
			break;
		case 9:
			//xSemaphoreTake(mutex, portMAX_DELAY);
			Status = BG96_SubscribeTopic(tcpconnectID, 1, sub_topic, qos);
			//xSemaphoreGive(mutex);
			osDelay(5000 / portTICK_PERIOD_MS);
			if (Status == BG96_OK) {
				internalState++;
			} else {
				osDelay(1000 / portTICK_PERIOD_MS);
			}
			break;
		case 10:
			//publih task is created
			//Atributes defined for connect wifi task
			if (publishTaskcHandle == NULL) {
				osThreadAttr_t publishTask_attributes = { .name =

				"connectWifi", .stack_mem = &publishTaskBuffer[0], .stack_size =
						sizeof(publishTaskBuffer), .cb_mem =
						&publishTaskControlBlock, .cb_size =
						sizeof(publishTaskControlBlock), .priority =
						(osPriority_t) osPriorityAboveNormal, };

				publishTaskcHandle = osThreadNew(publishTask, NULL,
						&publishTask_attributes);
				if (publishTaskcHandle == NULL) {
					printf("error creacion de tarea\r\n");
					while (1)
						;
				}
			}
			//block till button is pressed
			xSemaphoreTake(xSemaphoreButton, portMAX_DELAY);
			//delete task
			osStatus_t thread_status;
			thread_status = osThreadTerminate(publishTaskcHandle);

			if (thread_status == osOK) {
				publishTaskcHandle = NULL;
				internalState++;           // Thread was terminated successfully
			} else {
				// Failed to terminate a thread
			}

			break;
		case 11:

			Status = BG96_UnsubscribeTopic(tcpconnectID, 1, sub_topic);
			osDelay(100 / portTICK_PERIOD_MS);
			if (Status == BG96_OK) {
				internalState++;
			} else {
				osDelay(1000 / portTICK_PERIOD_MS);
			}

			break;
		case 12:
			Status = BG96_Disconnect(tcpconnectID);
			osDelay(100 / portTICK_PERIOD_MS);
			if (Status == BG96_OK) {
				internalState++;
			} else {
				osDelay(1000 / portTICK_PERIOD_MS);
			}

			break;
		case 13:
			if (connectWifiTaskcHandle == NULL) {
				osThreadAttr_t connectWifi_attributes = { .name =

				"connectWifi", .stack_mem = &connectWifiTaskBuffer[0],
						.stack_size = sizeof(connectWifiTaskBuffer), .cb_mem =
								&connectWifiTaskControlBlock, .cb_size =
								sizeof(connectWifiTaskControlBlock), .priority =
								(osPriority_t) osPriorityAboveNormal, };

				connectWifiTaskcHandle = osThreadNew(connectWifiTask, NULL,
						&connectWifi_attributes);
				if (connectWifiTaskcHandle == NULL) {
					printf("error creacion de tarea\r\n");
					while (1)
						;
				}
				connection_state = WIFI;   // Thread was terminated successfully
			}

//			Status = BG96_Close(tcpconnectID);
//			osDelay(100 / portTICK_PERIOD_MS);
//			if (Status == BG96_OK) {
//						internalState++;
//			} else {
//				osDelay(1000 / portTICK_PERIOD_MS);
//			}

			break;
		default:
			break;
		}

		vTaskDelay(1 / portTICK_PERIOD_MS);

		if (connection_state == WIFI)	//terminate tas
			break;
	}
	internalState = 0;
	osStatus_t connectthread_status;
	connectthread_status = osThreadTerminate(connectTaskcHandle);
	if (connectthread_status == osOK) {
		HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);//turn off led to indicate desconnection
		HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);//turn off led to indicate desconnection
	}
}

void connectWifiTask(void *argument) {
	ESP8266_StatusTypeDef Status;
	static int internalState = 0;
	HAL_UART_F_Init(&huart6);
	printf("Conectando a wifi, Espere por favor.. \r\n");
	for (;;) {
		switch (internalState) {
		case 0:

			Status = ESP_ConnectWifi(true, WIFI_AP_SSID, WIFI_AP_PASS);

			internalState = CheckFlag(Status, internalState);

			break;
		case 1:
			// Wait 1sec.
			Status = ESP_Delay(1000);
			//osDelay(1000 / portTICK_PERIOD_MS);
			internalState++;

			break;
		case 2:
			// Check the wifi connection status.
			Status = ESP_IsConnectedWifi();

			//internalState = CheckFlag(Status, internalState);
			internalState++;
			break;
		case 3:
			// Start TCP connection.

			Status = ESP_StartTCP((char*) host, port, keepalive, sslenable);

			//osDelay(500 / portTICK_PERIOD_MS);
			internalState = CheckFlag(Status, internalState);
			break;
		case 4:
			// Send the mqtt data.

			Status = mqtt_Connect();

			internalState = CheckFlag(Status, internalState);
			//osDelay(500 / portTICK_PERIOD_MS);

			break;
		case 5:

			//ALGO NO LE GUSTA DEL SUBSCRIBE

			Status = mqtt_SubscriberPacket(sub_topic,
					strlen((char*) sub_topic));

			internalState = CheckFlag(Status, internalState);

			osDelay(3000 / portTICK_PERIOD_MS);
			//internalState++;
			break;
		case 6:
			if (publishTaskcHandle == NULL) {
				osThreadAttr_t publishTask_attributes = { .name = "publish",
						.stack_mem = &publishTaskBuffer[0], .stack_size =
								sizeof(publishTaskBuffer), .cb_mem =
								&publishTaskControlBlock, .cb_size =
								sizeof(publishTaskControlBlock), .priority =
								(osPriority_t) osPriorityAboveNormal, };

				publishTaskcHandle = osThreadNew(publishTask, NULL,
						&publishTask_attributes);
				if (publishTaskcHandle == NULL) {
					printf("error creacion de tarea\r\n");
					while (1)
						;
				}
			}
			xSemaphoreSub = xSemaphoreCreateBinary();
			if (xSemaphoreSub != NULL) {
				//Atributes defined for connect wifi task
				osThreadAttr_t subscribeTask_attributes = { .name =
						"subscribe", .stack_mem = &subscribeTaskBuffer[0],
						.stack_size = sizeof(subscribeTaskBuffer), .cb_mem =
								&subscribeTaskControlBlock, .cb_size =
										sizeof(subscribeTaskControlBlock),
										.priority = (osPriority_t) osPriorityAboveNormal1, };

				publishTaskcHandle = osThreadNew(subscribeTask, NULL,
						&subscribeTask_attributes);
				if (publishTaskcHandle == NULL) {
					printf("error creacion de tarea\r\n");
					while (1)
						;
				}
			}
			//block till button is pressed
			xSemaphoreTake(xSemaphoreButton, portMAX_DELAY);
			//delete task
			osStatus_t thread_status;
			thread_status = osThreadTerminate(publishTaskcHandle);

			if (thread_status == osOK) {
				publishTaskcHandle = NULL;
				internalState++;           // Thread was terminated successfully
			} else {
				// Failed to terminate a thread
			}
			break;
		case 7:

			//DESCONECTAR Y UNSUBSCRIBE

			break;

		default:
			break;
		}
		osDelay(1 / portTICK_PERIOD_MS);
		if (connection_state == GSM)	//terminate tas
			break;
	}
	internalState = 0;
	osStatus_t connectthread_status;
	connectthread_status = osThreadTerminate(connectTaskcHandle);
	if (connectthread_status == osOK) {
		HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);//turn off led to indicate desconnection
		HAL_GPIO_WritePin(LED_ORANGE_GPIO_Port, LED_ORANGE_Pin, GPIO_PIN_RESET);//turn off led to indicate desconnection
	}

}

void publishTask(void *argument) {
	BG96_StatusTypeDef Status;
	uint8_t contador = 0;
	HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
	for (;;) {
		memset(pub_data, '\0', BUFFERSIZE_CMD);
		sprintf((char*) pub_data, "Contador: %d%c%c%c", contador, CTRL(z), '\r',
				'\n');

		switch (connection_state) {
		case GSM:
			Status = BG96_PublishTopic(tcpconnectID, msgID, qos, retain,
					pub_topic, pub_data, strlen((char*) pub_data));
			break;
		case WIFI:
			xSemaphoreTake(xSemaphoreMutexUart, 20000);
			mqtt_Publisher(pub_topic, pub_data, strlen((char*) pub_data));
			xSemaphoreGive(xSemaphoreMutexUart);
			break;
		}
		contador++;
		osDelay(1000 / portTICK_PERIOD_MS);

	}
}

void subscribeTask(void *argument) {
	ESP8266_StatusTypeDef Status;
	uint32_t RetLength = 0;

	for (;;) {
		xSemaphoreTake(xSemaphoreSub, portMAX_DELAY);
		xSemaphoreTake(xSemaphoreMutexUart, 20000);
		Status = mqtt_SubscriberReceive(sub_data, BUFFERSIZE_CMD, &RetLength); //dataSub.topic, dataSub.data, &dataSub.length);
		xSemaphoreGive(xSemaphoreMutexUart);
		if (Status == ESP8266_OK) {
			if (RetLength != 0) {
				sub_data[RetLength] = '\0';
				xQueueSend(xQueuePrintConsole, &sub_data, portMAX_DELAY);
			}
		}
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}


static void clean_Timer(TimerHandle_t *timerT) {
	Timer_Stop(timerT);
}

void buttonsTask(void *argument) {
	fsmButtonInit(&button_down);
	for (;;) {
		//update FSM button
		fsmButtonUpdate(&button_down);

		if (button_down.released) {
			xSemaphoreGive(xSemaphoreButton);

		}

		vTaskDelay(1 / portTICK_PERIOD_MS);

	}
}
