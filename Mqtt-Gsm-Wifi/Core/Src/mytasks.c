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
#include "sysctrl_specific_f.h"
#include "debounce.h"
#include "timer_freertos.h"
#include "mqtt.h"

/* Private define ------------------------------------------------------------*/

#define CTRL(x) (#x[0]-'a'+1)

#define STACK_SIZE				512UL
#define CONSOLE_QUEUE_LENGTH	2
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
	uint8_t cmd[BUFFERSIZE_CMD];
	uint8_t response[BUFFERSIZE_CMD];
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

static uint8_t host[32] = "broker.hivemq.com";
static uint16_t port = 1883; ///< Remote port number.
static uint8_t clientId[] = "fedeID";
static uint8_t userName[] = "";
static uint8_t password[] = "";
static uint8_t tcpconnectID = 0;
static uint8_t vsn = 3;
static uint8_t cid = 1; //
static uint8_t clean_session = 0;
static uint16_t keepalive = 120; ///< Default keepalive time in seconds.
static uint8_t sslenable = 0; ///< SSL is disabled by default.

static const uint32_t msgID = 0;
static const uint8_t qos = 0;
static const uint8_t retain = 0;
static uint8_t pub_topic[32];// = "ME/ID0001RX/PC";//
static uint8_t sub_topic[32];// = "ME/ID0001TX/PC";//
static uint8_t pub_data[BUFFERSIZE_CMD];
static uint8_t sub_data[BUFFERSIZE_CMD];
uint8_t analizeBuffer[BUFFERSIZE_RESPONSE];
static uint8_t wifi_username[64];
static uint8_t wifi_password[64];



enum connection_t {
	WIFI = 0,
	GSM = 1,
	NONE = 2
};

uint8_t connection_state;		//=0 gsm,


QueueHandle_t xQueuePrintConsole;
QueueHandle_t xQeuePubData;
QueueHandle_t xQeueSubData;

SemaphoreHandle_t mutex;
button_t button_down;
//SemaphoreHandle_t xSemaphoreButton;
SemaphoreHandle_t xSemaphoreMutexUart;
SemaphoreHandle_t xSemaphorePublish;
SemaphoreHandle_t xSemaphoreGSM;
SemaphoreHandle_t xSemaphoreWIFI;
SemaphoreHandle_t xSemaphoreControl;
SemaphoreHandle_t xSemaphoreSearchWifi;


osThreadId_t printTaskHandle;
uint32_t printTaskBuffer[256];
StaticTask_t printTaskControlBlock;

//osThreadId_t buttonsTaskHandle;
//uint32_t buttonsTaskBuffer[128];
//StaticTask_t buttonsTaskControlBlock;

osThreadId_t connectGSMTaskcHandle;
uint32_t connectGSMTaskBuffer[512];
StaticTask_t connectGSMTaskControlBlock;

osThreadId_t connectWifiTaskcHandle;
uint32_t connectWifiTaskBuffer[256];
StaticTask_t connectWifiTaskControlBlock;

osThreadId_t subscribeTaskcHandle;
uint32_t subscribeTaskBuffer[256];
StaticTask_t subscribeTaskControlBlock;

osThreadId_t publishTaskcHandle;
uint32_t publishTaskBuffer[512];
StaticTask_t publishTaskControlBlock;

osThreadId_t ledTaskcHandle;
uint32_t ledTaskBuffer[128];
StaticTask_t ledTaskControlBlock;

osThreadId_t controlTaskcHandle;
uint32_t controlTaskBuffer[128];
StaticTask_t controlTaskControlBlock;

osThreadId_t searchWifiTaskcHandle;
uint32_t searchWifiTaskBuffer[128];
StaticTask_t searchWifiTaskControlBlock;

osThreadId_t keepAliveTaskcHandle;
uint32_t keepAliveTaskBuffer[256];
StaticTask_t keepAliveTaskControlBlock;

/**
 * @brief	Check flag and set error led, increment internal state if it is needed
 * @param	status			flag that indicate if led should be on or off
 * @param	internalstate	state on FSM
 * @return next state in FSM
 */
static int CheckFlag(ESP8266_StatusTypeDef status, int internalState);

/**
 * @brief	Clean timeout timer
 * @param	timerT	timer to be cleaned
 * @return void
 */
static void clean_Timer(TimerHandle_t *timerT);

/*
 * @brief Initialization tasks, semaphores and queues.
 * @param argument	task argument
 * @return void
 */
