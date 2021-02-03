/**
//  *******************************************************************************************************************
  * @file    driver_console.c
  * @author  Valentina Denic
  * @brief   CONSOLE module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of the console.
  *           + Initialization function
  *           + Get character from console function
  *           + Put character to console function
  *
  *
  @verbatim
 ======================================================================================================================
                        ##### How to use this driver #####
 ======================================================================================================================
  [..]
    The CONSOLE module driver can be used as follows:

    (#) Declare a DRIVERConsoleHandler_t handle structure (eg. DRIVERConsoleHandler_t console).
    (#) Initialize the console low level resources by implementing the DRIVER_CONSOLE_Init():
        (++) Initialize addresses of buffers to store character and to put character from console.
        (++) Initialize size of buffers to get and put characters.
        (++) Initialize addresses of function for reading character from console and writing character to console.
    (#) Get characters from console using DRIVER_CONSOLE_Get() function
    (#) Put characters to console using DRIVER_CONSOLE_Put() function
        in the huart handle AdvancedInit structure.

  @endverbatim
  *
  **********************************************************************************************************************
  */


/* Includes -----------------------------------------------------------------------------------------------------------*/
#include <driver_console.h>

volatile uint32_t msgCount = 0;
volatile bool buffFullFlag = false;
volatile bool backslashFlag = false;

/* handler of circular buffer */
volatile DRIVERCircularBuffer_t currentCircularBufferConsole;
/* Handle of RxTask for notifiying */
static TaskHandle_t RxTaskHandle;

/* Console queue handle for transmiting messages*/
QueueHandle_t ConsoleQueueTransmit;
/* Console queue handle for receiving messages*/
QueueHandle_t ConsoleQueueReceive;

void TxTask(void* pvParameters);
void RxTask(void* pvParameters);

uint8_t start = 0;
uint8_t *backspaceEcho = (uint8_t *)"\b \b";
uint8_t *backslashEcho = (uint8_t *)"\r\n";
uint8_t *msgOverflow = (uint8_t *)"Buffer is full, message discarded!";
/* Set queue array of messages received */
DRIVERConsoleMsg_t msg[10];
DRIVERConsoleMsg_t msgEcho;

DRIVERConsoleMsg_t msgGet;

void RxISRCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == USART3){
		if(currentCircularBufferConsole.pointerWrite == (currentCircularBufferConsole.pointerEnd + 1))
		{
			currentCircularBufferConsole.pointerWrite = currentCircularBufferConsole.pointerStart;
		}

		/* Check fullness of buffer */
		char *buffFull = strchr((char*)currentCircularBufferConsole.pointerStart,'\0');
		if(buffFull != NULL)
		{
			if((uint8_t)(huart->Instance->RDR & 0xFF) == BACKSPACE)
			{
				backslashFlag = true;
				if(*(currentCircularBufferConsole.pointerWrite - 1) != 0)
				{
					if(currentCircularBufferConsole.pointerWrite == currentCircularBufferConsole.pointerStart)
						currentCircularBufferConsole.pointerWrite = currentCircularBufferConsole.pointerEnd;
					else
						currentCircularBufferConsole.pointerWrite--;
					*currentCircularBufferConsole.pointerWrite = 0;
				}
			}
			else
			{
				*currentCircularBufferConsole.pointerWrite = (uint8_t)(huart->Instance->RDR & 0xFF);
				currentCircularBufferConsole.pointerWrite++;
			}
		}
		else buffFullFlag = true;

		/* Number of received messages */
		if((uint8_t)(huart->Instance->RDR & 0xFF) == '\r') msgCount++;

		/* Notify rx task about new character received */
		xTaskNotifyFromISR(RxTaskHandle,0,eNoAction,NULL);
	}
}

/**
  * @brief Initialize the CONSOLE with the given configuration.
  * @param handler          CONSOLE handle.
  * @param config 			Configuration handle.
  * @retval DRIVERState_t status
  */
