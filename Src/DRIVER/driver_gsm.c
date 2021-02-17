/**
  **************************************************************************************************
  * @file    driver_gsm.c
  * @author  Valentina Denic
  * @brief   GSM module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of the gsm module.
  *           + Initialization function
  *           + Read message from gsm module function
  *           + Write message to gsm module function
  *           + Bring receiving buffer for characters from gsm to initial state
  *           + Collect characters in uart interrupt routine
  *           + Receive message from gsm in receiving task
  *			  + Transmit message to gsm in transmitting task
  *
  @verbatim
 ===================================================================================================
                        ##### How to use this driver #####
 ===================================================================================================
  [..]
    The GSM module driver can be used as follows:

    (#) Declare a DRIVERGsmHandler_t handle structure (eg. DRIVERGsmHandler_t gsm).
    (#) Initialize the gsm low level resources by implementing the DRIVER_GSM_Init()
    (#) Read characters from gsm using DRIVER_GSM_Read() function
    (#) Put message to console using DRIVER_GSM_Write() function
    (#) Flush gsm and bring him to initial state with DRIVER_GSM_Flush() function
    (#) Collect characters from gsm in interrupt routine uart module with
    	IRQ_UART_RX_GSM() function

  @endverbatim
  *
  **************************************************************************************************
  */

/* Includes ---------------------------------------------------------------------------------------*/
#include <driver_gsm.h>

/* Current global GSM handle */
static volatile DRIVERGsmHandler_t *currentGsmHandle;

/* handler of circular buffer */
volatile DRIVERCircularBuffer_t currentCircularBufferGsm;

/* Additional helpful variables for debuging */
static volatile uint8_t IndexOfLastReceivedCharHLP;

/* Console queue handle for transmiting messages*/
QueueHandle_t GsmQueueTransmit;
/* Console queue handle for receiving messages*/
QueueHandle_t GsmQueueReceive;

/* Handle of RxTaskGsm for notifiying */
static TaskHandle_t RxTaskHandleGsm;

void TxTaskGsm(void* pvParameters);
void RxTaskGsm(void* pvParameters);

uint8_t rxStart[2000] = {0};
DRIVERGsmMsg_t queueMsg = {.startMsg = rxStart,.sizeMsg = 0};
DRIVERGsmMsg_t queueMsgRead;

/**
  * @brief Callback function when receiving new character from UART is done.
  * @param huart          UART handle.
  * @retval void
  */
void IRQ_UART_RX_GSM(UART_HandleTypeDef *huart)
{
	/* Circular buffer - write characters to buffer */
	if(huart->Instance == USART6){
		if((uint8_t)(huart->Instance->RDR & 0xFF) != '\0')
		{
			/* Set console state to receiving */
			currentGsmHandle->State = GSM_STATE_RECEIVE;

			if(currentCircularBufferGsm.pointerWrite == (currentCircularBufferGsm.pointerEnd + 1))
			{
				currentCircularBufferGsm.pointerWrite = currentCircularBufferGsm.pointerStart;
			}
			*currentCircularBufferGsm.pointerWrite = (uint8_t)(huart->Instance->RDR & 0xFF);
			currentCircularBufferGsm.pointerWrite++;

			/* Set console state to idle */
			currentGsmHandle->State = GSM_STATE_IDLE;
		}
	}
}

/**
  * @brief Initialize the GSM with the given configuration.
  * @param handler          GSM handle.
  * @param config 			Configuration handle.
  * @retval DRIVERState_t status
  */
DRIVERState_t DRIVER_GSM_Init(DRIVERGsmHandler_t *handler, DRIVERGsmConfig_t *config)
{
	/* Check the configuration handle allocation */
	if (config == NULL)
	{
	  return DRIVER_ERROR;
	}

	/* Check the configuration parameters initialization */
	if(config->rxBuffer == NULL)
	{
	 return DRIVER_ERROR;
	}

	/* Check the configuration UART initialization */
	if((*(config->UartInit))() == DRIVER_ERROR)
	{
		return DRIVER_ERROR;
	}

	GsmQueueTransmit = xQueueCreate( QUEUELENGTH, sizeof(DRIVERGsmMsg_t)  );
	if( GsmQueueTransmit == NULL )
	{
		/* The queue could not be created. */
		return DRIVER_ERROR;
	}

	GsmQueueReceive = xQueueCreate( QUEUELENGTH, sizeof(DRIVERGsmMsg_t) );
	if( GsmQueueReceive == NULL )
	{
		/* The queue could not be created. */
		return DRIVER_ERROR;
	}

	if(xTaskCreate(TxTaskGsm,"TxTaskGsm", 2048,( void *) handler,3,NULL) == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY)
	{
		/* The task could not be created. */
		return DRIVER_ERROR;
	}

	if(xTaskCreate(RxTaskGsm,"RxTaskGsm", 2048,( void *) handler,3,&RxTaskHandleGsm) == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY)
	{
		/* The task could not be created. */
			return DRIVER_ERROR;
	}

	memset((uint8_t*)handler->rxBuffer,0,handler->rxSize);

	/* Set handler fields */
	handler->rxBuffer 	= config->rxBuffer;

	handler->rxSize   	= config->rxSize;

	handler->uartBase 	= config->uartBase;

	handler->State 		= GSM_STATE_IDLE;

	handler->InitState 	= GSM_INIT;

    handler->uartBase->RxISR = IRQ_UART_RX_GSM;

	/* Circular buffer init */
	currentCircularBufferGsm.pointerStart 	= (uint8_t*)handler->rxBuffer;
	currentCircularBufferGsm.pointerEnd	 	= (uint8_t*)(handler->rxBuffer + handler->rxSize - 1);
	currentCircularBufferGsm.pointerWrite 	= (uint8_t*)handler->rxBuffer;
	currentCircularBufferGsm.pointerRead  	= (uint8_t*)handler->rxBuffer;

	handler->GsmQueueTransmit = GsmQueueTransmit;

	handler->GsmQueueReceive = GsmQueueReceive;

	currentGsmHandle 	= handler;

 	return DRIVER_OK;
}

