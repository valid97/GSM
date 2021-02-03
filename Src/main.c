/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "string.h"

/* Private includes ----------------------------------------------------------*/
#include <driver_console.h>
#include <gsm.h>
#include <time.h>
#include <mqtt.h>

#include "FreeRTOS.h"
#include "task.h"


/* receving buffer for console */
uint8_t rxbufferConsole[2000];
/* transmiting buffer for console */
uint8_t txbufferConsole[2000];

uint8_t bufferConsole[2000] = {0};

/* Buffer for receiving characters from gsm in uart interrupt routine */
uint8_t rxBufferGsm[2000];

uint8_t bufferGsm[2000] = {0};

uint32_t size = 0;
uint32_t sizeGsm = 0;


/* Private variables -------------------------------------------------------------*/

/* Define handlers */
TIM_HandleTypeDef 		htim6;				/* Timer 6 for time menagement 		*/
UART_HandleTypeDef 		huart6;				/* Uart 6 for gsm communication 	*/
UART_HandleTypeDef 		huart3;				/* Uart 3 for console communication */
DRIVERConsoleHandler_t 	console;			/* Console handle 					*/
DRIVERConsoleConfig_t 	consoleConfig;		/* Console config handle			*/
DRIVERGsmHandler_t 		gsm;				/* Gsm handle						*/
DRIVERGsmConfig_t 		configGsm;			/* Gsm config handle				*/
TIMEHandler_t 			time;				/* Time handle						*/
TIMEConfig_t 			configTime;			/* Time config handle				*/
gsmHandler_t 			gsmHandler;			/* Handler for middleware			*/
gsmConfig_t				gsmCofig;			/* Config for middleware			*/
MQTTHandler_t 			mqttHandler;		/* Mqtt handle						*/
MQTTConfig_t 			mqttConfig;			/* Mqtt config						*/


/* Private function prototypes ---------------------------------------------------*/

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM6_Init(void);
DRIVERState_t MX_USART3_UART_Init(void);
DRIVERState_t MX_USART6_UART_Init(void);

void DemoTask(void* pvParameters);

uint8_t ReadChar( uint32_t timeout)
{
	uint8_t CharReceived = 0;
	if(HAL_UART_Receive(&huart3, &CharReceived, 1, timeout) == HAL_TIMEOUT)
		return 0;
	return CharReceived;
}

void WriteChar(uint8_t Char)
{
	HAL_UART_Transmit(&huart3, &Char, sizeof(Char),HAL_MAX_DELAY);
	return;
}