DRIVERState_t DRIVER_CONSOLE_Init(DRIVERConsoleHandler_t *handler, DRIVERConsoleConfig_t *config)
{
	/* Check the configuration handle allocation */
	if (config == NULL)
	{
		return DRIVER_ERROR;
	}

	/* Check the configuration parameters initialization */
	if(config->txBuffer == NULL || config->rxBuffer == NULL || config->UartInit == NULL)
	{
		return DRIVER_ERROR;
	}

	/* Check the configuration UART initialization */
	if((*(config->UartInit))() == DRIVER_ERROR)
	{
		return DRIVER_ERROR;
	}

	handler->txBuffer = config->txBuffer;

	handler->rxBuffer = config->rxBuffer;

	handler->txSize = config->txSize;

	handler->rxSize = config->rxSize;

	handler->State = COSNOLE_STATE_IDLE;

	handler->InitState = CONSOLE_INIT;

	handler->uartBase 	= config->uartBase;

	handler->uartBase->RxISR = RxISRCallback;

	memset((uint8_t*)handler->rxBuffer,0,handler->rxSize);

	memset((uint8_t*)handler->txBuffer,0,handler->txSize);

	/* Circular buffer init */
	currentCircularBufferConsole.pointerStart 	= (uint8_t*)handler->rxBuffer;
	currentCircularBufferConsole.pointerEnd	 	= (uint8_t*)(handler->rxBuffer + handler->rxSize - 1);
	currentCircularBufferConsole.pointerWrite 	= (uint8_t*)handler->rxBuffer;
	currentCircularBufferConsole.pointerRead  	= (uint8_t*)handler->rxBuffer;

	ConsoleQueueTransmit = xQueueCreate( QUEUELENGTH, sizeof(DRIVERConsoleMsg_t)  );
	if( ConsoleQueueTransmit == NULL )
	{
		/* The queue could not be created. */
		return DRIVER_ERROR;
	}

	ConsoleQueueReceive = xQueueCreate( QUEUELENGTH, sizeof(DRIVERConsoleMsg_t) );
	if( ConsoleQueueReceive == NULL )
	{
		/* The queue could not be created. */
		return DRIVER_ERROR;
	}

	handler->ConsoleQueueTransmit = ConsoleQueueTransmit;

	handler->ConsoleQueueReceive = ConsoleQueueReceive;

	if(xTaskCreate(TxTask,"TxTask", 1024,( void * ) handler,3,NULL) == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY )
		return DRIVER_ERROR;
	if(xTaskCreate(RxTask,"RxTask", 1024,( void * ) handler,3,&RxTaskHandle) == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY )
		return DRIVER_ERROR;

	return DRIVER_OK;
}

/**
  * @brief Put characters to CONSOLE.
  * @param handler          CONSOLE handle.
  * @param string       	String to put char by char to console .
  * @retval DRIVERState_t status
  */
DRIVERState_t DRIVER_CONSOLE_Put(DRIVERConsoleHandler_t *handler, const uint8_t *string)
{
	DRIVERConsoleMsg_t msgPut = {.startMsg = (uint8_t *)string, .sizeMsg = 0};

	/* Find size of message for transmiting string to gsm */
	uint32_t i = 0;
	for(;string[i] != 0;i++);
	msgPut.sizeMsg = i;

	/* Send string via queue to task for transmiting to gsm */
	xQueueSend(handler->ConsoleQueueTransmit,(void*) &msgPut, 0);
	return DRIVER_OK;
}

/**
  * @brief Get characters from CONSOLE.
  * @param handler          CONSOLE handle.
  * @param UserBuffer       Buffer to put incoming characters .
  * @param dataSize			Number of received characters.
  * @param timeout 			Timeout duration.
  * @retval DRIVERState_t status
  */
DRIVERState_t DRIVER_CONSOLE_Get(DRIVERConsoleHandler_t *handler, uint8_t *userBuffer, uint32_t* dataSize, uint32_t timeout)
{
	uint32_t i = 0;
	if(xQueueReceive(handler->ConsoleQueueReceive, &msgGet, timeout) == pdFALSE)
		return DRIVER_TIMEOUT;

	if(strcmp((char*)msgGet.startMsg,"Buffer is full, message discarded!") == 0)
	{
		strcpy((char*)userBuffer,(const char*)msgGet.startMsg);
		return DRIVER_ERROR;
	}

	*dataSize = msgGet.sizeMsg;

	/* Copy whatever is in receiving buffer */
	while(currentCircularBufferConsole.pointerRead != currentCircularBufferConsole.pointerWrite)
	{
		userBuffer[i++] = *currentCircularBufferConsole.pointerRead;
		*currentCircularBufferConsole.pointerRead = 0;
		if( currentCircularBufferConsole.pointerRead != currentCircularBufferConsole.pointerEnd)
		{
			currentCircularBufferConsole.pointerRead++;
		}
		else
		{
			currentCircularBufferConsole.pointerRead = currentCircularBufferConsole.pointerStart;
		}
	}

	return DRIVER_OK;
}