void initTasks() {

	connection_state = NONE;
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
	xSemaphorePublish = xSemaphoreCreateBinary();
	xSemaphoreGSM = xSemaphoreCreateCounting(2, 0);				//semaphore counting to activate gsm connection
	xSemaphoreWIFI = xSemaphoreCreateCounting(2, 0);			//semaphore counting to activate wifi connection
	xSemaphoreControl = xSemaphoreCreateCounting(5, 0);			//semaphore counting to get ME1040 commands
	xSemaphoreSearchWifi = xSemaphoreCreateBinary();			//semahore to search wifi access point
	xQeuePubData = xQueueCreate(50, sizeof(data_publish_t));
	xQeueSubData = xQueueCreate(50, sizeof(data_publish_t));

	if (xQueuePrintConsole == NULL || xQeuePubData == NULL|| xQeueSubData == NULL) {
		printf("error queue creation\r\n");
		while (1)
			;
	}

	if (mutex == NULL || xSemaphoreMutexUart == NULL) {
		printf("error semaphore creation\r\n");
		while (1)
			;
	}

	if (xSemaphorePublish == NULL	// || xSemaphoreButton == NULL
				|| xSemaphoreGSM == NULL || xSemaphoreWIFI == NULL
				|| xSemaphoreControl == NULL || xSemaphoreSearchWifi == NULL) {
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


	//Atributes defined for connect bg96 task
	osThreadAttr_t connectBG96Task_attributes = { .name = "connectBG96",
					.stack_mem = &connectGSMTaskBuffer[0], .stack_size =
							sizeof(connectGSMTaskBuffer), .cb_mem =
							&connectGSMTaskControlBlock, .cb_size =
							sizeof(connectGSMTaskControlBlock), .priority =
							(osPriority_t) osPriorityAboveNormal, };
		connectGSMTaskcHandle = osThreadNew(connectGSMTask, NULL,
				&connectBG96Task_attributes);
		if (connectGSMTaskcHandle == NULL) {
			printf("error connect task creation\r\n");
			while (1)
				;
		}

		//Atributes defined for connect bg96 task
		osThreadAttr_t connectWifiTask_attributes = { .name = "connectWifi",
				.stack_mem = &connectWifiTaskBuffer[0], .stack_size =
						sizeof(connectWifiTaskBuffer), .cb_mem =
								&connectWifiTaskControlBlock, .cb_size =
										sizeof(connectWifiTaskControlBlock), .priority =
												(osPriority_t) osPriorityAboveNormal, };
		connectWifiTaskcHandle = osThreadNew(connectWifiTask, NULL,
				&connectWifiTask_attributes);
		if (connectWifiTaskcHandle == NULL) {
			printf("error creacion de tarea\r\n");
			while (1)
				;
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

	//Atributes defined for control task
	osThreadAttr_t controlTask_attributes = { .name = "control", .stack_mem =
			&controlTaskBuffer[0], .stack_size = sizeof(controlTaskBuffer), .cb_mem =
					&controlTaskControlBlock, .cb_size = sizeof(controlTaskControlBlock),
				.priority = (osPriority_t) osPriorityAboveNormal1, };

		controlTaskcHandle = osThreadNew(controlTask, NULL, &controlTask_attributes);
		if (controlTaskcHandle == NULL) {
			printf("error control task creation\r\n");
			while (1)
				;
		}

		//Atributes defined for search wifi task
			osThreadAttr_t searchWifiTask_attributes = { .name = "searchWifi", .stack_mem =
					&searchWifiTaskBuffer[0], .stack_size = sizeof(searchWifiTaskBuffer), .cb_mem =
							&searchWifiTaskControlBlock, .cb_size = sizeof(searchWifiTaskControlBlock),
						.priority = (osPriority_t) osPriorityAboveNormal1, };

			searchWifiTaskcHandle = osThreadNew(searchWifiTask, NULL, &searchWifiTask_attributes);
				if (searchWifiTaskcHandle == NULL) {
					printf("error control task creation\r\n");
					while (1)
						;
				}
}

/*
 * @brief Search wifi available network.
 * @param argument	task argument
 * @return void
 */
void searchWifiTask(void *argument) {

	for (;;) {
		xSemaphoreTake(xSemaphoreSearchWifi, portMAX_DELAY);
		ESP_SearchWifi();
		osDelay(1);
	}
}

/*
 * @brief Receive command from UART from Microelect device and excecute desire task.
 * @param argument	task argument
 * @return void
 */
void controlTask(void *argument) {
	HAL_UART_F_Init(&huart1);
	memset((char*)analizeBuffer, '\0', sizeof(analizeBuffer));
	uint8_t states = 0;	//indicate that it should connect so ignore any if disconnect instruction ( need disconnect befor connecting another net)
	for (;;) {
		xSemaphoreTake(xSemaphoreControl, portMAX_DELAY);
		int i, j;
		j = 0;
		for (i = 0; i < (ControlBuffer.tail - ControlBuffer.head); i++){
			if(j < sizeof(analizeBuffer) && ControlBuffer.data[ControlBuffer.head + i] != '\0'){
				analizeBuffer[j++] = ControlBuffer.data[ControlBuffer.head + i];
			}
		}
		ControlBuffer.head = ControlBuffer.tail;
		if(strstr((char *)analizeBuffer, AT_MECWLAP_STRING) != NULL){
			xSemaphoreGive(xSemaphoreSearchWifi);
		}
		if(strstr((char *)analizeBuffer, AT_MECWJAP_STRING) != NULL){
			int i;
			uint8_t cont1 = 0;
			uint8_t length_aux = 0;
			//uint8_t length_pass = 0;
			uint32_t length = strlen((char *)analizeBuffer);
			for(i = 0; i < length; i++){
				if(analizeBuffer[i] == '"'){
					cont1++;
				}
				if(cont1 == 1){
					length_aux++;
				}
				if(cont1 == 2){//get wifi username
					strncpy((char*)wifi_username,  (char *)&analizeBuffer[i - length_aux + 1], (length_aux - 1));
					length_aux = 0;
					cont1++;
				}
				if(cont1 == 4){
					length_aux++;
				}
				if(cont1 == 5){//get wifi password
					strncpy((char*)wifi_password,  (char *)&analizeBuffer[i - length_aux + 1], (length_aux - 1));
					length_aux = 0;
					cont1++;
				}
				if(cont1 == 7){
					length_aux++;
				}
				if(cont1 == 8){//get publish topic
					strncpy((char*)pub_topic,  (char *)&analizeBuffer[i - length_aux + 1], (length_aux - 1));
					length_aux = 0;
					cont1++;
				}
				if(cont1 == 10){
					length_aux++;
				}
				if(cont1 == 11){//get subscribe topic
					strncpy((char*)sub_topic,  (char *)&analizeBuffer[i - length_aux + 1], (length_aux - 1));
					length_aux = 0;
					cont1++;
					break;
				}
			}
			connection_state = WIFI;
			states = 1;
			xSemaphoreGive(xSemaphoreWIFI);
		}
		else{
			if (states == 1 && strstr((char *)analizeBuffer, AT_MEDISCWIFI_STRING) != NULL){
				xSemaphoreGive(xSemaphoreWIFI);
				connection_state = NONE;
				states = 0;
			}
			else{
				if (states == 0 && strstr((char *)analizeBuffer, AT_MEGSM_STRING) != NULL){
					connection_state = GSM;
					states = 1;
					xSemaphoreGive(xSemaphoreGSM);
				}
				else{
					if (states == 1 && strstr((char *)analizeBuffer, AT_MEDISCGSM_STRING) != NULL){
						xSemaphoreGive(xSemaphoreGSM);
						connection_state = NONE;
						states = 0;
					}
				}
			}
		}
		xSemaphoreTake(mutex, 300 / portTICK_PERIOD_MS);
		HAL_UART_F_Send(&huart1, AT_OK_STRING, strlen((char*)AT_OK_STRING));
		xSemaphoreGive(mutex);
		memset((char*)analizeBuffer, '\0', sizeof(analizeBuffer));

		osDelay(1);
	}
}

/*
 * @brief Led status.
 * @param argument	task argument
 * @return void
 */
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
		case NONE:
			HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
			break;
		}

		osDelay(100);
	}
	/* USER CODE END 5 */
}

