/*
 * mytasks.c
 *
 *  Created on: Jul 20, 2020
 *      Author: fedepacher
 */

#include "mytasks.h"
#include "connect_Client.h"
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

typedef struct{
	uint8_t cmd[CMD_LENGTH];
	uint32_t cmd_timeout;
	uint32_t fsm_timeout;
}cmd_type_t;

static const cmd_type_t BG96_COMMAND_INIT [] =
{
		{AT_IFC_A,	 				CMD_TIMEOUT_300, 		FSM_TIMEOUT_100},
		{AT_CMD, 					CMD_TIMEOUT_300, 		FSM_TIMEOUT_100},
		{ATE1_A, 					CMD_TIMEOUT_300, 		FSM_TIMEOUT_100},
		{AT_CMEE_A, 				CMD_TIMEOUT_300, 		FSM_TIMEOUT_100},
		{ATV1_A, 					CMD_TIMEOUT_300, 		FSM_TIMEOUT_100},
		{AT_AND_D0_A,	 			CMD_TIMEOUT_300, 		FSM_TIMEOUT_100},
		{AT_CGMR_CMD, 				CMD_TIMEOUT_300, 		FSM_TIMEOUT_100},
		{AT_CFUN_0_A, 				CMD_TIMEOUT_15000, 		FSM_TIMEOUT_100},
		{AT_QCFG_BAND_A, 			CMD_TIMEOUT_300, 		FSM_TIMEOUT_100},
		{AT_QCFG_IOT_A, 			CMD_TIMEOUT_300, 		FSM_TIMEOUT_100},
		{AT_QCFG_NWSCANSEQ_A, 		CMD_TIMEOUT_300, 		FSM_TIMEOUT_100},
		{AT_QCFG_NWSCANMODE_A,		CMD_TIMEOUT_300, 		FSM_TIMEOUT_100},

		{END_OF_ARRAY,				CMD_TIMEOUT_300, 		FSM_TIMEOUT_100},
};

static const cmd_type_t BG96_COMMAND_1 [] =
{
		{AT_QICSGP1_A, 				CMD_TIMEOUT_5000, 		FSM_TIMEOUT_20000},
		{AT_QICSGP2_A, 				CMD_TIMEOUT_5000, 		FSM_TIMEOUT_20000},
		{AT_CFUN_1_A,	 			CMD_TIMEOUT_15000, 		FSM_TIMEOUT_20000},
		{AT_QCCID_CMD, 				CMD_TIMEOUT_300, 		FSM_TIMEOUT_20000},

		{END_OF_ARRAY,				CMD_TIMEOUT_300, 		FSM_TIMEOUT_100},
};

static const cmd_type_t BG96_COMMAND_2 [] =
{

		{AT_QINITSTAT_CMD,			CMD_TIMEOUT_300, 		FSM_TIMEOUT_20000},
		{AT_QINITSTAT_CMD,			CMD_TIMEOUT_300, 		FSM_TIMEOUT_20000},
		{AT_QINITSTAT_CMD,			CMD_TIMEOUT_300, 		FSM_TIMEOUT_20000},

		{END_OF_ARRAY,				CMD_TIMEOUT_300, 		FSM_TIMEOUT_100},
};


