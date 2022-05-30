/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "string.h"

/* Private includes ----------------------------------------------------------*/
#include <driver_console.h>
#include <gsm.h>
#include <time.h>
#include <mqtt.h>
#include <mqtt_client.h>

#include "FreeRTOS.h"
#include "task.h"


/* receving buffer for console */
uint8_t rxbufferConsole[2000];

/* transmiting buffer for console */
uint8_t txbufferConsole[2000];

/* buffer in main task that are receiving message from console with get function */
uint8_t bufferConsole[2000] = {0};

/* Buffer for receiving characters from gsm in uart interrupt routine */
uint8_t rxBufferGsm[2000];

/* buffer in main task that are receiving message from gsm with get function */
uint8_t bufferGsm[2000] = {0};

/* Variable that are containing size of message stored in buffers for gsm and console */
uint32_t size = 0;
uint32_t sizeGsm = 0;

/* Variable for waiting user's input */
uint32_t timeout = 30000;


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
MQTTHandler_t 			mqtt;				/* Mqtt handle						*/
MQTTConfig_t 			mqttConfig;			/* Mqtt config						*/
MQTTClientHandler_t 	mqttCient;			/* Mqtt client handle				*/
MQTTClientConfig_t 		mqttClientConfig;	/* Mqtt client config				*/


/* Private function prototypes ---------------------------------------------------*/

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM6_Init(void);
DRIVERState_t MX_USART3_UART_Init(void);
DRIVERState_t MX_USART6_UART_Init(void);

void DemoTask(void* pvParameters);