/*
 * @brief Print message to the console
 * @param argument	task argument
 * @return void
 */
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
#if ME_CONSOLE
		HAL_UART_F_Send(&huart1, (char*)&dataQueuePrint.data_cmd[0], strlen((char*)dataQueuePrint.data_cmd));
#else
		printf("%s", dataQueuePrint.data_cmd);
#endif
		xSemaphoreGive(mutex); //taskEXIT_CRITICAL(); //
		vTaskDelay(1 / portTICK_PERIOD_MS);
	}
}
#endif
#endif

/**
 * @brief	Check flag and set error led, increment internal state if it is needed
 * @param	status			flag that indicate if led should be on or off
 * @param	internalstate	state on FSM
 */
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

/*
 * @brief Connect to gsm network.
 * @param argument	task argument
 * @return void
 */
void connectGSMTask(void *argument) {

	BG96_StatusTypeDef Status;
	int internalState = 1;
	sysctrl_status_t modem_status;

	static uint8_t indice = 0;

	TimerHandle_t timer_timeout_cmd;
	Timer_StatusTypedef cmd_timeout_status = TIMER_STOPPED;

	if (Timer_Create(&timer_timeout_cmd, 1000, pdFALSE, &cmd_timeout_status)
			== TIMER_ERROR) {
		printf("error creacion de timer\r\n");
		while (1)
			;
	}
	xSemaphoreTake(xSemaphoreGSM, portMAX_DELAY);
	printf("Conectando a red GSM, Espere por favor.. \r\n");
	for (;;) {
		if (cmd_timeout_status == TIMER_TIMEOUT) {
			cmd_timeout_status = TIMER_STOPPED;
			internalState = 0;
			indice = 0;
		}
		switch (internalState) {
		case 0:
			modem_status = SysCtrl_BG96_power_off();
			if (modem_status == SCSTATUS_OK) {
				internalState++;
			}

			break;
		case 1:
			modem_status = SysCtrl_BG96_power_on();
			if (modem_status == SCSTATUS_OK) {
				internalState++;
				HAL_UART_F_Init(&huart2);
			} else {
				modem_status = SysCtrl_BG96_power_off();
			}

			break;
		case 2:
			Status = BG96_ConnectTCP(BG96_COMMAND_INIT[indice].cmd,
					strlen((char*) BG96_COMMAND_INIT[indice].cmd),
					BG96_COMMAND_INIT[indice].response,
					BG96_COMMAND_INIT[indice].cmd_timeout);
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
			modem_status = SysCtrl_BG96_sim_select(SC_MODEM_SIM_SOCKET_0);
			(void) osDelay(100);  // waiting for 10ms after sim selection
			if (modem_status == SCSTATUS_OK) {
				internalState++;
			}
			break;
		case 4:
			Status = BG96_ConnectTCP(BG96_COMMAND_1[indice].cmd,
					strlen((char*) BG96_COMMAND_1[indice].cmd),
					BG96_COMMAND_1[indice].response,
					BG96_COMMAND_1[indice].cmd_timeout);
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
			Status = BG96_ConnectTCP(BG96_COMMAND_2[indice].cmd,
					strlen((char*) BG96_COMMAND_2[indice].cmd),
					BG96_COMMAND_2[indice].response,
					BG96_COMMAND_2[indice].cmd_timeout);
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
			Status = BG96_ConnectTCP(BG96_COMMAND_3[indice].cmd,
					strlen((char*) BG96_COMMAND_3[indice].cmd),
					BG96_COMMAND_3[indice].response,
					BG96_COMMAND_3[indice].cmd_timeout);
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
			Status = BG96_MQTTOpen(host, port, tcpconnectID, vsn, cid,
					clean_session, keepalive, sslenable);
			osDelay(3000 / portTICK_PERIOD_MS);
			if (Status == BG96_OK) {
				internalState++;
			} else {
				osDelay(1000 / portTICK_PERIOD_MS);
			}
			break;
		case 8:
			Status = BG96_MQTTConnect(clientId, userName, password,
					tcpconnectID);
			osDelay(3000 / portTICK_PERIOD_MS);
			if (Status == BG96_OK) {
				internalState++;
			} else {
				osDelay(1000 / portTICK_PERIOD_MS);
			}
			break;
		case 9:
			Status = BG96_SubscribeTopic(tcpconnectID, 1, sub_topic, qos);
			osDelay(5000 / portTICK_PERIOD_MS);
			if (Status == BG96_OK) {
				internalState++;
			} else {
				osDelay(1000 / portTICK_PERIOD_MS);
			}
			break;
		case 10:
			//publih task is created
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
			xSemaphoreTake(mutex, 300 / portTICK_PERIOD_MS);
			HAL_UART_F_Send(&huart1, AT_MECONNOK_STRING, strlen((char*)AT_MECONNOK_STRING));
			xSemaphoreGive(mutex);
			xSemaphoreTake(xSemaphoreGSM, portMAX_DELAY);
			//delete task
			osStatus_t thread_status;
			thread_status = osThreadTerminate(publishTaskcHandle);

			if (thread_status == osOK) {
				publishTaskcHandle = NULL;
				internalState++;
			}
			break;
		case 11:
			internalState++;
			Status = BG96_UnsubscribeTopic(tcpconnectID, 1, sub_topic);
			osDelay(100 / portTICK_PERIOD_MS);
			if (Status == BG96_OK) {
				internalState++;
			} else {
				osDelay(1000 / portTICK_PERIOD_MS);
			}
			break;
		case 12:
			internalState++;
			Status = BG96_Disconnect(tcpconnectID);
			osDelay(100 / portTICK_PERIOD_MS);
			if (Status == BG96_OK) {
				internalState++;
			} else {
				osDelay(1000 / portTICK_PERIOD_MS);
			}
			break;
		case 13:
			internalState = 0;
			HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);
			xSemaphoreTake(xSemaphoreGSM, portMAX_DELAY);
			printf("Conectando a red GSM, Espere por favor.. \r\n");
			break;
		default:
			break;
		}
		vTaskDelay(1 / portTICK_PERIOD_MS);

	}
}