static const cmd_type_t BG96_COMMAND_3 [] =
{
		{AT_CPIN_Q, 				CMD_TIMEOUT_5000, 		FSM_TIMEOUT_20000},
		{AT_QGMR_CMD, 				CMD_TIMEOUT_300, 		FSM_TIMEOUT_60000},
		{AT_CPIN_Q, 				CMD_TIMEOUT_5000, 		FSM_TIMEOUT_20000},
		{AT_CGDCONT_Q,				CMD_TIMEOUT_300, 		FSM_TIMEOUT_20000},
		{AT_CREG_A,					CMD_TIMEOUT_300, 		FSM_TIMEOUT_90000},
		{AT_CGREG_A, 				CMD_TIMEOUT_300, 		FSM_TIMEOUT_60000},
		{AT_CEREG_A, 				CMD_TIMEOUT_300, 		FSM_TIMEOUT_60000},
		{AT_GSN_CMD, 				CMD_TIMEOUT_300, 		FSM_TIMEOUT_60000},
		{AT_CGMI_CMD, 				CMD_TIMEOUT_300, 		FSM_TIMEOUT_60000},
		{AT_CGMM_CMD, 				CMD_TIMEOUT_300, 		FSM_TIMEOUT_60000},
		{AT_QGMR_CMD, 				CMD_TIMEOUT_300, 		FSM_TIMEOUT_60000},
		{AT_CGSN_CMD, 				CMD_TIMEOUT_300, 		FSM_TIMEOUT_60000},
		{AT_QCCID_CMD, 				CMD_TIMEOUT_300, 		FSM_TIMEOUT_20000},
		{AT_CIMI_CMD, 				CMD_TIMEOUT_300, 		FSM_TIMEOUT_20000},
		{AT_COPS_Q, 				CMD_TIMEOUT_180000,		FSM_TIMEOUT_20000},
		{AT_COPS_A, 				CMD_TIMEOUT_180000,		FSM_TIMEOUT_20000},
		{AT_CEREG_Q, 				CMD_TIMEOUT_300, 		FSM_TIMEOUT_60000},
		{AT_CREG_Q,					CMD_TIMEOUT_300, 		FSM_TIMEOUT_90000},
		{AT_CGREG_Q, 				CMD_TIMEOUT_300, 		FSM_TIMEOUT_60000},
		{AT_CGATT_A, 				CMD_TIMEOUT_140000,		FSM_TIMEOUT_60000},
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
		{AT_CGEREP_A, 				CMD_TIMEOUT_300,		FSM_TIMEOUT_60000},
		{AT_QIACT_Q, 				CMD_TIMEOUT_150000,		FSM_TIMEOUT_150000},
		{AT_QIACT_A, 				CMD_TIMEOUT_150000,		FSM_TIMEOUT_150000},
		{AT_QIACT_Q, 				CMD_TIMEOUT_150000,		FSM_TIMEOUT_150000},
		//{AT_QIACT_Q, 				CMD_TIMEOUT_150000,		FSM_TIMEOUT_60000},
		{AT_CSQ_CMD, 				CMD_TIMEOUT_300,		FSM_TIMEOUT_60000},
		{AT_QCSQ_CMD, 				CMD_TIMEOUT_300,		FSM_TIMEOUT_60000},
		{AT_QNWINFO_CMD, 			CMD_TIMEOUT_300,		FSM_TIMEOUT_60000},
		/*{AT_CSQ_CMD, 				CMD_TIMEOUT_300,		FSM_TIMEOUT_60000},
		{AT_QCSQ_CMD, 				CMD_TIMEOUT_5000,		FSM_TIMEOUT_60000},
		{AT_QNWINFO_CMD, 			CMD_TIMEOUT_300,		FSM_TIMEOUT_60000},*/

		{END_OF_ARRAY,				CMD_TIMEOUT_300, 		FSM_TIMEOUT_100},
};


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


static uint8_t host[32] = "broker.hivemq.com"; ///< HostName i.e. "test.mosquitto.org"//"broker.mqttdashboard.com";//
static uint16_t port = 1883; ///< Remote port number.
static uint8_t clientId[] = "fedeID";
static uint8_t userName[] = "";
static uint8_t password[] = "";
static uint8_t tcpconnectID = 0;
static uint8_t vsn = 3;
static uint8_t cid = 1;
static uint8_t clean_session = 0;
static uint16_t keepalive = 120; ///< Default keepalive time in seconds.
static uint8_t sslenable = 0; ///< SSL is disabled by default.

static const uint32_t msgID = 0;
static const uint8_t qos = 0;
static const uint8_t retain = 0;
static uint8_t pub_topic[32] = "topic/pub";//"test/pdm\0";
static uint8_t pub_data[BUFFERSIZE_CMD];