/**
  * @brief Task for transmition message to console
  */
void TxTask(void* pvParameters){
	DRIVERConsoleHandler_t * handler = pvParameters;
	uint8_t txBuff[2000] = {0};
	DRIVERConsoleMsg_t msgTx;
	for(;;){

		xQueueReceive(ConsoleQueueTransmit, &msgTx, portMAX_DELAY);
		memcpy(txBuff,msgTx.startMsg ,msgTx.sizeMsg);
		handler->State = COSNOLE_STATE_TRANSMIT;
		HAL_UART_Transmit(handler->uartBase, msgTx.startMsg, msgTx.sizeMsg, HAL_MAX_DELAY);
		handler->State = COSNOLE_STATE_IDLE;
	}
}

/**
  * @brief Task for receiving characters from console
  */
void RxTask(void* pvParameters){
	DRIVERConsoleHandler_t * handler = pvParameters;
	uint8_t i = 0;
	for(;;){

		xTaskNotifyWait( 0x00, /* Don't clear any notification bits on entry. */
		 ULONG_MAX, /* Reset the notification value to 0 on exit. */
		 NULL, /* Notified value is ommited. */
		 portMAX_DELAY ); /* Block indefinitely. */

		__HAL_UART_DISABLE_IT(handler->uartBase, UART_IT_RXNE);

		/* Return character to console(Echo message) */
		msgEcho.sizeMsg = 0;
		if(*(currentCircularBufferConsole.pointerWrite - 1) != '\r')
		{
			if(backslashFlag == true)
			{
				/* Send backspace echo to console */
				msgEcho.startMsg = backspaceEcho;
				msgEcho.sizeMsg = 3;
				backslashFlag = false;
			}
			else
			{
				/* Send received character echo to console */
				start = *(currentCircularBufferConsole.pointerWrite - 1);
				msgEcho.startMsg = &start;
				msgEcho.sizeMsg = 1;
			}
		}
		else
		{
			/* Send backslash echo to console */
			msgEcho.startMsg = backslashEcho;
			msgEcho.sizeMsg = 2;
		}

		/* Write received character to console */
		handler->State = COSNOLE_STATE_RECEIVE;
		xQueueSend(handler->ConsoleQueueTransmit,(void*) &msgEcho, 0);
		handler->State = COSNOLE_STATE_IDLE;

		i = 0;
		/* If buffer is full and no other messages are in it */
		if(buffFullFlag == true && msgCount == 0)
		{
			msgEcho.startMsg = msgOverflow;
			msgEcho.sizeMsg = sizeof(msgOverflow);
			/* Reset rx buffer */
			memset(handler->rxBuffer,0,handler->rxSize);
			buffFullFlag = false;
			xQueueSend(ConsoleQueueReceive, (void *) &msg[i], 0);
		}

		/*  if there is messages to process */
		while(msgCount > 0)
		{
			if(i > 10)
			{
				i = 0;
				msgEcho.startMsg = msgOverflow;
				msgEcho.sizeMsg = sizeof(msgOverflow);
			}
			else
			{
				/* set size of message */
				if(currentCircularBufferConsole.pointerWrite > currentCircularBufferConsole.pointerRead)
					msg[i].sizeMsg = currentCircularBufferConsole.pointerWrite - currentCircularBufferConsole.pointerRead;
				else
					msg[i].sizeMsg = (currentCircularBufferConsole.pointerEnd - currentCircularBufferConsole.pointerRead) + 1 +
					(currentCircularBufferConsole.pointerWrite - currentCircularBufferConsole.pointerStart);

				/* Decrease number of messages */
				msgCount--;

				msg[i].startMsg = currentCircularBufferConsole.pointerRead;

			}
			/* Send message to queue */
			xQueueSend(ConsoleQueueReceive, (void *) &msg[i], 0);
			i++;
		}

		__HAL_UART_ENABLE_IT(handler->uartBase, UART_IT_RXNE);

	}
}