/*
 * @brief Connect to wifi network.
 * @param argument	task argument
 * @return void
 */
void connectWifiTask(void *argument) {
	ESP8266_StatusTypeDef Status;
	static int internalState = 0;
	HAL_UART_F_Init(&huart6);
	xSemaphoreTake(xSemaphoreWIFI, portMAX_DELAY);
	printf("Conectando a wifi, Espere por favor.. \r\n");
	for (;;) {

		switch (internalState) {
		case 0:
			Status = ESP_ConnectWifi(true, (char*)wifi_username, (char*)wifi_password);
			internalState = CheckFlag(Status, internalState);
			break;
		case 1:
			// Wait 1sec.
			Status = ESP_Delay(1000);
			internalState++;
			break;
		case 2:
			// Check the wifi connection status.
			Status = ESP_IsConnectedWifi();
			internalState++;
			break;
		case 3:
			// Start TCP connection.
			Status = ESP_StartTCP((char*) host, port, keepalive, sslenable);
			internalState = CheckFlag(Status, internalState);
			break;
		case 4:
			// Send the mqtt data.
			Status = mqtt_Connect(clientId, userName, password);
			internalState = CheckFlag(Status, internalState);
			break;
		case 5:
			Status = mqtt_SubscriberPacket(sub_topic,
					strlen((char*) sub_topic));
			internalState = CheckFlag(Status, internalState);
			osDelay(3000 / portTICK_PERIOD_MS);
			break;
		case 6:
			/* Create Publish Task*/
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

			/* Create keepAlive Task*/
			if (keepAliveTaskcHandle == NULL) {
				osThreadAttr_t keepAliveTask_attributes = { .name = "keepAlive",
						.stack_mem = &keepAliveTaskBuffer[0], .stack_size =
								sizeof(keepAliveTaskBuffer), .cb_mem =
										&keepAliveTaskControlBlock, .cb_size =
												sizeof(keepAliveTaskControlBlock), .priority =
														(osPriority_t) osPriorityNormal, };

				keepAliveTaskcHandle = osThreadNew(keepAliveTask, NULL,
						&keepAliveTask_attributes);
				if (keepAliveTaskcHandle == NULL) {
					printf("error creacion de tarea\r\n");
					while (1)
						;
				}
			}

			/* Create Subscribe Task*/
			if (xQeueSubData != NULL) {
				osThreadAttr_t subscribeTask_attributes = { .name =
						"subscribe", .stack_mem = &subscribeTaskBuffer[0],
						.stack_size = sizeof(subscribeTaskBuffer), .cb_mem =
								&subscribeTaskControlBlock, .cb_size =
										sizeof(subscribeTaskControlBlock),
										.priority = (osPriority_t) osPriorityAboveNormal1, };

				subscribeTaskcHandle = osThreadNew(subscribeTask, NULL,
						&subscribeTask_attributes);
				if (subscribeTaskcHandle == NULL) {
					printf("error creacion de tarea\r\n");
					while (1)
						;
				}
			}
			osDelay(3000 / portTICK_PERIOD_MS);
			internalState++;
			break;
		case 7:
			xSemaphoreTake(mutex, 300 / portTICK_PERIOD_MS);
			HAL_UART_F_Send(&huart1, AT_MECONNOK_STRING, strlen((char*)AT_MECONNOK_STRING));
			xSemaphoreGive(mutex);

			xSemaphoreTake(xSemaphoreWIFI, portMAX_DELAY);
			//delete task
			osStatus_t thread_status_pub, thread_status_sub, thread_status_keepAlive;
			thread_status_pub = osThreadTerminate(publishTaskcHandle);
			thread_status_sub = osThreadTerminate(subscribeTaskcHandle);
			thread_status_keepAlive = osThreadTerminate(keepAliveTaskcHandle);

			if (thread_status_pub == osOK && thread_status_sub == osOK && thread_status_keepAlive == osOK) {
				publishTaskcHandle = NULL;
				subscribeTaskcHandle = NULL;
				keepAliveTaskcHandle = NULL;
				internalState++;           // Thread was terminated successfully
			}
			break;
		case 8:
			internalState = 0;
			HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);//turn off led to indicate desconnection
			HAL_GPIO_WritePin(LED_ORANGE_GPIO_Port, LED_ORANGE_Pin, GPIO_PIN_RESET);//turn off led to indicate desconnection
			xSemaphoreTake(xSemaphoreWIFI, portMAX_DELAY);
			printf("Conectando a wifi, Espere por favor.. \r\n");

			//DESCONECTAR Y UNSUBSCRIBE

			break;

		default:
			break;
		}
		osDelay(1 / portTICK_PERIOD_MS);
	}
}