QueueHandle_t xQueuePrintConsole;
SemaphoreHandle_t mutex;
button_t button_down;
SemaphoreHandle_t xSemaphoreButton;

osThreadId_t printTaskHandle;
uint32_t printTaskBuffer[ 512 ];
StaticTask_t printTaskControlBlock;

osThreadId_t buttonsTaskHandle;
uint32_t buttonsTaskBuffer[ 512 ];
StaticTask_t buttonsTaskControlBlock;

osThreadId_t connectTaskHandle;
uint32_t connectTaskBuffer[ 512 ];
StaticTask_t connectTaskControlBlock;
//osThreadId_t Handle;
//uint32_t _calcAvgBuffer[ 512 ];
////osStaticThreadDef_t _calcAvgControlBlock;
//const osThreadAttr_t _calcAvg_attributes = {
//  .name = "_calcAvg",
//  .stack_mem = &_calcAvgBuffer[0],
//  .stack_size = sizeof(_calcAvgBuffer),
//  .cb_mem = &_calcAvgControlBlock,
//  .cb_size = sizeof(_calcAvgControlBlock),
//  .priority = (osPriority_t) osPriorityNormal,
//};


static void clean_Timer(TimerHandle_t *timerT);


void initTasks(){


	//button set
	button_down.GPIOx = BTN_GPIO_Port;
	button_down.GPIO_Pin = BTN_Pin;

	//HAL_UART_F_Init();
#if WRITE_CHAR
	xQueuePrintConsole = xQueueCreate(100, sizeof(uint8_t));
#else
	xQueuePrintConsole = xQueueCreate(CONSOLE_QUEUE_LENGTH, sizeof(data_print_console_t));
#endif

	mutex = xSemaphoreCreateMutex();
	xSemaphoreButton = xSemaphoreCreateCounting(10, 0);


	if (xQueuePrintConsole == NULL){
		printf("error creacion de cola\r\n");
		while(1);
	}

	if (mutex == NULL){
		printf("error creacion de cola\r\n");
		while(1);
	}

	if (xSemaphoreButton == NULL){
		printf("error creacion de semaforo botones\r\n");
		while(1);
	}

	osThreadAttr_t printConsoleTask_attributes = {
	    .name = "printConsole",
	    .stack_mem = &printTaskBuffer[0],
	    .stack_size = sizeof(printTaskBuffer),
	    .cb_mem = &printTaskControlBlock,
	    .cb_size = sizeof(printTaskControlBlock),
	    .priority = (osPriority_t) osPriorityAboveNormal,
	  };

	osThreadAttr_t buttonsTask_attributes = {
		    .name = "buttons",
		    .stack_mem = &buttonsTaskBuffer[0],
		    .stack_size = sizeof(buttonsTaskBuffer),
		    .cb_mem = &buttonsTaskControlBlock,
		    .cb_size = sizeof(buttonsTaskControlBlock),
		    .priority = (osPriority_t) osPriorityAboveNormal,
		  };

	osThreadAttr_t connectTask_attributes = {
			    .name = "connect",
			    .stack_mem = &connectTaskBuffer[0],
			    .stack_size = sizeof(connectTaskBuffer),
			    .cb_mem = &connectTaskControlBlock,
			    .cb_size = sizeof(connectTaskControlBlock),
			    .priority = (osPriority_t) osPriorityAboveNormal,
			  };

	osThreadId_t res = osThreadNew(printConsoleTask, NULL, &printConsoleTask_attributes);
	if (res == NULL) {
		printf("error creacion de tarea\r\n");
		while(1);
	}


	res = osThreadNew(connectTask,  NULL, &connectTask_attributes);
	if (res == NULL) {
		printf("error creacion de tarea connect\r\n");
		while(1);
	}
	/*
	res = osThreadNew(buttonsTask, NULL, &buttonsTask_attributes);
	if (res == NULL) {
		printf("error creacion de tarea buttons\r\n");
	}
*/


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
	void printConsoleTask(void *argument){
		data_print_console_t dataQueuePrint;
		for(;;){
			xQueueReceive(xQueuePrintConsole, &dataQueuePrint, portMAX_DELAY);
			xSemaphoreTake(mutex, 300 / portTICK_PERIOD_MS); //taskENTER_CRITICAL(); //
			printf("%s", dataQueuePrint.data_cmd);
			xSemaphoreGive(mutex); //taskEXIT_CRITICAL(); //
			vTaskDelay(1 / portTICK_PERIOD_MS);
		}
	}
	#endif
#endif


void connectTask(void *argument){
	//driver_uart_t * uart = (driver_uart_t *)argument;
	ESP8266_StatusTypeDef Status;
	int internalState = 1;
	sysctrl_status_t modem_status;
	printf("Conectando a red, Espere por favor.. \r\n");
	static uint8_t indice = 0;
	static uint8_t contador = 0;
	TimerHandle_t timer_timeout_cmd;
	Timer_StatusTypedef cmd_timeout_status = TIMER_STOPPED;

	if(Timer_Create(&timer_timeout_cmd, 1000, pdFALSE, &cmd_timeout_status) == TIMER_ERROR){
		printf("error creacion de timer\r\n");
		while(1);
	}

	for (;;) {

		if(cmd_timeout_status == TIMER_TIMEOUT){
			cmd_timeout_status = TIMER_STOPPED;
			internalState = 0;
			indice = 0;
		}
		//xSemaphoreTake(xSemaphoreButton, portMAX_DELAY);
		switch (internalState) {
		case 0:
			modem_status = SysCtrl_BG96_power_off();
			//xSemaphoreGive(mutex);
			if(modem_status == SCSTATUS_OK){
				internalState++;
			}

		break;
		case 1:
			//xSemaphoreTake(mutex, portMAX_DELAY);
			modem_status = SysCtrl_BG96_power_on();
			//xSemaphoreGive(mutex);
			if(modem_status == SCSTATUS_OK){
				internalState++;
				HAL_UART_F_Init();
			}
			else{
				modem_status = SysCtrl_BG96_power_off();
			}

			break;
		case 2:
			//xSemaphoreTake(mutex, portMAX_DELAY);
			Status = ConnectTCP(BG96_COMMAND_INIT[indice].cmd, strlen((char*)BG96_COMMAND_INIT[indice].cmd), BG96_COMMAND_INIT[indice].cmd_timeout);
			//xSemaphoreGive(mutex);
			osDelay(100 / portTICK_PERIOD_MS);
			if (Status == ESP8266_OK) {
				indice++;
			}
			else{
				if (Status == ESP8266_EOA){
					internalState++;
					indice = 0;
				}
			}
			break;
		case 3:
			//xSemaphoreTake(mutex, portMAX_DELAY);
			modem_status = SysCtrl_BG96_sim_select(SC_MODEM_SIM_SOCKET_0);//SC_STM32_SIM_2//SC_MODEM_SIM_SOCKET_0//SC_MODEM_SIM_ESIM_1
			//xSemaphoreGive(mutex);
			(void)osDelay(100);  // waiting for 10ms after sim selection
			if(modem_status == SCSTATUS_OK){
				internalState++;
			}
			break;
		case 4:
			//xSemaphoreTake(mutex, portMAX_DELAY);
			Status = ConnectTCP(BG96_COMMAND_1[indice].cmd, strlen((char*)BG96_COMMAND_1[indice].cmd), BG96_COMMAND_1[indice].cmd_timeout);
			//xSemaphoreGive(mutex);
			osDelay(100 / portTICK_PERIOD_MS);
			if (Status == ESP8266_OK) {
				indice++;
				clean_Timer(&timer_timeout_cmd);
			}
			else{
				if(Status == ESP8266_EOA){
					clean_Timer(&timer_timeout_cmd);
					internalState++;
					indice = 0;
					break;
				}
				else{//if there is an error, timer startup
					osDelay(3000 / portTICK_PERIOD_MS);
					if(cmd_timeout_status == TIMER_STOPPED){
						Timer_Change_Period(&timer_timeout_cmd, BG96_COMMAND_1[indice].fsm_timeout);
					}
				}
			}
			break;
		case 5:
			//xSemaphoreTake(mutex, portMAX_DELAY);
			Status = ConnectTCP(BG96_COMMAND_2[indice].cmd, strlen((char*)BG96_COMMAND_2[indice].cmd), BG96_COMMAND_2[indice].cmd_timeout);
			//xSemaphoreGive(mutex);
			osDelay(200 / portTICK_PERIOD_MS);
			if (Status == ESP8266_ERROR) {	//get an error because the firmware is not up today but this command is needed
				indice++;
			}
			else{
				if(Status == ESP8266_EOA){
					internalState++;
					indice = 0;
					break;
				}
			}
			break;
		case 6:
			///xSemaphoreTake(mutex, portMAX_DELAY);
			Status = ConnectTCP(BG96_COMMAND_3[indice].cmd, strlen((char*)BG96_COMMAND_3[indice].cmd), BG96_COMMAND_3[indice].cmd_timeout);
			//xSemaphoreGive(mutex);
			osDelay(100 / portTICK_PERIOD_MS);
			if (Status == ESP8266_OK) {
				indice++;
				clean_Timer(&timer_timeout_cmd);
			}
			else{
				if(Status == ESP8266_EOA){
					clean_Timer(&timer_timeout_cmd);
					internalState++;
					indice = 0;
					break;
				}
				else{//if there is an error, timer startup
					osDelay(3000 / portTICK_PERIOD_MS);
					if(cmd_timeout_status == TIMER_STOPPED){
						Timer_Change_Period(&timer_timeout_cmd, BG96_COMMAND_3[indice].fsm_timeout);
					}
				}
			}
			break;
		case 7:
			//xSemaphoreTake(mutex, portMAX_DELAY);
			Status = MQTTOpen(host, port, tcpconnectID, vsn, cid, clean_session, keepalive, sslenable);
			//xSemaphoreGive(mutex);
			osDelay(3000 / portTICK_PERIOD_MS);
			if (Status == ESP8266_OK) {
				internalState++;
			}
			else{
				osDelay(1000 / portTICK_PERIOD_MS);
			}
			break;
		case 8:
			//xSemaphoreTake(mutex, portMAX_DELAY);
			Status = MQTTConnect(clientId, userName, password, tcpconnectID);
			//xSemaphoreGive(mutex);
			osDelay(1000 / portTICK_PERIOD_MS);
			if (Status == ESP8266_OK) {
				internalState++;
			}
			else{
				osDelay(1000 / portTICK_PERIOD_MS);
			}
			break;
		case 9:
			//xSemaphoreTake(xSemaphoreButton, portMAX_DELAY);
			//xSemaphoreTake(mutex, portMAX_DELAY);
			memset(pub_data, '\0', BUFFERSIZE_CMD);
			sprintf((char*)pub_data, "Contador: %d%c%c%c", contador, CTRL(z),'\r', '\n');
			Status = PubData(tcpconnectID, msgID, qos, retain, pub_topic, pub_data, strlen((char*) pub_data));
			//xSemaphoreGive(mutex);
			osDelay(2000 / portTICK_PERIOD_MS);
			contador++;
			break;
		default:
			break;
		}

		vTaskDelay(1 / portTICK_PERIOD_MS);
	}
}

static void clean_Timer(TimerHandle_t *timerT){
	Timer_Stop(timerT);
}

void buttonsTask(void *argument){
	fsmButtonInit(&button_down);
	for (;;) {
		//update FSM button
		fsmButtonUpdate(&button_down);

		if(button_down.released){
			xSemaphoreGive(xSemaphoreButton);

			//Timer_Change_Period(&timer_timeout_cmd, 1000);
		}



		/*if (cmd_timeout_status == ESP8266_TIMEOUT){
			cmd_timeout_status = ESP8266_OK;
			HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);

		}*/

		vTaskDelay(1 / portTICK_PERIOD_MS);

	}
}