/* Write and read char function we are not using 'cause we have implementation
 *  of console with tasks */
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
  xTaskCreate(DemoTask,"DemoTask", 10000,NULL,2,NULL);

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
//  gsmCofig.mqtt	 			= &mqtt;

  /* Set mqtt protocol config handle */
  mqttConfig.gsmHandler 	= &gsm;
  mqttConfig.consoleHandler = &console;

  /* Set mqtt client config handle */
  mqttClientConfig.gsm	 	= &gsm;
  mqttClientConfig.console 	= &console;
  mqttClientConfig.mqtt		= &mqtt;

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
  if(MQTT_Init(&mqtt, &mqttConfig) != MQTT_OK )
  {
	  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_SET);
  }

  /* Initialize mqtt client interface  */
  if(MQTT_CLIENT_Init(&mqttCient, &mqttClientConfig) != MQTT_CLIENT_OK )
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

	  /* Write main menu when we turn on/reset microcontroller  */
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n available commands:\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"set echo - set echo on or off!\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"set message format - it's necessary to set format before sending messages!\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"test storage - see which storages are available!\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"Set storage - it's necessary to set storage to be able to receive, send or delete messages!\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"list messages - list al types of messages currently in storages!\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"read message - read message using index of message parameter!\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"delete message - delete message using index of message parameter or delete all messages of specific type!\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"send message - send message from storage or directly!\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"store message - store message in storage!\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Commands to directly communicate with gsm modul:\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"read - read buffer for receving characters from gsm\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"flush - set buffer for receiving characters from gsm to initial state\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"*it's possible to directly send at commands to gsm module - just type the command in console and run command \"read\" after \r\n");
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
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"set packet format - set type of TCPIP packet format(hexadecimal or decimal)\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"connect to server - connect with server with specified PDP context\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"disconnect from server - disconnect from server\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"check server connection/check server - check status of connection with server\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"send data to server - send data to server\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Command for mqtt protocol(only use this commands when you set connection to mobile network and connection to specified server/broker!): \r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"connect to broker - connect command sent to broker\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"disconnect from broker - disconnect command sent to broker\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"publish - publish message to the topic on specified broke\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"subscribe - subscribe to topic on specified broker\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"ping/ping broker/ping req/ping request - send ping request to broker and wait for ping response from broker\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"establish tcpip - does 4 commands: turn on mobile network, active pdp, connect to server and set packet format of TCPIP connection\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Command that implement mqtt client:\r\n");
	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"mqtt client set - set state of mqtt client(either can be CLOSE or LISTEN)\r\n");
	  for(;;){

		  /* Waiting user's input from cosole */
		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nWaiting input command...\r\n");
		  DRIVER_CONSOLE_Get(&console, bufferConsole, &size, portMAX_DELAY);

		  /* Finding user's command and activating that command in the next part of the code */
		  if(strstr((const char*)bufferConsole,(const char*)"flush\r") != NULL)
		  {
			  DRIVER_GSM_Flush(&gsm);
			  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nFlushed\r\n");
		  }
		  else if(strstr((const char*)bufferConsole,(const char*)"set echo\r") != NULL)
		  {
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nSetting echo ...\r\n");

				/* Buffer for putting answer from gsm */
				uint8_t buffer[100] = {0};

				/* Flag that indicates when error in receiving from console occured */
				uint8_t breakFlag = 0;

				/* How many characters are received from gsm-it's importent to be zero initialize */
				uint32_t size = 0;

				/* Buffer of response from console about echo mode */
				GSMEcho_t echoOnOFF = GSM_ECHO_ON;

		  		/* Structure to return gsm response */
		  		OutputStruct_t outputStruct;

				/* Ask user witch mode of echo he wants */
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)"Enter 1 or 2 :\r\n 1: echo ON \r\n 2: echo OFF \r\n");
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

				/* Response from user */
				uint8_t incorrectInput = 0;
				while(incorrectInput != 3)
				{
					/* function that request from user only number for answer */
					switch(onlyPutNumber(&console, buffer, &size, sizeof(buffer), timeout)){
					case DRIVER_TIMEOUT:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_ERROR:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_OK:
						if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						break;
					}

					/* Exit this function and wait for another command */
					if(breakFlag == 1) break;

					/* Ask if input is expected  */
					if(*buffer == '1' && buffer[1] == '\r') {echoOnOFF = GSM_ECHO_ON; break;}
					else if(*buffer == '2' && buffer[1] == '\r') {echoOnOFF = GSM_ECHO_OFF; break;}
					else
					{

						if(incorrectInput == 2)
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						else
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Enter number 1 or 2!\r\n ");
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
							memset(buffer,0,sizeof(buffer));
							size = 0;
							incorrectInput++;
						}
					}

				}

				/* Exit this function and wait for another command */
				if(breakFlag == 0)
				{
					memset(buffer,0,sizeof(buffer));
					size = 0;
					outputStruct.gsmRsp = buffer;
					GSM_SetEcho(&gsmHandler, timeout, echoOnOFF,&outputStruct);
				}
		  }
		  else if(strstr((const char*)bufferConsole,(const char*)"set message format\r") != NULL)
		  {
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nSetting format ...");

		  		/* Buffer for putting answer from gsm */
		  		uint8_t buffer[100] = {0};

		  		uint8_t breakFlag = 0;

		  		/* How many characters are received from gsm */
		  		uint32_t size = 0;

		  		GSMMsgFormat_t format = GSM_TEXT_MODE;

		  		/* Structure to return gsm response */
		  		OutputStruct_t outputStructMsgFormat;

		  		/* Ask user witch mode he wants */
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nEnter 1 or 2 :\r\n 1: SMS text mode \r\n 2: SMS pdu mode \r\n ");
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

		  		/* Response from user */
		  		uint8_t incorrectInput = 0;
		  		while(incorrectInput != 3)
		  		{
					/* function that request from user only number for answer */
					switch(onlyPutNumber(&console, buffer, &size, sizeof(buffer), timeout)){
					case DRIVER_TIMEOUT:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_ERROR:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
		  			case DRIVER_OK:
		  				if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
		  				{
		  					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
		  					DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
		  				}
		  				break;
		  			}

					/* Exit this function and wait for another command */
					if(breakFlag == 1) break;

					/* Ask if input is expected  */
		  			if(*buffer == '1') {format = GSM_TEXT_MODE; break;}
		  			else if(*buffer == '2') {format = GSM_PDU_MODE; break;}
		  			else
		  			{

						if(incorrectInput == 2)
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						else
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Enter number 1 or 2!\r\n ");
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
							memset(buffer,0,sizeof(buffer));
							size = 0;
							incorrectInput++;
						}
		  			}
		  		}
		  		/* Exit this function and wait for another command */
		  		if(breakFlag == 0)
		  		{
					memset(buffer,0,sizeof(buffer));
					size = 0;
					outputStructMsgFormat.gsmRsp = buffer;
		  			GSM_MsgFormat(&gsmHandler, timeout, format,&outputStructMsgFormat);
		  		}
		  }
		  else if(strstr((const char*)bufferConsole,(const char*)"set storage\r") != NULL)
		  {
	  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nSetting storage ...\r\n");

				/* Memory for reading and delating messages */
				uint8_t memMsgReadDelate;

				/* Memory for writing and sending messages */
				uint8_t memMsgWriteSend;

				/* Memory for receiving messages */
				uint8_t memMsgReceive;

		  		/* Buffer for putting answer from gsm */
		  		uint8_t buffer[100] = {0};

		  		/* Flag that indicates when error in receiving from console occured */
		  		uint8_t breakFlag = 0;

		  		/* How many characters are received from gsm */
		  		uint32_t size = 0;

		  		/* Set buffer for storing answer from gsm */
		  		uint8_t gsmRsp[1000] = {0};

		  		/* Set input structure */
		  		SetMsgStrgInputStruct_t inputStruct =
		  		{
		  				.memMsgReadDelate 	= 0,
						.memMsgWriteSend 	= 0,
						.memMsgReceive 		= 0
		  		};

		  		/* Set output structure */
		  		OutputStruct_t outputStruct = {.gsmRsp = gsmRsp};

		  		/* Ask user witch memory he wants to use */
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Enter number for storage memory for reading and deleting messages :"
		  			  "\r\n 1 phone memory \r\n 2 SIM memory \r\n ");
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

		  		/* Response from user */
		  		uint8_t incorrectInput = 0;
		  		while(incorrectInput != 3)
		  		{
					/* function that request from user only number for answer */
					switch(onlyPutNumber(&console, buffer, &size, sizeof(buffer), timeout)){
					case DRIVER_TIMEOUT:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_ERROR:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
		  			case DRIVER_OK:
		  				if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
		  				{
		  					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
		  					DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
		  				}
		  				break;
		  			}

					/* Exit this function and wait for another command */
					if(breakFlag == 1) break;

					/* Ask if input is expected  */
		  			if(*buffer == '1') {memMsgReadDelate = 1; break;}
		  			else if(*buffer == '2') {memMsgReadDelate = 2; break;}
		  			else
		  			{

						if(incorrectInput == 2)
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						else
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Enter number 1 or 2!\r\n ");
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
							memset(buffer,0,sizeof(buffer));
							size = 0;
							incorrectInput++;
						}
		  			}
		  		}

		  		/* If first input is incorrect then leave function */
		  		if(breakFlag == 0)
		  		{
					/* Set buffer and his size to zero */
					memset(buffer,0,sizeof(buffer));
					size = 0;

					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n  Enter number for storage memory for writing and sending messages :"
					  "\r\n 1 phone memory \r\n 2 SIM memory \r\n");
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

					/* Response from user */
					uint8_t incorrectInput = 0;
					while(incorrectInput != 3)
					{
						/* function that request from user only number for answer */
						switch(onlyPutNumber(&console, buffer, &size, sizeof(buffer), timeout)){
						case DRIVER_TIMEOUT:
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						case DRIVER_ERROR:
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						case DRIVER_OK:
							if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							}
							break;
						}

						/* Exit this function and wait for another command */
						if(breakFlag == 1) break;

						/* Ask if input is expected  */
						if(*buffer == '1') {memMsgWriteSend = 1; break;}
						else if(*buffer == '2') {memMsgWriteSend = 2; break;}
						else
						{

							if(incorrectInput == 2)
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							}
							else
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Enter number 1 or 2!\r\n ");
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
								memset(buffer,0,sizeof(buffer));
								size = 0;
								incorrectInput++;
							}
						}
					}
					/* If second input is incorrect and first is good, leave function */
					if(breakFlag == 0)
					{
						/* Set buffer and his size to zero */
						memset(buffer,0,sizeof(buffer));
						size = 0;

						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n  Enter number for storage memory for receiving messages :"
						  "\r\n 1 phone memory \r\n 2 SIM memory \r\n ");
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

						/* Response from user */
						uint8_t incorrectInput = 0;
						while(incorrectInput != 3)
						{
							/* function that request from user only number for answer */
							switch(onlyPutNumber(&console, buffer, &size, sizeof(buffer), timeout)){
							case DRIVER_TIMEOUT:
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							case DRIVER_ERROR:
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							case DRIVER_OK:
								if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
								{
									DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
									DRIVER_GSM_Flush(&gsm);
									breakFlag = 1;
									break;
								}
								break;
							}

							/* Exit this function and wait for another command */
							if(breakFlag == 1) break;

							/* Ask if input is expected  */
							if(*buffer == '1') {memMsgReceive = 1; break;}
							else if(*buffer == '2') {memMsgReceive = 2; break;}
							else
							{

								if(incorrectInput == 2)
								{
									DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
									DRIVER_GSM_Flush(&gsm);
									breakFlag = 1;
									break;
								}
								else
								{
									DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Enter number 1 or 2!\r\n ");
									DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
									memset(buffer,0,sizeof(buffer));
									size = 0;
									incorrectInput++;
								}
							}
						}
						/* If third input is incorrect and 1st and 2nd are good, leave function */
						if(breakFlag == 0)
						{
							inputStruct.memMsgReadDelate = memMsgReadDelate;
							inputStruct.memMsgReceive = memMsgReceive;
							inputStruct.memMsgWriteSend = memMsgWriteSend;
							GSM_SetMsgStorage(&gsmHandler, timeout,inputStruct, &outputStruct);
						}
					}
		  		}
		  }
		  else if(strstr((const char*)bufferConsole,(const char*)"test storage\r") != NULL)
		  {
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nTesting storage ...\r\n");
				GSM_TestMsgStorage(&gsmHandler, timeout);
		  }
		  else if(strstr((const char*)bufferConsole,(const char*)"list messages\r") != NULL)
		  {
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nListing messages ...\r\n");

				/* Buffer for putting answer from gsm */
				uint8_t buffer[100] = {0};

				/* Flag that indicates when error in receiving from console occured */
				uint8_t breakFlag = 0;

				/* How many characters are received from gsm-it's importent to be zero initialize */
				uint32_t size = 0;

				/* First variable is for pdu format, second is for text format*/
				ListMsgInputStruct_t inputStruct;
				ListMsgOutputStruct_t outputStruct;

				/* output of function for list messages function */
				uint32_t indexofMsg[20] = {0};

				uint8_t msg0[100] = {0};
				uint8_t msg1[100] = {0};
				uint8_t msg2[100] = {0};
				uint8_t msg3[100] = {0};
				uint8_t msg4[100] = {0};
				uint8_t msg5[100] = {0};
				uint8_t msg6[100] = {0};
				uint8_t msg7[100] = {0};
				uint8_t msg8[100] = {0};
				uint8_t msg9[100] = {0};
				uint8_t msg10[100] = {0};
				uint8_t msg11[100] = {0};
				uint8_t msg12[100] = {0};
				uint8_t msg13[100] = {0};
				uint8_t msg14[100] = {0};
				uint8_t msg15[100] = {0};
				uint8_t msg16[100] = {0};
				uint8_t msg17[100] = {0};
				uint8_t msg18[100] = {0};
				uint8_t msg19[100] = {0};
				uint8_t *message[20] = { msg0, msg1, msg2, msg3, msg4, msg5, msg6, msg7, msg8, msg9, msg10, msg11, msg12, msg13, msg14, msg15, msg16, msg17, msg18, msg19};

				uint8_t no0[100] = {0};
				uint8_t no1[100] = {0};
				uint8_t no2[100] = {0};
				uint8_t no3[100] = {0};
				uint8_t no4[100] = {0};
				uint8_t no5[100] = {0};
				uint8_t no6[100] = {0};
				uint8_t no7[100] = {0};
				uint8_t no8[100] = {0};
				uint8_t no9[100] = {0};
				uint8_t no10[100] = {0};
				uint8_t no11[100] = {0};
				uint8_t no12[100] = {0};
				uint8_t no13[100] = {0};
				uint8_t no14[100] = {0};
				uint8_t no15[100] = {0};
				uint8_t no16[100] = {0};
				uint8_t no17[100] = {0};
				uint8_t no18[100] = {0};
				uint8_t no19[100] = {0};
				uint8_t *number[20] = { no0, no1, no2, no3, no4, no5, no6, no7, no8, no9, no10, no11, no12, no13, no14, no15, no16, no17, no18, no19};

				uint8_t tpy0[100] = {0};
				uint8_t tpy1[100] = {0};
				uint8_t tpy2[100] = {0};
				uint8_t tpy3[100] = {0};
				uint8_t tpy4[100] = {0};
				uint8_t tpy5[100] = {0};
				uint8_t tpy6[100] = {0};
				uint8_t tpy7[100] = {0};
				uint8_t tpy8[100] = {0};
				uint8_t tpy9[100] = {0};
				uint8_t tpy10[100] = {0};
				uint8_t tpy11[100] = {0};
				uint8_t tpy12[100] = {0};
				uint8_t tpy13[100] = {0};
				uint8_t tpy14[100] = {0};
				uint8_t tpy15[100] = {0};
				uint8_t tpy16[100] = {0};
				uint8_t tpy17[100] = {0};
				uint8_t tpy18[100] = {0};
				uint8_t tpy19[100] = {0};
				uint8_t *typeOfMsg[20] = { tpy0, tpy1, tpy2, tpy3, tpy4, tpy5, tpy6, tpy7, tpy8, tpy9, tpy10, tpy11, tpy12, tpy13, tpy14, tpy15, tpy16, tpy17, tpy18, tpy19};

				uint8_t time0[100] = {0};
				uint8_t time1[100] = {0};
				uint8_t time2[100] = {0};
				uint8_t time3[100] = {0};
				uint8_t time4[100] = {0};
				uint8_t time5[100] = {0};
				uint8_t time6[100] = {0};
				uint8_t time7[100] = {0};
				uint8_t time8[100] = {0};
				uint8_t time9[100] = {0};
				uint8_t time10[100] = {0};
				uint8_t time11[100] = {0};
				uint8_t time12[100] = {0};
				uint8_t time13[100] = {0};
				uint8_t time14[100] = {0};
				uint8_t time15[100] = {0};
				uint8_t time16[100] = {0};
				uint8_t time17[100] = {0};
				uint8_t time18[100] = {0};
				uint8_t time19[100] = {0};
				uint8_t *timeReceived[20] = { time0, time1, time2, time3, time4, time5, time6, time7, time8, time9, time10, time11, time12, time13, time14, time15, time16, time17, time18, time19};

				uint32_t msgNo = 0;
				outputStruct.msgNoStruct = &msgNo;
				/* input of function */
				char typeOfMsgChar = 0;
				uint8_t typeOfMsgStr[11] = {0};
				uint8_t sizeOfTypeOfMsgStr = 0;

				inputStruct.typeOfMsgChar = 0;
				inputStruct.sizeOfTypeOfMsgStr = 0;
				memset(inputStruct.typeOfMsgStr,0,sizeof(inputStruct.typeOfMsgStr));

				/* set output structure */
				outputStruct.msgNoStruct = 0;

				for ( uint8_t i = 0; i < sizeof(outputStruct.index); i++)
				{
					outputStruct.index[i] = indexofMsg[i]; /* assign the address of integer. */
				}
				for ( uint8_t i = 0; i <  sizeof(outputStruct.number); i++)
				{
					outputStruct.number[i] = number[i]; /* assign the address of integer. */
				}
				for ( uint8_t i = 0; i <  sizeof(outputStruct.timeReceived); i++)
				{
					outputStruct.timeReceived[i] = timeReceived[i]; /* assign the address of integer. */
				}
				for ( uint8_t i = 0; i <  20; i++)
				{
					outputStruct.typeOfMsg[i] = typeOfMsg[i]; /* assign the address of integer. */
				}
				for ( uint8_t i = 0; i < 20; i++)
				{
					outputStruct.message[i] = message[i]; /* assign the address of integer. */

				}

				/* Request to user */
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Which type of message would you like to list?\r\n");
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)" 1 Received unread message\r\n 2 Received read message\r\n");
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)" 3 Stored unsent message\r\n 4 Stored sent message\r\n");
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)" 5 All messages \r\n ");
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)"Enter number: \r\n ");
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

				/* Response from user */
				uint8_t incorrectInput = 0;
				while(incorrectInput != 3)
				{
					/* function that request from user only number for answer */
					switch(onlyPutNumber(&console, buffer, &size, sizeof(buffer), timeout)){
					case DRIVER_TIMEOUT:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_ERROR:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_OK:
						if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						break;
					}

					/* Exit this function and wait for another command */
					if(breakFlag == 1) break;

					/* Ask if input is expected  */
					if(*buffer == '1') {typeOfMsgChar = '1'; strcat((char*)typeOfMsgStr,(const char*)"REC UNREAD");sizeOfTypeOfMsgStr = 10;break;}
					else if(*buffer == '2') {typeOfMsgChar = '2';strcat((char*)typeOfMsgStr,(const char*)"REC READ");sizeOfTypeOfMsgStr = 8;break;}
					else if(*buffer == '3') {typeOfMsgChar = '3';strcat((char*)typeOfMsgStr,(const char*)"STO UNSENT");sizeOfTypeOfMsgStr = 10;break;}
					else if(*buffer == '4') {typeOfMsgChar = '4';strcat((char*)typeOfMsgStr,(const char*)"STO SENT");sizeOfTypeOfMsgStr = 8;break;}
					else if(*buffer == '5') {typeOfMsgChar = '5';strcat((char*)typeOfMsgStr,(const char*)"ALL");sizeOfTypeOfMsgStr = 3;break;}
					else
					{

						if(incorrectInput == 2)
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						else
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Enter number from 1 to 5 only!\r\n ");
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
							memset(buffer,0,sizeof(buffer));
							size = 0;
							incorrectInput++;
						}
					}

				}
				/* Exit this function and wait for another command */
				if(breakFlag == 0)
				{
					inputStruct.sizeOfTypeOfMsgStr = sizeOfTypeOfMsgStr;

					strcpy((char*)inputStruct.typeOfMsgStr,(const char*)typeOfMsgStr);

					inputStruct.typeOfMsgChar = typeOfMsgChar;

					GSM_ListMsg(&gsmHandler, 10000,inputStruct, &outputStruct);

					/* Set buffer and his size to zero */
					memset(buffer,0,sizeof(buffer));
					size = 0;

					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n  Do you want to publish to broker:"
					  "\r\n 1 Yes \r\n 2 No \r\n ");

					/* Response from user */
					uint8_t incorrectInput = 0;
					while(incorrectInput != 3)
					{
						/* function that request from user only number for answer */
						switch(onlyPutNumber(&console, buffer, &size, sizeof(buffer), timeout)){
						case DRIVER_TIMEOUT:
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						case DRIVER_ERROR:
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						case DRIVER_OK:
							if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							}
							break;
						}

						/* Exit this function and wait for another command */
						if(breakFlag == 1) break;

						/* Ask if input is expected  */
						if(*buffer == '1' || *buffer == '2')break;
						else
						{

							if(incorrectInput == 2)
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							}
							else
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Enter number 1 or 2!\r\n ");
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
								memset(buffer,0,sizeof(buffer));
								size = 0;
								incorrectInput++;
							}
						}
					}
					/* Call function for publishing to broker if we setted to */
					if(breakFlag == 0)
					{
						if(*buffer == '1')
						{
							uint8_t msgtoSend[1000] = {0};
							uint32_t msgSize = 0;
							for(uint32_t i = 0;i < *outputStruct.msgNoStruct;i++)
							{
								if(outputStruct.index[i] >= 10)
								{
									uint32_t mod = outputStruct.index[i] / 10;
									uint32_t rest =  outputStruct.index[i] % 10;
									msgtoSend[msgSize] = mod + '0';
									msgtoSend[msgSize + 1] = rest + '0';
								}
								else
									msgtoSend[msgSize] = outputStruct.index[i] + '0';
								strcat((char*)msgtoSend," ");
								strcat((char*)msgtoSend,(const char*)outputStruct.typeOfMsg[i]);
								strcat((char*)msgtoSend," ");
								strcat((char*)msgtoSend,(const char*)outputStruct.number[i]);
								strcat((char*)msgtoSend," ");
								if(outputStruct.timeReceived[i][2] != 0)
								{
									strcat((char*)msgtoSend,(const char*)outputStruct.timeReceived[i]);
									strcat((char*)msgtoSend," ");
								}
								strcat((char*)msgtoSend,(const char*)outputStruct.message[i]);
								strcat((char*)msgtoSend,"\r\n");
								msgSize = strlen((const char*)msgtoSend);
							}

							/* topic that will be sent to gsm */
							uint8_t topic[1000] = {0};

							memset(buffer,0,sizeof(buffer));
							size = 0;

							/* Request to user */
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Enter topic name: \r\n ");
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

							/* Receive from user */
							switch(DRIVER_CONSOLE_Get(&console, buffer, &size, timeout)){
							case DRIVER_TIMEOUT:
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							case DRIVER_ERROR:
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError received, not enough space for receiving characters! Please update your buffer! \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							case DRIVER_OK:
								if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end sending message */
								{
									DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Leaving command... \r\nMessage unsent!\r\n");
									DRIVER_GSM_Flush(&gsm);
									breakFlag = 1;
								}
								else
								{
									strcat((char*)topic,(const char*)buffer);
								}
								break;
							}

							MQTT_Publish(&mqtt,timeout,topic,msgtoSend);
						}
					}
				}
		  }
		  else if(strstr((const char*)bufferConsole,(const char*)"read message\r") != NULL)
		  {
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nReading message...\r\n");

				/* Buffer for putting answer from gsm */
				uint8_t buffer[10] = {0};

				/* Flag that indicates when error in receiving from console occured */
				uint8_t breakFlag = 0;

				/* How many characters are received from gsm-it's importent to be zero initialize */
				uint32_t size = 0;

				uint8_t typeOfMsg[15] = {0};
				uint8_t number[25] = {0};
				uint8_t timeReceived[25] = {0};
				uint8_t message[100] = {0};
				/* First variable is for pdu format, second is for text format*/
				ReadMsgInputStruct_t inputStruct = {.msgIndex = {0}};
				ReadMsgOutputStruct_t outputStruct =
				{
						.typeOfMsg = typeOfMsg,
						.number = number,
						.timeReceived = timeReceived,
						.message = message
				};




				DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Enter index of message to read (from 1 to number of messages): \r\n ");
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

				/* Response from user */
				uint8_t incorrectInput = 0;
				while(incorrectInput != 3)
				{
					/* function that request from user only number for answer */
					switch(onlyPutNumber(&console, buffer, &size, sizeof(buffer), timeout)){
					case DRIVER_TIMEOUT:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_ERROR:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_OK:
						if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						break;
					}

					/* Exit this function and wait for another command */
					if(breakFlag == 1) break;

					/* Ask if input is expected  */
					if(*buffer != '0') break;
					else
					{

						if(incorrectInput == 2)
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						else
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Enter any number except zero(0)!\r\n ");
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
							memset(buffer,0,sizeof(buffer));
							size = 0;
							incorrectInput++;
						}
					}
				}

				/* Exit this function and wait for another command */
				if(breakFlag == 0)
				{
					strcpy((char*)inputStruct.msgIndex,(const char*)buffer);

					GSM_ReadMsg(&gsmHandler, timeout, inputStruct, &outputStruct);

					/* Set buffer and his size to zero */
					memset(buffer,0,sizeof(buffer));
					size = 0;

					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n  Do you want to publish to broker:"
					  "\r\n 1 Yes \r\n 2 No \r\n ");

					/* Response from user */
					uint8_t incorrectInput = 0;
					while(incorrectInput != 3)
					{
						/* function that request from user only number for answer */
						switch(onlyPutNumber(&console, buffer, &size, sizeof(buffer), timeout)){
						case DRIVER_TIMEOUT:
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						case DRIVER_ERROR:
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						case DRIVER_OK:
							if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							}
							break;
						}

						/* Exit this function and wait for another command */
						if(breakFlag == 1) break;

						/* Ask if input is expected  */
						if(*buffer == '1' || *buffer == '2')break;
						else
						{

							if(incorrectInput == 2)
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							}
							else
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Enter number 1 or 2!\r\n ");
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
								memset(buffer,0,sizeof(buffer));
								size = 0;
								incorrectInput++;
							}
						}
					}
					/* Call function for publishing to broker if we setted to */
					if(breakFlag == 0)
					{
						if(*buffer == '1')
						{
							uint8_t msgtoSend[1000] = {0};

							if(inputStruct.msgIndex[1] != '\r')
							{
								msgtoSend[0] = *inputStruct.msgIndex;
								msgtoSend[1] = *inputStruct.msgIndex;
							}
							else
							{
								msgtoSend[0] = *inputStruct.msgIndex;
							}
							strcat((char*)msgtoSend," ");
							strcat((char*)msgtoSend,(const char*)outputStruct.typeOfMsg);
							strcat((char*)msgtoSend," ");
							strcat((char*)msgtoSend,(const char*)outputStruct.number);
							strcat((char*)msgtoSend," ");
							if(outputStruct.timeReceived[2] != 0)
							{
								strcat((char*)msgtoSend,(const char*)outputStruct.timeReceived);
								strcat((char*)msgtoSend," ");
							}
							strcat((char*)msgtoSend,(const char*)outputStruct.message);
							strcat((char*)msgtoSend,"\r\n");

							/* topic that will be sent to gsm */
							uint8_t topic[1000] = {0};

							memset(buffer,0,sizeof(buffer));
							size = 0;

							/* Request to user */
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Enter topic name: \r\n ");
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

							/* Receive from user */
							switch(DRIVER_CONSOLE_Get(&console, buffer, &size, timeout)){
							case DRIVER_TIMEOUT:
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							case DRIVER_ERROR:
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError received, not enough space for receiving characters! Please update your buffer! \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							case DRIVER_OK:
								if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end sending message */
								{
									DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Leaving command... \r\nMessage unsent!\r\n");
									DRIVER_GSM_Flush(&gsm);
									breakFlag = 1;
								}
								else
								{
									strcat((char*)topic,(const char*)buffer);
								}
								break;
							}

							MQTT_Publish(&mqtt,timeout,topic,msgtoSend);
						}
					}
				}
		  }
		  else if(strstr((const char*)bufferConsole,(const char*)"delete message\r") != NULL)
		  {
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nDeleting message...\r\n");

				/* Buffer for putting answer from gsm */
				uint8_t buffer[100] = {0};

				/* Flag that indicates when error in receiving from console occured */
				uint8_t breakFlag = 0;

				/* How many characters are received from gsm-it's importent to be zero initialize */
				uint32_t size = 0;

		  		/* Structure to return gsm response */
		  		OutputStruct_t outputStruct;

		  		/* Input structure for delete message function */
		  		DeleteMsgInputStruct_t inputStruct;

		  		/* Request to user */
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Enter which type of messagees would you like to delete: \r\n ");
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)"1 Certain message \r\n 2 All messages of a particular type \r\n ");
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

				/* Response from user */
				uint8_t incorrectInput = 0;
				while(incorrectInput != 3)
				{
					/* function that request from user only number for answer */
					switch(onlyPutNumber(&console, buffer, &size, sizeof(buffer), timeout)){
					case DRIVER_TIMEOUT:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_ERROR:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_OK:
						if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						break;
					}

					/* Exit this function and wait for another command */
					if(breakFlag == 1) break;

					/* Ask if input is expected  */
					if(*buffer == '1' || *buffer == '2') break;
					else
					{

						if(incorrectInput == 2)
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						else
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Enter number 1 or 2!\r\n ");
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
							memset(buffer,0,sizeof(buffer));
							size = 0;
							incorrectInput++;
						}
					}
				}

				/* Set type of preveous response and reset bufer and his size */
				uint8_t deleteType = *buffer;
				memset(buffer,0,sizeof(buffer));
				size = 0;


				/* Request to user */
				if(deleteType == '1')
				{
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nWhich is the index of the message you want to delete? Enter number:\r\n ");
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

					/* Response from user */
					uint8_t incorrectInput = 0;
					while(incorrectInput != 3)
					{
						/* function that request from user only number for answer */
						switch(onlyPutNumber(&console, buffer, &size, sizeof(buffer), timeout)){
						case DRIVER_TIMEOUT:
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						case DRIVER_ERROR:
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						case DRIVER_OK:
							if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							}
							break;
						}

						/* Exit this function and wait for another command */
						if(breakFlag == 1) break;

						/* Ask if input is expected  */
						if(*buffer != '0') break;
						else
						{

							if(incorrectInput == 2)
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							}
							else
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Enter any number except zero(0)!\r\n ");
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
								memset(buffer,0,sizeof(buffer));
								size = 0;
								incorrectInput++;
							}
						}
					}
				}
				else
				{
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nWhich type of message you want to delete?\r\n ");
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"1 Delete all received read messages?\r\n ");
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"2 Delete all received read and stored sent messages?\r\n ");
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"3 Delete all received read, stored sent messages and stored unsent messages?\r\n ");
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"4 Delete all messages of any type?\r\n ");
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

					/* Response from user */
					uint8_t incorrectInput = 0;
					while(incorrectInput != 3)
					{
						/* function that request from user only number for answer */
						switch(onlyPutNumber(&console, buffer, &size, sizeof(buffer), timeout)){
						case DRIVER_TIMEOUT:
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						case DRIVER_ERROR:
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						case DRIVER_OK:
							if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							}
							break;
						}

						/* Exit this function and wait for another command */
						if(breakFlag == 1) break;

						/* Ask if input is expected  */
						if(*buffer == '1' || *buffer == '2' || *buffer == '3' || *buffer == '4') break;
						else
						{

							if(incorrectInput == 2)
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							}
							else
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Enter number from 1 to 4!\r\n ");
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
								memset(buffer,0,sizeof(buffer));
								size = 0;
								incorrectInput++;
							}
						}
					}
				}
				/* Exit this function and wait for another command */
				if(breakFlag == 0)
				{
					/* initialize output structure */
					memset(buffer,0,sizeof(buffer));
					size = 0;
					outputStruct.gsmRsp = buffer;

					/* Set input structure */
					inputStruct.deleteType = deleteType;
					inputStruct.userRsp = buffer;

					GSM_DeleteMsg(&gsmHandler, timeout,inputStruct,&outputStruct);
				}
		  }
		  else if(strstr((const char*)bufferConsole,(const char*)"send message\r") != NULL ||
				  strstr((const char*)bufferConsole,(const char*)"store message\r") != NULL)
		  {
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nSending message...\r\n");

				/* Buffer for putting answer from gsm */
				uint8_t buffer[1000] = {0};

				/* Flag that indicates when error in receiving from console occured */
				uint8_t breakFlag = 0;

				/* How many characters are received from gsm-it's importent to be zero initialize */
				uint32_t size = 0;

				/* status of storing message or directly sending */
				uint8_t sendOrStoreFlag = 0;

				/* Status of send stored message or send directly by typing message */
				uint8_t storeOrSendDirectFlag = 0;

				/* Index of message that will be sent from stored if we choose to send from storage */
				uint8_t msgIndex[10] = {0};

				/* Number to which we will send the message */
				uint8_t number[100] = {0};

				/* Message that will be sent to gsm */
				uint8_t message[1000] = {0};

				/* Set input structure */
				SendOrStoreInputStruct_t inputStruct =
				{
						.index = msgIndex,
						.number = number,
						.message = message,
						.sendOrStoreFlag = sendOrStoreFlag,
						.storeOrSendDirectFlag = storeOrSendDirectFlag
				};

				/* Set output structure for storing response from gsm */
				OutputStruct_t outputStruct;


		  		/* Request to user */
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Enter number whether you want the message to be send immediately or first stored then later sent: \r\n ");
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)"1 Immediately send\r\n 2 First store\r\n");
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

				/* Response from user */
				uint8_t incorrectInput = 0;
				while(incorrectInput != 3)
				{
					/* function that request from user only number for answer */
					switch(onlyPutNumber(&console, buffer, &size, sizeof(buffer), timeout)){
					case DRIVER_TIMEOUT:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_ERROR:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_OK:
						if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						break;
					}

					/* Exit this function and wait for another command */
					if(breakFlag == 1) break;

					/* Ask if input is expected  */
					if(*buffer == '1' || *buffer == '2') {sendOrStoreFlag = *buffer; break;}
					else
					{

						if(incorrectInput == 2)
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						else
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Enter number 1 or 2!\r\n ");
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
							memset(buffer,0,sizeof(buffer));
							size = 0;
							incorrectInput++;
						}
					}
				}

				/* Ask if user want to send message from storage or directly send message we have to ask user that
				 * and set command for gsm */
				if(sendOrStoreFlag == '1')
				{
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"Do you want to send message from storage or directly send? Enter number:\r\n");
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"1 Send from storage\r\n2 Send directly\r\n");

					/* set buffer and his size to zero */
					memset(buffer,0,sizeof(buffer));
					size = 0;

					/* Response from user */
					incorrectInput = 0;
					while(incorrectInput != 3)
					{
						/* function that request from user only number for answer */
						switch(onlyPutNumber(&console, buffer, &size, sizeof(buffer), timeout)){
						case DRIVER_TIMEOUT:
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						case DRIVER_ERROR:
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						case DRIVER_OK:
							if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							}
							break;
						}

						/* Exit this function and wait for another command */
						if(breakFlag == 1) break;

						/* Ask if input is expected  */
						if(*buffer == '1' || *buffer == '2'){storeOrSendDirectFlag = *buffer; break;}
						else
						{

							if(incorrectInput == 2)
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							}
							else
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Enter number 1 or 2!\r\n ");
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
								memset(buffer,0,sizeof(buffer));
								size = 0;
								incorrectInput++;
							}
						}
					}
				}

				/** If message need to be sent from storage we have to demand index of that message **/
				if(storeOrSendDirectFlag == '1')
				{
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nWhich is the index of the message you want to send from storage? Enter number:\r\n ");
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

					/* set buffer and his size to zero */
					memset(buffer,0,sizeof(buffer));
					size = 0;

					/* Response from user */
					incorrectInput = 0;
					while(incorrectInput != 3)
					{
						/* function that request from user only number for answer */
						switch(onlyPutNumber(&console, buffer, &size, sizeof(buffer), timeout)){
						case DRIVER_TIMEOUT:
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						case DRIVER_ERROR:
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						case DRIVER_OK:
							if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							}
							break;
						}

						/* Exit this function and wait for another command */
						if(breakFlag == 1) break;

						/* Ask if input is expected  */
						if(*buffer != '0') {strcat((char*)msgIndex,(const char*)buffer);break;}
						else
						{

							if(incorrectInput == 2)
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							}
							else
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Enter any number except zero(0)!\r\n ");
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
								memset(buffer,0,sizeof(buffer));
								size = 0;
								incorrectInput++;
							}
						}
					}
				}

				/* set buffer and his size to zero */
				memset(buffer,0,sizeof(buffer));
				size = 0;

				/* Request to user */
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Enter mobile phone number whom you want to send message: \r\n ");
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

				/* Response from user */
				incorrectInput = 0;
				while(incorrectInput != 3)
				{
					/* function that request from user only number for answer */
					switch(DRIVER_CONSOLE_Get(&console, buffer, &size, timeout)){
					case DRIVER_TIMEOUT:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_ERROR:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_OK:
						if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
							breakFlag = 1;
						}
						break;
					}
					/* Exit this function and wait for another command */
					if(breakFlag == 1) break;

					uint8_t i = 0;
					for(i = 0; i < size - 1; i++)
					{
						if((buffer[i] < '0') || (buffer[i] > '9'))
						{
							if(buffer[i] != '+')
							{

								if(incorrectInput == 2)
								{
									DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
									DRIVER_GSM_Flush(&gsm);
									breakFlag = 1;
									break;
								}
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Enter only NUMBER greater then 0!\r\n");
								incorrectInput++;
								break;
							}
						}
					}

					/* If all characters are good, leave while, otherwise demand new input */
					if(i == size - 1 && *buffer != '\r') {strcat((char*)number,(const char*)buffer);break;}
					else
					{
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
						memset(buffer,0,sizeof(buffer));
						size = 0;
					}
				}

				if(storeOrSendDirectFlag != '1')
				{
					/* Reset buffer and his size */
					memset(buffer,0,sizeof(buffer));
					size = 0;

					/* Request to user */
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Enter message to send: \r\n ");
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

					/* Receive from user */
					switch(DRIVER_CONSOLE_Get(&console, buffer, &size, timeout)){
					case DRIVER_TIMEOUT:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_ERROR:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError received, not enough space for receiving characters! Please update your buffer! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_OK:
						if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end sending message */
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Leaving command... \r\nMessage unsent!\r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						strcat((char*)message,(const char*)buffer);
						break;
					}
				}

				if(breakFlag == 0)
				{
					inputStruct.sendOrStoreFlag = sendOrStoreFlag;
					inputStruct.storeOrSendDirectFlag = storeOrSendDirectFlag;
					/* set buffer and his size to zero */
					memset(buffer,0,sizeof(buffer));
					size = 0;
					outputStruct.gsmRsp = buffer;

					GSM_SendStoreMsg(&gsmHandler, timeout, inputStruct, &outputStruct);
				}
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

				/* Buffer for putting answer from gsm */
				uint8_t buffer[1000] = {0};

				/* Flag that indicates when error in receiving from console occured */
				uint8_t breakFlag = 0;

				/* How many characters are received from gsm-it's importent to be zero initialize */
				uint32_t size = 0;

				/* Variable for number of PDP context */
				uint8_t PDPNo[3] = {0};

				/* Variable for type of PDP context */
				uint8_t PDPType[10] = {0};

				/* Variable for type of APN */
				uint8_t APNType[100] = {0};

		  		/* Set input structure */
		  		SetPDPInputStruct_t inputStruct;

		  		/* Request to user */
		  		uint8_t maxSocNumStr[3] = {0};
		  		itoa(MAX_SOCKET_NUMBER,(char*) maxSocNumStr, 10);
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Enter number of which PDP context to use (range from 1 to ");
		  		DRIVER_CONSOLE_Put(&console, maxSocNumStr);
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)"): \r\n ");
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

				/* Response from user */
				uint8_t incorrectInput = 0;
				while(incorrectInput != 3)
				{
					/* function that request from user only number for answer */
					switch(onlyPutNumber(&console, buffer, &size, sizeof(buffer), timeout)){
					case DRIVER_TIMEOUT:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_ERROR:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_OK:
						if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						break;
					}

					/* Exit this function and wait for another command */
					if(breakFlag == 1) break;

					/* Ask if input is expected  */
					if( (buffer[0] == '1' && buffer[1] == '\r') || (buffer[0] == '2' && buffer[1] == '\r') || (buffer[0] == '3' && buffer[1] == '\r') ||
					    (buffer[0] == '4' && buffer[1] == '\r') || (buffer[0] == '5' && buffer[1] == '\r') || (buffer[0] == '6' && buffer[1] == '\r') ||
						(buffer[0] == '7' && buffer[1] == '\r') || (buffer[0] == '8' && buffer[1] == '\r') || (buffer[0] == '9' && buffer[1] == '\r') ||
						(buffer[0] == '1' && buffer[1] == '0')  || (buffer[0] == '1' && buffer[1] == '1')  || (buffer[0] == '1' && buffer[1] == '2')  ||
						(buffer[0] == '1' && buffer[1] == '3')  || (buffer[0] == '1' && buffer[1] == '4')  || (buffer[0] == '1' && buffer[1] == '5')  ||
						(buffer[0] == '1' && buffer[1] == '6') )
					{
						strcat((char*)PDPNo,(const char*)buffer);
						break;
					}
					else
					{

						if(incorrectInput == 2)
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						else
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Enter number from 1 to 16!\r\n ");
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
							memset(buffer,0,sizeof(buffer));
							size = 0;
							incorrectInput++;
						}
					}
				}

				/* Request to user */
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Enter number of witch packet data protocol you will be using:\r\n 1 IP(Internet Protocol)\r\n "
						"2 IPV6(Internet Protocol, version 6)\r\n 3 PPP(Point to Point Protocol)\r\n ");
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

				/* Response from user */
				incorrectInput = 0;
				while(incorrectInput != 3)
				{
					/* function that request from user only number for answer */
					switch(onlyPutNumber(&console, buffer, &size, sizeof(buffer), timeout)){
					case DRIVER_TIMEOUT:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_ERROR:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_OK:
						if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						break;
					}

					/* Exit this function and wait for another command */
					if(breakFlag == 1) break;

					/* Ask if input is expected  */
					if(*buffer == '1' || *buffer == '2' || *buffer == '3')
					{
						strcat((char*)PDPType,(const char*)buffer);
						break;
					}
					else
					{

						if(incorrectInput == 2)
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						else
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Enter number from 1 to 16!\r\n ");
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
							memset(buffer,0,sizeof(buffer));
							size = 0;
							incorrectInput++;
						}
					}
				}

				/* Request to user */
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Enter access point name:\r\n ");
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

				/* Response from user */
				switch(DRIVER_CONSOLE_Get(&console, buffer, &size, timeout)){
				case DRIVER_TIMEOUT:
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
					breakFlag = 1;
					break;
				case DRIVER_ERROR:
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
					breakFlag = 1;
					break;
				case DRIVER_OK:
					if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
					{
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					}
					strcat((char*)APNType,(const char*)buffer);
					break;
				}

				if(breakFlag == 0)
				{
					inputStruct.APNType = APNType;
					inputStruct.PDPNo = PDPNo;
					inputStruct.PDPTypeFlag = PDPType;
					GSM_SetPDPContext(&gsmHandler, timeout, inputStruct);
				}
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
		  else if(strcmp((const char*)bufferConsole,(const char*)"active pdp\r") == 0)
		  {
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nActivating PDP...\r\n");

				/* Buffer for putting answer from gsm */
				uint8_t buffer[1000] = {0};

				/* Flag that indicates when error in receiving from console occured */
				uint8_t breakFlag = 0;

				/* How many characters are received from gsm-it's importent to be zero initialize */
				uint32_t size = 0;

				/* Variable for number of PDP context */
				uint8_t PDP[2] = {0};

		  		/* Request to user */
		  		uint8_t maxSocNumStr[3];
		  		memset(maxSocNumStr,0,sizeof(maxSocNumStr));
		  		itoa(MAX_SOCKET_NUMBER,(char*) maxSocNumStr, 10);
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Enter number of PDP context (range from 1 to ");
		  		DRIVER_CONSOLE_Put(&console, maxSocNumStr);
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)"): \r\n ");
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

				/* Response from user */
				uint8_t incorrectInput = 0;
				while(incorrectInput != 3)
				{
					/* function that request from user only number for answer */
					switch(onlyPutNumber(&console, buffer, &size, sizeof(buffer), timeout)){
					case DRIVER_TIMEOUT:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_ERROR:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_OK:
						if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						break;
					}

					/* Exit this function and wait for another command */
					if(breakFlag == 1) break;

					/* Ask if input is expected  */
					if( (buffer[0] == '1' && buffer[1] == '\r') || (buffer[0] == '2' && buffer[1] == '\r') || (buffer[0] == '3' && buffer[1] == '\r') ||
					    (buffer[0] == '4' && buffer[1] == '\r') || (buffer[0] == '5' && buffer[1] == '\r') || (buffer[0] == '6' && buffer[1] == '\r') ||
						(buffer[0] == '7' && buffer[1] == '\r') || (buffer[0] == '8' && buffer[1] == '\r') || (buffer[0] == '9' && buffer[1] == '\r') ||
						(buffer[0] == '1' && buffer[1] == '0')  || (buffer[0] == '1' && buffer[1] == '1')  || (buffer[0] == '1' && buffer[1] == '2')  ||
						(buffer[0] == '1' && buffer[1] == '3')  || (buffer[0] == '1' && buffer[1] == '4')  || (buffer[0] == '1' && buffer[1] == '5')  ||
						(buffer[0] == '1' && buffer[1] == '6') )
					{
						strcat((char*)PDP,(const char*)buffer);
						break;
					}
					else
					{

						if(incorrectInput == 2)
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						else
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Enter number from 1 to 16!\r\n ");
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
							memset(buffer,0,sizeof(buffer));
							size = 0;
							incorrectInput++;
						}
					}
				}

				if(breakFlag == 0)
				{
					GSM_ActivePDPContext(&gsmHandler, timeout,PDP);
				}
		  }
		  else if(strstr((const char*)bufferConsole,(const char*)"deactive gprs pdp\r") != NULL)
		  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nDeactivating GPRS PDP...\r\n");
		  		  GSM_DeactiveGPRSPDPContext(&gsmHandler);
		  }
		  else if(strcmp((const char*)bufferConsole,(const char*)"deactive pdp\r") == 0)
		  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nDeactivating PDP...\r\n");
					/* Buffer for putting answer from gsm */
					uint8_t buffer[1000] = {0};

					/* Flag that indicates when error in receiving from console occured */
					uint8_t breakFlag = 0;

					/* How many characters are received from gsm-it's importent to be zero initialize */
					uint32_t size = 0;

					/* Variable for number of PDP context */
					uint8_t PDP[2] = {0};

			  		/* Request to user */
			  		uint8_t maxSocNumStr[3];
			  		memset(maxSocNumStr,0,sizeof(maxSocNumStr));
			  		itoa(MAX_SOCKET_NUMBER,(char*) maxSocNumStr, 10);
			  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Enter number of PDP context (range from 1 to ");
			  		DRIVER_CONSOLE_Put(&console, maxSocNumStr);
			  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)"): \r\n ");
			  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

					/* Response from user */
					uint8_t incorrectInput = 0;
					while(incorrectInput != 3)
					{
						/* function that request from user only number for answer */
						switch(onlyPutNumber(&console, buffer, &size, sizeof(buffer), timeout)){
						case DRIVER_TIMEOUT:
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						case DRIVER_ERROR:
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						case DRIVER_OK:
							if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							}
							break;
						}

						/* Exit this function and wait for another command */
						if(breakFlag == 1) break;

						/* Ask if input is expected  */
						if( (buffer[0] == '1' && buffer[1] == '\r') || (buffer[0] == '2' && buffer[1] == '\r') || (buffer[0] == '3' && buffer[1] == '\r') ||
						    (buffer[0] == '4' && buffer[1] == '\r') || (buffer[0] == '5' && buffer[1] == '\r') || (buffer[0] == '6' && buffer[1] == '\r') ||
							(buffer[0] == '7' && buffer[1] == '\r') || (buffer[0] == '8' && buffer[1] == '\r') || (buffer[0] == '9' && buffer[1] == '\r') ||
							(buffer[0] == '1' && buffer[1] == '0')  || (buffer[0] == '1' && buffer[1] == '1')  || (buffer[0] == '1' && buffer[1] == '2')  ||
							(buffer[0] == '1' && buffer[1] == '3')  || (buffer[0] == '1' && buffer[1] == '4')  || (buffer[0] == '1' && buffer[1] == '5')  ||
							(buffer[0] == '1' && buffer[1] == '6') )
						{
							strcat((char*)PDP,(const char*)buffer);
							break;
						}
						else
						{

							if(incorrectInput == 2)
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							}
							else
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Enter number from 1 to 16!\r\n ");
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
								memset(buffer,0,sizeof(buffer));
								size = 0;
								incorrectInput++;
							}
						}
					}

					if(breakFlag == 0)
					{
						GSM_DeactivePDPContext(&gsmHandler, timeout,PDP);
					}
		  }
		  else if(strstr((const char*)bufferConsole,(const char*)"set timer\r") != NULL)
		  {
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nSeting auto sending timer ...");

				/* Buffer for putting answer from gsm */
				uint8_t buffer[100] = {0};

				/* Flag that indicates when error in receiving from console occured */
				uint8_t breakFlag = 0;

				/* How many characters are received from gsm-it's importent to be zero initialize */
				uint32_t size = 0;

				/* Buffer of response from console about echo mode */
				uint8_t status = 0;

				/* Varible that contains time (number from 1 to 101) */
				uint8_t time[4] = {0};

		  		/* Request to user */
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Enter option: \r\n 1 Not set timer \r\n 2 Set timer \r\n ");
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

				/* Response from user */
				uint8_t incorrectInput = 0;
				while(incorrectInput != 3)
				{
					/* function that request from user only number for answer */
					switch(onlyPutNumber(&console, buffer, &size, sizeof(buffer), timeout)){
					case DRIVER_TIMEOUT:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_ERROR:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_OK:
						if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						break;
					}

					/* Exit this function and wait for another command */
					if(breakFlag == 1) break;

					/* Ask if input is expected  */
					if(*buffer == '1' && buffer[1] == '\r') {status = '1'; break;}
					else if(*buffer == '2' && buffer[1] == '\r') {status = '2'; break;}
					else
					{

						if(incorrectInput == 2)
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						else
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Enter number 1 or 2!\r\n ");
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
							memset(buffer,0,sizeof(buffer));
							size = 0;
							incorrectInput++;
						}
					}

				}

				if(status == '2')
				{
					/* Reset buffer and his size */
					memset(buffer,0,sizeof(buffer));
					size = 0;

					/* Request to user */
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Enter the seconds after which the data will be sent to server\r\n ");
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

					/* Response from user */
					incorrectInput = 0;
					while(incorrectInput != 3)
					{
						/* function that request from user only number for answer */
						switch(onlyPutNumber(&console, buffer, &size, sizeof(buffer), timeout)){
						case DRIVER_TIMEOUT:
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						case DRIVER_ERROR:
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						case DRIVER_OK:
							if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							}
							break;
						}

						/* Exit this function and wait for another command */
						if(breakFlag == 1) break;

						/* Ask if input is expected  */
						/* 3 types of input could be:
						 * 1. buffer[0] in the range of 1 to 9 and buffer[1] is '\r' char
						 * 2. buffer[0] in the range of 1 to 9 and buffer[1] in the range of 0 to 9 and buffer[2] is '\r' char
						 * 3. buffer[0] is 1 and buffer[1] is 0 and buffer[2] is 0 */
						if( ((buffer[0] == '1' || buffer[0] == '2' || buffer[0] == '3' || buffer[0] == '4' || buffer[0] == '5'    ||
							  buffer[0] == '6' || buffer[0] == '7' || buffer[0] == '8' || buffer[0] == '9') && buffer[1] == '\r') ||

							((buffer[0] == '1' || buffer[0] == '2' || buffer[0] == '3' || buffer[0] == '4' || buffer[0] == '5'    ||
							  buffer[0] == '6' || buffer[0] == '7' || buffer[0] == '8' || buffer[0] == '9') &&
							( buffer[1] == '1' || buffer[1] == '2' || buffer[1] == '3' || buffer[1] == '4' || buffer[1] == '5'    ||
							  buffer[1] == '6' || buffer[1] == '7' || buffer[1] == '8' || buffer[1] == '9' || buffer[1] == '0')	&&
							  buffer[2] == '\r') ||

							 (buffer[0] == '1' && buffer[1] == '0' && buffer[2] == '0'))
						{
							strcat((char*)time,(const char*) buffer);
							break;
						}
						else
						{

							if(incorrectInput == 2)
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							}
							else
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Enter number between 1 and 101!\r\n ");
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
								memset(buffer,0,sizeof(buffer));
								size = 0;
								incorrectInput++;
							}
						}

					}

				}

				/* Exit this function and wait for another command */
				if(breakFlag == 0)
				{
					GSM_SetAutoSendingTimerIP(&gsmHandler, timeout,status, time);
				}
		  }
		  else if(strstr((const char*)bufferConsole,(const char*)"set packet format\r") != NULL)
		  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nSeting packet format for TCPIP protocol ...");

					/* Buffer for putting answer from gsm */
					uint8_t buffer[100] = {0};

					/* Flag that indicates when error in receiving from console occured */
					uint8_t breakFlag = 0;

					/* How many characters are received from gsm-it's importent to be zero initialize */
					uint32_t size = 0;

					/* Buffer of response from console about echo mode */
					uint8_t format = 0;

					/* Ask user witch mode of echo he wants */
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"Set sending packets format of TCPIP \r\n Enter 1 or 2 :\r\n 1: Hexadecimal format \r\n 2: Decimal format\r\n");
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

					/* Response from user */
					uint8_t incorrectInput = 0;
					while(incorrectInput != 3)
					{
						/* function that request from user only number for answer */
						switch(onlyPutNumber(&console, buffer, &size, sizeof(buffer), timeout)){
						case DRIVER_TIMEOUT:
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						case DRIVER_ERROR:
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						case DRIVER_OK:
							if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							}
							break;
						}

						/* Exit this function and wait for another command */
						if(breakFlag == 1) break;

						/* Ask if input is expected  */
						if(*buffer == '1' && buffer[1] == '\r') {format = '1'; break;}
						else if(*buffer == '2' && buffer[1] == '\r') {format = '2'; break;}
						else
						{

							if(incorrectInput == 2)
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
								DRIVER_GSM_Flush(&gsm);
								breakFlag = 1;
								break;
							}
							else
							{
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Enter number 1 or 2!\r\n ");
								DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
								memset(buffer,0,sizeof(buffer));
								size = 0;
								incorrectInput++;
							}
						}
					}

					/* Exit this function and wait for another command */
					if(breakFlag == 0)
					{
						GSM_SetSendingIPFormat(&gsmHandler, timeout,format);
					}
		  }
		  else if(strstr((const char*)bufferConsole,(const char*)"connect to server\r") != NULL)
		  {
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nConnecting to server...\r\n");

				/* Buffer for putting answer from gsm */
				uint8_t buffer[100] = {0};

				/* Flag that indicates when error in receiving from console occured */
				uint8_t breakFlag = 0;

				/* How many characters are received from gsm-it's importent to be zero initialize */
				uint32_t size = 0;

				/* type of connection - TCP or UDP */
				uint8_t connectType = 0;

				/* IP address */
				uint8_t ipAddr[17] = {0};

				/* port of server */
				uint8_t port[7] = {0};

				/* Set input structure */
				ConnectSrvrInputStruct_t inputStruct =
				{
						.connectType = 0
				};

		  		/* Request to user */
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Do you want to use TCP or UDP type of connection? \r\n Enter number : \r\n 1 TCP \r\n 2 UDP\r\n ");
		  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

				/* Response from user */
				uint8_t incorrectInput = 0;
				while(incorrectInput != 3)
				{
					/* function that request from user only number for answer */
					switch(onlyPutNumber(&console, buffer, &size, sizeof(buffer), timeout)){
					case DRIVER_TIMEOUT:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_ERROR:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_OK:
						if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						break;
					}

					/* Exit this function and wait for another command */
					if(breakFlag == 1) break;

					/* Ask if input is expected  */
					if(*buffer == '1' && buffer[1] == '\r') {connectType = '1'; break;}
					else if(*buffer == '2' && buffer[1] == '\r') {connectType = '2'; break;}
					else
					{

						if(incorrectInput == 2)
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						else
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Enter number 1 or 2!\r\n ");
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
							memset(buffer,0,sizeof(buffer));
							size = 0;
							incorrectInput++;
						}
					}
				}

				/* Reset buffer and his size */
				memset(buffer,0,sizeof(buffer));
				size = 0;

				/* Request to user */
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Insert IP address of server with whom you will connect to:\r\n ");
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

				/* Response from user */
				incorrectInput = 0;
				while(incorrectInput != 3)
				{
					/* Get IP address that contains numbers(0-9) and dots(.) Example: 125.2.0.129 */
					switch(DRIVER_CONSOLE_Get(&console, buffer, &size, timeout)){
					case DRIVER_TIMEOUT:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_ERROR:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_OK:
						if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
						}
						break;
					}

					/* Exit this function and wait for another command */
					if(breakFlag == 1) break;

					uint8_t i = 0;
					for(i = 0; i < size - 1; i++)
					{
						/* Check if characters are only numbers from 0 to 9 or dots */
						if((buffer[i] < '0') || (buffer[i] > '9'))
						{
							if(buffer[i] != '.')
							{
								if(incorrectInput == 2)
								{
									DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
									DRIVER_GSM_Flush(&gsm);
									breakFlag = 1;
								}
								else
								{
									DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Insert only numbers and dots in form of n.n.n.n , when n is number! \r\n");
									incorrectInput++;
									memset(buffer,0,sizeof(buffer));
									size = 0;
								}
								break;
							}
						}
					}

					/* Exit this function and wait for another command */
					if(breakFlag == 1) break;

					/* If all characters are good, leave while, otherwise demand new input */
					if(i == size - 1 && *buffer != '\r') break;
					else
					{
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
						memset(buffer,0,sizeof(buffer));
						size = 0;
					}
				} // end of while

				/* Set IP address */
				strcat((char*)ipAddr,(const char*)buffer);

				/* Reset buffer and his size */
				memset(buffer,0,sizeof(buffer));
				size = 0;

				/* Request to user */
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Insert server port: \r\n ");
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

				/* Response from user */
				/* function that request from user only number for answer */
				switch(onlyPutNumber(&console, buffer, &size, sizeof(buffer), timeout)){
				case DRIVER_TIMEOUT:
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
					breakFlag = 1;
					break;
				case DRIVER_ERROR:
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
					breakFlag = 1;
					break;
				case DRIVER_OK:
					if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
					{
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
					}
					else
					{	/* Set port */
						strcat((char*)port,(const char*)buffer);
					}
					break;
				}

				if(breakFlag == 0)
				{
					inputStruct.connectType = connectType;
					inputStruct.ipAddr = ipAddr;
					inputStruct.port = port;
					GSM_ConnectToServer(&gsmHandler, timeout,inputStruct);
				}
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

					/* Buffer for putting answer from gsm */
					uint8_t buffer[1000] = {0};

					/* Flag that indicates when error in receiving from console occured */
					uint8_t breakFlag = 0;

					/* How many characters are received from gsm-it's importent to be zero initialize */
					uint32_t size = 0;

					/* Message that will be sent to gsm */
					uint8_t message[1000] = {0};

					/* Request to user */
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Enter message to send: \r\n ");
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

					/* Receive from user */
					switch(DRIVER_CONSOLE_Get(&console, buffer, &size, timeout)){
					case DRIVER_TIMEOUT:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_ERROR:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError received, not enough space for receiving characters! Please update your buffer! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_OK:
						if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end sending message */
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Leaving command... \r\nMessage unsent!\r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						strcat((char*)message,(const char*)buffer);
						break;
					}

					if(breakFlag == 0)
					{
						GSM_SendToServer(&gsmHandler,timeout,message);
					}
		  }
		  else if(strstr((const char*)bufferConsole,(const char*)"establish tcp client connection\r") != NULL ||
				  strstr((const char*)bufferConsole,(const char*)"establish connection\r") != NULL ||
				  strstr((const char*)bufferConsole,(const char*)"establish tcp connection\r") != NULL )
		  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nEstablishing TCP client connection...\r\n");
		  		  EstablishTCPClientConnection(&gsmHandler,timeout);
		  }
		  else if(strstr((const char*)bufferConsole,(const char*)"connect to broker\r") != NULL)
		  {
			  	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nConnecting to broker...\r\n");
		  		  MQTT_Connect(&mqtt);
		  }
		  else if(strstr((const char*)bufferConsole,(const char*)"disconnect from broker\r") != NULL)
		  {
			  	  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nDisonnecting to broker...\r\n");
		  		  MQTT_Disconnect(&mqtt);
		  }
		  else if(strstr((const char*)bufferConsole,(const char*)"publish\r") != NULL)
		  {
			  	DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nPublishing message to the topic...\r\n");

				/* Buffer for putting answer from gsm */
				uint8_t buffer[1000] = {0};

				/* Flag that indicates when error in receiving from console occured */
				uint8_t breakFlag = 0;

				/* How many characters are received from gsm-it's importent to be zero initialize */
				uint32_t size = 0;

				/* topic that will be sent to gsm */
				uint8_t topic[1000] = {0};

				/* Message that will be sent to gsm */
				uint8_t message[1000] = {0};

				/* Request to user */
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Enter topic name: \r\n ");
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

				/* Receive from user */
				switch(DRIVER_CONSOLE_Get(&console, buffer, &size, timeout)){
				case DRIVER_TIMEOUT:
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
					DRIVER_GSM_Flush(&gsm);
					breakFlag = 1;
					break;
				case DRIVER_ERROR:
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError received, not enough space for receiving characters! Please update your buffer! \r\n");
					DRIVER_GSM_Flush(&gsm);
					breakFlag = 1;
					break;
				case DRIVER_OK:
					if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end sending message */
					{
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Leaving command... \r\nMessage unsent!\r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
					}
					else
					{
						strcat((char*)topic,(const char*)buffer);
					}
					break;
				}

				/* Reset buffer and his size */
				memset(buffer,0,sizeof(buffer));
				size = 0;

				/* Request to user */
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Enter message to publish: \r\n ");
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

				/* Receive from user */
				switch(DRIVER_CONSOLE_Get(&console, buffer, &size, timeout)){
				case DRIVER_TIMEOUT:
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
					DRIVER_GSM_Flush(&gsm);
					breakFlag = 1;
					break;
				case DRIVER_ERROR:
					DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError received, not enough space for receiving characters! Please update your buffer! \r\n");
					DRIVER_GSM_Flush(&gsm);
					breakFlag = 1;
					break;
				case DRIVER_OK:
					if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end sending message */
					{
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Leaving command... \r\nMessage unsent!\r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
					}
					else
					{
						strcat((char*)message,(const char*)buffer);
					}
					break;
				}


			  	if(breakFlag == 0)
			  	{
			  		MQTT_Publish(&mqtt,timeout,topic,message);
			  	}
		  }
		  else if(strstr((const char*)bufferConsole,(const char*)"subscribe\r") != NULL)
		  {
			  	DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nSubscribing to the topic...\r\n");

				/* Buffer for putting answer from gsm */
				uint8_t buffer[1000] = {0};

				/* Flag that indicates when error in receiving from console occured */
				uint8_t breakFlag = 0;

				/* How many characters are received from gsm-it's importent to be zero initialize */
				uint32_t size = 0;

				/* Message that will be sent to gsm */
				uint8_t topic[1000] = {0};

			  	/* Request to user */
			  	DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Enter topic name: \r\n ");
			  	DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

			  	/* Receive from user */
			  	switch(DRIVER_CONSOLE_Get(&console, buffer, &size, timeout)){
			  	case DRIVER_TIMEOUT:
			  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
			  		DRIVER_GSM_Flush(&gsm);
					breakFlag = 1;
					break;
			  	case DRIVER_ERROR:
			  		DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError received, not enough space for receiving characters! Please update your buffer! \r\n");
			  		DRIVER_GSM_Flush(&gsm);
					breakFlag = 1;
					break;
			  	case DRIVER_OK:
			  		if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end sending message */
			  		{
			  			DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Leaving command... \r\nMessage unsent!\r\n");
			  			DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
					}
			  		else
			  		{
			  			strcat((char*)topic,(const char*)buffer);
			  		}
			  		break;
			  	}

			  	if(breakFlag == 0)
			  	{
			  		MQTT_Subscribe(&mqtt,timeout,topic);
			  	}
		  }
		  else if(strstr((const char*)bufferConsole,(const char*)"ping\r") != NULL ||
				  strstr((const char*)bufferConsole,(const char*)"ping broker\r") != NULL ||
				  strstr((const char*)bufferConsole,(const char*)"ping req\r") != NULL ||
				  strstr((const char*)bufferConsole,(const char*)"ping request\r") != NULL)
		  {
				  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nPing broker...\r\n");
				  MQTT_PingReq(&mqtt,timeout);
		  }
		  else if(strstr((const char*)bufferConsole,(const char*)"establish tcpip\r") != NULL)
		  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nEstablishing TCPIP protocol ...\r\n");
		  		  EstablishTCPClientConnection(&gsmHandler, timeout);
		  }
		  else if(strstr((const char*)bufferConsole,(const char*)"mqtt client set\r") != NULL)
		  {
			  	DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nSettig mqtt client state...\r\n");

				/* Buffer for putting answer from gsm */
				uint8_t buffer[1000] = {0};

				/* Flag that indicates when error in receiving from console occured */
				uint8_t breakFlag = 0;

				/* How many characters are received from gsm-it's importent to be zero initialize */
				uint32_t size = 0;

				MQTTClientState_t state = MQTT_CLIENT_CLOSE;

				DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Enter which state you want:"
				  "\r\n 1 CLOSE STATE \r\n 2 LISTEN STATE \r\n ");
				DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");

				/* Response from user */
				uint8_t incorrectInput = 0;
				while(incorrectInput != 3)
				{
					/* function that request from user only number for answer */
					switch(onlyPutNumber(&console, buffer, &size, sizeof(buffer), timeout)){
					case DRIVER_TIMEOUT:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_ERROR:
						DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
						DRIVER_GSM_Flush(&gsm);
						breakFlag = 1;
						break;
					case DRIVER_OK:
						if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						break;
					}

					/* Exit this function and wait for another command */
					if(breakFlag == 1) break;

					/* Ask if input is expected  */
					if(*buffer == '1') {state = MQTT_CLIENT_CLOSE; break;}
					else if(*buffer == '2') {state = MQTT_CLIENT_LISTEN; break;}
					else
					{

						if(incorrectInput == 2)
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
							DRIVER_GSM_Flush(&gsm);
							breakFlag = 1;
							break;
						}
						else
						{
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\nError! Enter number 1 or 2!\r\n ");
							DRIVER_CONSOLE_Put(&console,(const uint8_t*)">>");
							memset(buffer,0,sizeof(buffer));
							size = 0;
							incorrectInput++;
						}
					}
				}
				if(breakFlag == 0)
				{
					MQTT_CLIENT_SetState(&mqttCient,state);
				}
		  }
		  else if(strstr((const char*)bufferConsole,(const char*)"main menu\r") != NULL ||
				  strstr((const char*)bufferConsole,(const char*)"help\r") != NULL)
		  {
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n available commands:\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"set echo - set echo on or off!\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"set message format - it's necessary to set format before sending messages!\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"test storage - see which storages are available!\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"Set storage - it's necessary to set storage to be able to receive, send or delete messages!\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"list messages - list al types of messages currently in storages!\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"read message - read message using index of message parameter!\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"delete message - delete message using index of message parameter or delete all messages of specific type!\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"send message - send message from storage or directly!\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"store message - store message in storage!\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Commands to directly communicate with gsm modul:\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"read - read buffer for receving characters from gsm\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"flush - set buffer for receiving characters from gsm to initial state\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"*it's possible to directly send at commands to gsm module - just type the command in console  and run command \"read\" after \r\n");
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
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"set packet format - set type of TCPIP packet format(hexadecimal or decimal)\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"connect to server - connect with server with specified PDP context\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"disconnect from server - disconnect from server\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"check server connection - check status of connection with server\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"send data to server - send data to server\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Command for mqtt protocol(only use this commands when you set connection to mobile network and connection to specified server/broker!): \r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"connect to broker - connect command sent to broker\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"disconnect from broker - disconnect command sent to broker\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"publish - publish message to the topic on specified broke\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"subscribe - subscribe to topic on specified broker\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"ping/ping broker/ping req/ping request - send ping request to broker and wait for ping response from broker\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"establish tcpip - does 4 commands: turn on mobile network, active pdp, connect to server and set packet format of TCPIP connection\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"\r\n Command that implement mqtt client:\r\n");
		  		  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"mqtt client set - set state of mqtt client(either can be CLOSE or LISTEN)\r\n");
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
				  uint8_t *startMsg = (uint8_t *)strstr((char*)bufferGsm,(const char*)mqtt.mqttPacket.payload.topicName);
				  if(startMsg != NULL)
				  {
					  startMsg = startMsg + mqtt.mqttPacket.payload.topicLen;
					  DRIVER_CONSOLE_Put(&console, startMsg);
				  }
				  else
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
			  DRIVER_CONSOLE_Put(&console,(const uint8_t*)"Mistake in writing command. Please, write command correctly or type \"help\" for support!\r\n");
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