/*
 * @brief Publish message to the mqtt topic.
 * @param argument	task argument
 * @return void
 */
void publishTask(void *argument) {
	BG96_StatusTypeDef Status;
	ESP8266_StatusTypeDef Status_Wifi;
	data_publish_t dataPub;
	uint8_t contador = 0;

	HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
	for (;;) {
		xQueueReceive(xQeuePubData, &dataPub, portMAX_DELAY);

		switch (connection_state) {
		case GSM:

			//AGREGAR EL CTRL(z) PASCUAL

			dataPub.data[dataPub.length] = CTRL(z);
			sprintf((char*) pub_data, "Contador: %d%c", contador, CTRL(z));
			Status = BG96_PublishTopic(tcpconnectID, msgID, qos, retain,
					pub_topic, dataPub.data, strlen((char*)dataPub.data));
			break;
		case WIFI:
			xSemaphoreTake(xSemaphoreMutexUart, 20000);
			Status_Wifi = mqtt_Publisher(pub_topic, dataPub.data, dataPub.length);
			xSemaphoreGive(xSemaphoreMutexUart);
			break;
		}
		contador++;
		osDelay(10 / portTICK_PERIOD_MS);
	}
}

/*
 * @brief Send ping packet to the broker mqtt to keep alive connection.
 * @param argument	task argument
 * @return void
 */