/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* MCU Configuration------------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();

  /* Set the lowest priority for systick timer */
  HAL_NVIC_SetPriority(SysTick_IRQn, 15 ,0U);

  /* Create demo task */
  xTaskCreate(DemoTask,"DemoTask", 4096,NULL,2,NULL);

  /* Set console config handle */
  consoleConfig.rxBuffer 	= rxbufferConsole;
  consoleConfig.txBuffer 	= txbufferConsole;
  consoleConfig.rxSize 		= sizeof(rxbufferConsole);
  consoleConfig.txSize 		= sizeof(txbufferConsole);
  consoleConfig.ReadChar 	= ReadChar;
  consoleConfig.WriteChar 	= WriteChar;
  consoleConfig.UartInit 	= MX_USART3_UART_Init;
  consoleConfig.uartBase 	= &huart3;

  /* Set gsm config handle */
  configGsm.rxBuffer 		= rxBufferGsm;
  configGsm.rxSize 			= sizeof(rxBufferGsm);
  configGsm.uartBase 		= &huart6;
  configGsm.UartInit 		= MX_USART6_UART_Init;

  /* Set time config handle */
  configTime.timerBase 		= &htim6;
  configTime.TimerInit 		= MX_TIM6_Init;

  /* Initialize handler for middleware layer */
  gsmCofig.gsm 				= &gsm;
  gsmCofig.console 			= &console;

  /* Set mqtt protocol config handle */
  mqttConfig.gsmHandler 	= &gsm;
  mqttConfig.consoleHandler = &console;

  /* Initialize console  */
  if(DRIVER_CONSOLE_Init(&console, &consoleConfig) != DRIVER_OK )
  {
	  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_SET);
  }

  /* Initialize gsm  */
  if(DRIVER_GSM_Init(&gsm, &configGsm) != DRIVER_OK )
  {
	  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_SET);
  }

  /* Initialize time  */
  if(TIME_Init(&time, &configTime) != TIME_OK )
  {
	  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_SET);
  }

  /* Initialize gsm handler in middleware layer */
  if(GSM_Init(&gsmHandler, &gsmCofig) != DRIVER_OK )
  {
	  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_SET);
  }

  /* Initialize mqtt protocol  */
  if(MQTT_Init(&mqttHandler, &mqttConfig) != MQTT_OK )
  {
	  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_SET);
  }

  /* Start timer for counting time */
  HAL_TIM_Base_Start_IT(&htim6);

  /* Set priority of interrupt routines */
  HAL_NVIC_SetPriority(USART3_IRQn,6,0);
  HAL_NVIC_SetPriority(USART6_IRQn,2,0);
  HAL_NVIC_SetPriority(TIM6_DAC_IRQn,8,0);
  HAL_NVIC_EnableIRQ(USART6_IRQn);
  HAL_NVIC_EnableIRQ(USART3_IRQn);
  HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);

  /* initalize buffers */
  memset(bufferConsole,0,sizeof(bufferConsole));
  memset(bufferGsm,0,sizeof(bufferConsole));

  /* start scheduler */
  vTaskStartScheduler();

  while (1)
  {

  }

}
void DemoTask(void* pvParameters){

	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Aviable commands:\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"set echo - set echo on or off!\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"set message format - it necessery to set format before sending messages!\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"test storage - see which storages are aviable!\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"Set storage - it necessery to set storage to be able to receive, send or delete messages!\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"list messages - list al types of messages currently in storages!\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"read message - read message using index of message parameter!\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"delete message - delete message using index of message parameter or delete all messages of specific type!\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"send message - send message from storage or directly!\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"store message - store message in storage!\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Commands to directly comunicate with gsm modul:\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"read - read buffer for receving characters from gsm\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"flush - set buffer for receiving characters from gsm to initial state\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"send cmd - send command directly to gsm modul \r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Commands for network and TCPIP connection:\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"turn on mobile network - network registration to mobile station\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"turn off mobile network - network deregistration(mobile cannot make calls,send messages and use network)\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"check mobile network - checking if gsm modul is network registered to mobile station\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"attach to gprs service - establish connection with base station\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"set apn - set Access Point Name \r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"check apn - check registered Access Point Name \r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"set wireless gprs connection - establish connection with mobile station with gprs\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"get ip address - get current IP adderess\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"set pdp - set Packet Data Protocol context to connect with server\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"check setted pdp - Check how many PDP contexts are setted(ruturn list of setted PDP context)\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"check active pdp - Check how many active PDP are there(ruturn list of active PDP context)\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"show pdp ip - Showing PDP IP adresses\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"active pdp - active Packet Data Protocol context \r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"deactive gprs pdp - deactive Packet Data Protocol context for GPRS connection \r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"deactive pdp - deactive Packet Data Protocol context \r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"set timer - set or unset auto sending timer: the seconds after which the data will be sent to server\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"connect to server - connect with server with specified PDP context\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"disconnect from server - disconnect from server\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"check server connection/check server - check status of connection with server\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"send data to server - send data to server\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Command for mqtt protocol(only use this commands when you set connection to mobile network and connection to specified server/broker!): \r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"connect to broker - connect command sent to broker\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"publish - publish message to the topic on specified broke\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"subscribe - subscribe to topic on specified broker\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"establish tcpip - does 3 commands: turn on mobile network, active pdp and connect to server\r\n");
	for(;;){

		DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nWaiting input command...\r\n");
		DRIVER_CONSOLE_Get(&console, bufferConsole, &size, portMAX_DELAY);

		  if(strstr((const char*)bufferConsole,(const char*)"flush\r") != NULL)
		  {
			  DRIVER_GSM_Flush(&gsm);
			  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nFlushed\r\n");
		  }
		  else if(strstr((const char*)bufferConsole,(const char*)"set echo\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nSetting echo ...\r\n");
		  		  GSM_SetEcho(&gsmHandler, 30000);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"set message format\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nSetting format ...");
		  		  GSM_MsgFormat(&gsmHandler, 30000);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"set storage\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nSetting storage ...\r\n");
		  		  GSM_SetMsgStorage(&gsmHandler, 30000);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"test storage\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nTesting storage ...\r\n");
		  		  GSM_TestMsgStorage(&gsmHandler, 30000);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"list messages\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nListing messages ...\r\n");
		  		  GSM_ListMsg(&gsmHandler, 10000);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"read message\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nReading message...\r\n");
		  		  GSM_ReadMsg(&gsmHandler, 30000);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"delete message\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nDeleting message...\r\n");
		  		  GSM_DeleteMsg(&gsmHandler, 30000);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"send message\r") != NULL || strstr((const char*)bufferConsole,(const char*)"store message\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nSending message...\r\n");
		  		  GSM_SendStoreMsg(&gsmHandler, 30000);
		  	  }
		  /* FROM HERE STARTS FUNCTIONS FOR NETWORK */
		  else if(strstr((const char*)bufferConsole,(const char*)"turn on mobile network\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nTurning on mobile network...\r\n");
		  		  GSM_NetworkRegistered(&gsmHandler);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"turn off mobile network\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nTurning off mobile network...\r\n");
		  		  GSM_NetworkDeregistered(&gsmHandler);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"attach to gprs service\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Attaching to gprs service...\r\n");
		  		  GSM_AttachToGPRSService(&gsmHandler);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"check mobile network\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nChecking mobile network...\r\n");
		  		  GSM_CheckNetworkRegistered(&gsmHandler);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"set apn\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nSetting APN...\r\n");
		  		  GSM_SetAPN(&gsmHandler);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"check apn\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nChecking APN...\r\n");
		  		  GSM_CheckAPN(&gsmHandler);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"set wireless gprs connection\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nSetting GPRS wireless connection...\r\n");
		  		  GSM_SetWirelessConnectionGPRS(&gsmHandler);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"get ip address\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nGetting local IP address...\r\n");
		  		  GSM_GetLocalIPAddress(&gsmHandler);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"set pdp\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nSetting PDP...\r\n");
		  		  GSM_SetPDPContext(&gsmHandler, 30000);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"check setted pdp\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nChecking setted PDP contexts...\r\n");
		  		  GSM_CheckSettedPDPContext(&gsmHandler);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"check active pdp\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nChecking active PDP contexts...\r\n");
		  		  GSM_CheckActivePDPContext(&gsmHandler);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"show pdp ip\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nShowing PDP IP addresses...\r\n");
		  		  GSM_ShowPDPIP(&gsmHandler);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"active pdp\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nActivating PDP...\r\n");
		  		  GSM_ActivePDPContext(&gsmHandler, 30000);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"deactive gprs pdp\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nDeactivating GPRS PDP...\r\n");
		  		  GSM_DeactiveGPRSPDPContext(&gsmHandler);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"deactive pdp\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nDeactivating PDP...\r\n");
		  		  GSM_DeactivePDPContext(&gsmHandler,30000);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"set timer\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nSeting auto sending timer ...");
		  		  GSM_SetAutoSendingTimerIP(&gsmHandler, 30000);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"connect to server\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nConnecting to server...\r\n");
		  		  GSM_ConnectToServer(&gsmHandler, 30000);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"disconnect from server\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nDisconnecting from server...\r\n");
		  		  GSM_DisconnectFromServer(&gsmHandler);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"check server connection\r") != NULL ||
				  strstr((const char*)bufferConsole,(const char*)"check server\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nChecking IP connection with server...\r\n");
		  		  GSM_CheckConnection(&gsmHandler);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"send data to server\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nSending data to server...\r\n");
		  		  GSM_SendToServer(&gsmHandler,30000);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"establish tcp client connection\r") != NULL ||
				  strstr((const char*)bufferConsole,(const char*)"establish connection\r") != NULL ||
				  strstr((const char*)bufferConsole,(const char*)"establish tcp connection\r") != NULL )
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nEstablishing TCP client connection...\r\n");
		  		  EstablishTCPClientConnection(&gsmHandler,30000);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"connect to broker\r") != NULL)
		  	  {
			  	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nConnectin to broker...\r\n");
		  		  MQTT_Connect(&mqttHandler,30000);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"publish\r") != NULL)
		  	  {
			  	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nPublishing message to the topic...\r\n");
		  		  MQTT_Publish(&mqttHandler,30000);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"establish tcpip\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nEstablishing TCPIP protocol ...\r\n");
		  		  EstablishTCPClientConnection(&gsmHandler, 30000);
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"main menu\r") != NULL ||
				  strstr((const char*)bufferConsole,(const char*)"help\r") != NULL)
		  	  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Aviable commands:\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"set echo - set echo on or off!\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"set message format - it necessery to set format before sending messages!\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"test storage - see which storages are aviable!\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"Set storage - it necessery to set storage to be able to receive, send or delete messages!\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"list messages - list al types of messages currently in storages!\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"read message - read message using index of message parameter!\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"delete message - delete message using index of message parameter or delete all messages of specific type!\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"send message - send message from storage or directly!\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"store message - store message in storage!\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Commands to directly comunicate with gsm modul:\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"read - read buffer for receving characters from gsm\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"flush - set buffer for receiving characters from gsm to initial state\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"send cmd - send command directly to gsm modul \r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Commands for network and TCPIP connection:\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"turn on mobile network - network registration to mobile station\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"turn off mobile network - network deregistration(mobile cannot make calls,send messages and use network)\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"check mobile network - checking if gsm modul is network registered to mobile station\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"attach to gprs service - establish connection with base station\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"set apn - set Access Point Name \r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"check apn - check registered Access Point Name \r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"set wireless gprs connection - establish connection with mobile station with gprs\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"get ip address - get current IP adderess\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"set pdp - set Packet Data Protocol context to connect with server\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"check setted pdp - Check how many PDP contexts are setted(ruturn list of setted PDP context)\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"check active pdp - Check how many active PDP are there(ruturn list of active PDP context)\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"show pdp ip - Showing PDP IP adresses\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"active pdp - active Packet Data Protocol context \r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"deactive gprs pdp - deactive Packet Data Protocol context for GPRS connection \r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"deactive pdp - deactive Packet Data Protocol context \r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"set timer - set or unset auto sending timer: the seconds after which the data will be sent to server\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"connect to server - connect with server with specified PDP context\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"disconnect from server - disconnect from server\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"check server connection - check status of connection with server\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"send data to server - send data to server\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Command for mqtt protocol(only use this commands when you set connection to mobile network and connection to specified server/broker!): \r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"connect to broker - connect command sent to broker\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"publish - publish message to the topic on specified broke\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"subscribe - subscribe to topic on specified broker\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"establish tcpip - does 3 commands: turn on mobile network, active pdp and connect to server\r\n");
		  	  }
		  else if(strstr((const char*)bufferConsole,(const char*)"read\r") != NULL)
		  {
			  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nReading ...\r\n");
			  DRIVER_GSM_Read(&gsm, bufferGsm , &sizeGsm);
			  if(sizeGsm == 0)
			  {
				  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nBuffer empty\r\n");
			  }
			  else
			  {
				  DRIVER_CONSOLE_Put(&console, bufferGsm);
				  DRIVER_CONSOLE_Put(&console, (const uint8_t*)"\r\n");
			  }
			  sizeGsm = 0;
		  }
		  else if(strstr((const char*)bufferConsole,(const char*)"at") != NULL || strstr((const char*)bufferConsole,(const char*)"AT") != NULL)
		  {
			  strcpy((char*)bufferGsm,(const char*)bufferConsole);
			  DRIVER_GSM_Write(&gsm, bufferGsm, size);
			  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"Command sent to gsm!\r\n");
