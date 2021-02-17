/**
  **************************************************************************************************
  * @file    mqtt_cient.c
  * @author  Valentina Denic
  * @brief   Mqtt client implementation.
  *          This file provides firmware functions to manage the following
  *          functionalities of the console.
  *           + Initialization function
  *           + Set state of client function
  *
  @verbatim
 ===================================================================================================
                        ##### How to use this driver #####
 ===================================================================================================
  [..]
    The mqtt protocol driver can be used as follows:

    (#) Declare a MQTTClientHandler_t handle structure (eg. MQTTClientHandler_t mqttClient).
    (#) Initialize the mqtt client low level resources by implementing the MQTT_CLIENT_Init()
    (#) Set state of client using MQTT_CLIENT_SetState() function
    	-State can be either BLOCK STATE or LISTEN STATE. Using MQTTClientState_t enum, you
    	 can set desirable state.

  @endverbatim
  *
  **************************************************************************************************
  */

/* Includes ---------------------------------------------------------------------------------------*/
#include <mqtt_client.h>

/* Console queue handle for transmiting messages*/
QueueHandle_t GsmQueueTransmit;

/* Console queue handle for receiving messages*/
QueueHandle_t mqttClientQueue;

/*  Wait Message task declaration */
void WaitMessageTask(void* pvParameters);

/**
  * @brief Initialize MQTT handler to contains gsm modul and console for use.
  * @param handler      MQTT CLIENT handle.
  * @param config       Configuration handle.
  * @retval MQTTClientType_t status
  */
MQTTClientType_t MQTT_CLIENT_Init(MQTTClientHandler_t *handler, MQTTClientConfig_t *config)
{
	/* When we don't have any handler to initalize current handle, exit and return error */
	if(handler == NULL || config == NULL || config->gsm == NULL || config->console == NULL)
		return MQTT_CLIENT_ERROR;

	mqttClientQueue = xQueueCreate( QUEUELENGTH, sizeof(MQTTClientMsg_t) );
	if( mqttClientQueue == NULL )
	{
		/* The queue could not be created. */
		return MQTT_CLIENT_ERROR;
	}

	if(xTaskCreate(WaitMessageTask,"WaitMessageTask", 2048,( void *) handler,2,NULL) == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY)
	{
		/* The task could not be created. */
		return MQTT_CLIENT_ERROR;
	}

	handler->gsm 				= config->gsm;

	handler->console			= config->console;

	handler->mqtt				= config->mqtt;

	handler->mqttClientQueue 	= mqttClientQueue;

	handler->state 				= MQTT_CLIENT_CLOSE;

	return MQTT_CLIENT_OK;
}

/**
  * @brief Publish message to a specific topic in broker.
  * @param handler	   	Handle that contains everything about mqtt client(which gsm will be used and console and mqtt file).
  * @retval MQTTClientType_t status
  */
MQTTClientType_t MQTT_CLIENT_SetState(MQTTClientHandler_t *handler, MQTTClientState_t state)
{
	MQTTClientMsg_t queueState;

	queueState.state = state;

	xQueueSend(handler->mqttClientQueue,(void*) &queueState, 0);

	return DRIVER_OK;
}


/**
  * @brief Task for waiting message from broker
  */
void WaitMessageTask(void* pvParameters)
{
	MQTTClientHandler_t * handler = pvParameters;
	/* State of mqtt clent- it can be blocking state or listening,
	 * so we set queue's blocking period with this parameter */
	uint32_t blockPeriod = MQTT_CLIENT_BLOCK_INFINITY;
	uint8_t buffer[4000] = {0};
	uint32_t size = 0;
	MQTTClientMsg_t msg;
	uint32_t sizeMsg = 0;
	uint8_t firstTimeTopicFlag = 0;
	for(;;)
	{
		xQueueReceive(mqttClientQueue, &msg, blockPeriod);
		switch(msg.state){
		case MQTT_CLIENT_LISTEN:
			blockPeriod = MQTT_CLIENT_NO_BLOCK;
			/* Read what is in buffer */
			DRIVER_GSM_Read(handler->gsm,buffer, &size);
			/* If topic name occurs in buffer, we have to set size of message that is sent from broker */
			if(strstr((char*)buffer,(const char*)handler->mqtt->mqttPacket.payload.topicName) != NULL)
			{
				/* Only when we reaches this if first time (that means that we have in buffer topic name and
				 * we now that after topic name goes message characters), we need to set size of message */
				if(firstTimeTopicFlag == 0)
				{
					firstTimeTopicFlag = 1;
					uint8_t *startPacket = (uint8_t*)strchr((char*)buffer,'0');
					sizeMsg = *(startPacket + 1)- 2 - handler->mqtt->mqttPacket.payload.topicLen;
				}
			}

			/* If we setted size of message, we need to wait for full message to arrive */
			if(firstTimeTopicFlag == 1)
			{
				/* Check if all message arrived (we now size of message and expect that much characters) */
				uint8_t *startMsg = (uint8_t*)strstr((char*)buffer,(const char*)handler->mqtt->mqttPacket.payload.topicName);
				startMsg = startMsg + handler->mqtt->mqttPacket.payload.topicLen;
				uint8_t *endMsg = (uint8_t*)strchr((char*)buffer,'\0');
				if(endMsg - startMsg == sizeMsg)
				{
					DRIVER_CONSOLE_Put(handler->console, startMsg);
					DRIVER_CONSOLE_Put(handler->console, (const uint8_t*)"\r\n");
					firstTimeTopicFlag = 0;
					memset(buffer,0,sizeof(buffer));
					size = 0;
					sizeMsg = 0;
				}

			}
			break;
		case MQTT_CLIENT_CLOSE:
			blockPeriod = MQTT_CLIENT_BLOCK_INFINITY;
			memset(buffer,0,sizeof(buffer));
			size = 0;
			break;
		}
	}
}