void keepAliveTask(void *argument) {
	ESP8266_StatusTypeDef Status_Wifi;
	for (;;) {
		Status_Wifi = mqtt_keepAlive();
		vTaskDelay(10000 / portTICK_PERIOD_MS);
	}
}

/*
 * @brief Receive message to the mqtt topic and retransmit it to the ME1040.
 * @param argument	task argument
 * @return void
 */
void subscribeTask(void *argument) {
	data_publish_t dataSub;
	for (;;) {
		memset((char*)dataSub.data, '\0', sizeof(dataSub.data));
		xQueueReceive(xQeueSubData, &dataSub, portMAX_DELAY);

		xSemaphoreTake(mutex, 300 / portTICK_PERIOD_MS);
		HAL_UART_F_Send(&huart1, (char*)&dataSub.data[0], dataSub.length);
		osDelay(100 / portTICK_PERIOD_MS);
		HAL_UART_F_Send(&huart1, (char*)&dataSub.data[0], dataSub.length);
		osDelay(100 / portTICK_PERIOD_MS);
		HAL_UART_F_Send(&huart1, (char*)&dataSub.data[0], dataSub.length);
		xSemaphoreGive(mutex);
		vTaskDelay(250 / portTICK_PERIOD_MS);
	}
}


static void clean_Timer(TimerHandle_t *timerT) {
	Timer_Stop(timerT);
}