//			  DRIVER_GSM_Read(&gsm, bufferGsm , &sizeGsm);
//			  if(sizeGsm != 0)
//			  {
//				  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nResponse from gsm:\r\n");
//				  DRIVER_CONSOLE_Put(&console, (const uint8_t*)"\r\n");
//				  DRIVER_CONSOLE_Put(&console, bufferGsm);
//				  DRIVER_CONSOLE_Put(&console, (const uint8_t*)"\r\n");
//			  }
//			  sizeGsm = 0;
		  }
		  else
		  {
			  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"Mistake in writing command. Please, write command correctly!\r\n");
		  }


		  /* Reset buffers for console and gsm */
		  memset(bufferConsole,0,sizeof(bufferConsole));
		  memset(bufferGsm,0,sizeof(bufferGsm));
	}
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART3;
  PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
  PeriphClkInitStruct.Usart16ClockSelection = RCC_USART16CLKSOURCE_D2PCLK2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 32000;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 1;
  htim6.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim6.Init.RepetitionCounter = 0;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
DRIVERState_t MX_USART6_UART_Init(void)
{
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  huart6.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart6.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart6.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    //Error_Handler();
	  return DRIVER_ERROR;
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart6, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    //Error_Handler();
	  return DRIVER_ERROR;
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart6, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    //Error_Handler();
	  return DRIVER_ERROR;
  }
  if (HAL_UARTEx_DisableFifoMode(&huart6) != HAL_OK)
  {
    //Error_Handler();
	  return DRIVER_ERROR;
  }
  /* USER CODE BEGIN USART6_Init 2 */
  /* Enable interrupt for uart 6 */
  __HAL_UART_ENABLE_IT(&huart6, UART_IT_RXNE);

  return DRIVER_OK;
  /* USER CODE END USART6_Init 2 */

}
/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
DRIVERState_t MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */
  /* Enable interrupt for uart 3 RX line */
  __HAL_UART_ENABLE_IT(&huart3, UART_IT_RXNE);

  return DRIVER_OK;
  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LD1_Pin|LD3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);

  /*Configure GPIO pins : LD1_Pin LD3_Pin */
  GPIO_InitStruct.Pin = LD1_Pin|LD3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
 for(;;);
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