/**
  * @brief Get characters from GSM module.
  * @param handler          GSM handle.
  * @param userBuffer       Buffer to put incoming characters .
  * @param timeout 			Timeout duration.
  * @param size 			Number of received characters.
  * @retval DRIVERState_t status
  */
DRIVERState_t DRIVER_GSM_Read(DRIVERGsmHandler_t *handler, uint8_t* userBuffer, uint32_t* size)
{

	/* Notify Rx task to send received answer from gsm */
	xTaskNotify(RxTaskHandleGsm,0,eNoAction);

	/* Wait Rx task to send a message that contains answer from gsm */
	xQueueReceive(handler->GsmQueueReceive, &queueMsgRead, portMAX_DELAY);

	/* When we aren't receive any character, just finish
	 * function */
	if(queueMsgRead.sizeMsg == 0) return DRIVER_OK;

	/* When answer from gsm is received, copy that messages into user buffer */
	uint32_t i = 0;
	uint32_t index = *size;
	while(i < queueMsgRead.sizeMsg)
	{
		userBuffer[index++] = queueMsgRead.startMsg[i++];
	}

	/* Set size of that answer and finish function */
	*size = index;

	return DRIVER_OK;

}

/**
  * @brief Put message to GSM module.
  * @param handler      GSM handle.
  * @param msg       	Message to send to GSM .
  * @retval DRIVERState_t status
  */
DRIVERState_t DRIVER_GSM_Write(DRIVERGsmHandler_t *handler, const uint8_t* msg, uint32_t msgSize)
{
	DRIVERGsmMsg_t queueMsg;

	queueMsg.startMsg = (uint8_t *) msg;
	queueMsg.sizeMsg = msgSize;

	xQueueSend(handler->GsmQueueTransmit,(void*) &queueMsg, 0);
	return DRIVER_OK;

}

/**
  * @brief Task for transmition message to gsm module
  */
void TxTaskGsm(void* pvParameters)
{
	DRIVERGsmHandler_t * handler = pvParameters;
	uint8_t txBuff[2000] = {0};
	for(;;)
	{
		DRIVERGsmMsg_t msg = {.startMsg = txBuff, .sizeMsg = 0};
		xQueueReceive(GsmQueueTransmit, &msg, portMAX_DELAY);
		HAL_UART_Transmit(handler->uartBase, msg.startMsg, msg.sizeMsg, HAL_MAX_DELAY);
	}
}

/**
  * @brief Task for receiving message from gsm module
  */
void RxTaskGsm(void* pvParameters)
{
	DRIVERGsmHandler_t * handler = pvParameters;
	uint32_t i = 0;
	for(;;)
	{
		xTaskNotifyWait( 0x00, /* Don't clear any notification bits on entry. */
		 ULONG_MAX, /* Reset the notification value to 0 on exit. */
		 NULL, /* Notified value is ommited. */
		 portMAX_DELAY ); /* Block indefinitely. */

		uint32_t j = 0;
		for(;j != sizeof(rxStart);j++) queueMsg.startMsg[j] = 0;
		queueMsg.sizeMsg = 0;
		i = 0;

		__HAL_UART_DISABLE_IT(handler->uartBase, UART_IT_RXNE);

		/* set size of message */
		if(currentCircularBufferGsm.pointerWrite >= currentCircularBufferGsm.pointerRead)
			queueMsg.sizeMsg = currentCircularBufferGsm.pointerWrite - currentCircularBufferGsm.pointerRead;
		else
			queueMsg.sizeMsg = (currentCircularBufferGsm.pointerEnd - currentCircularBufferGsm.pointerRead) + 1 +
			(currentCircularBufferGsm.pointerWrite - currentCircularBufferGsm.pointerStart);

		/* Copy whatever is in receiving buffer */
		while(currentCircularBufferGsm.pointerRead != currentCircularBufferGsm.pointerWrite)
		{
			queueMsg.startMsg[i++] = *currentCircularBufferGsm.pointerRead;
			*currentCircularBufferGsm.pointerRead = 0;
			if( currentCircularBufferGsm.pointerRead != currentCircularBufferGsm.pointerEnd)
			{
				currentCircularBufferGsm.pointerRead++;
			}
			else
			{
				currentCircularBufferGsm.pointerRead = currentCircularBufferGsm.pointerStart;
			}

		}
		__HAL_UART_ENABLE_IT(handler->uartBase, UART_IT_RXNE);
		/* Send message to queue */
		xQueueSend(GsmQueueReceive, (void *) &queueMsg, 0);

	}
}
/**
  * @brief Bring receiving buffer for gsm module to its initial state.
  * @param handler      GSM handle.
  * @retval DRIVERState_t status
  */
DRIVERState_t DRIVER_GSM_Flush(DRIVERGsmHandler_t *handler)
{
	currentCircularBufferGsm.pointerWrite = currentCircularBufferGsm.pointerStart;
	currentCircularBufferGsm.pointerRead  = currentCircularBufferGsm.pointerStart;

	memset((uint8_t*)currentGsmHandle->rxBuffer,0,currentGsmHandle->rxSize);

	return DRIVER_OK;
}
