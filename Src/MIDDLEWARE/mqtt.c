/**
  **************************************************************************************************
  * @file    mqtt.c
  * @author  Valentina Denic
  * @brief   Mqtt protocol driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of the console.
  *           + Initialization function
  *           + Connect to broker function
  *           + Disconnect from broker function
  *           + Publish message to topic on broker function
  *           + Subscribe to topic on broker function
  *           + Function that set hexadecimal format of sending TCPIP packet(needed in mqtt protocol)
  *           + Ping server function
  *           + Additional function for number base convertion
  *
  @verbatim
 ===================================================================================================
                        ##### How to use this driver #####
 ===================================================================================================
  [..]
    The mqtt protocol driver can be used as follows:

    (#) Declare a MQTTHandler_t handle structure (eg. MQTTHandler_t mqtt).
    (#) Initialize the mqtt low level resources by implementing the MQTT_Init()
    (#) Connect to broker with MQTT_Connect() function
    (#) Disconnect from broker with MQTT_Disconnect() function
    (#) Set hexadecimal format of sending packets to broker with MQTT_SetHexFormat() function
    (#) Publish message to topic on connected broker with MQTT_Publish() function
    (#) Subscribe to the specified topic on broker with  MQTT_Subscribe() function
    (#) Ping server with PINGREQ Packet. The Server MUST send a PINGRESP Packet in response
     to a PINGREQ Packet. This is implemented using MQTT_PingReq() function.

  @endverbatim
  *
  **************************************************************************************************
  */

/* Includes ---------------------------------------------------------------------------------------*/
#include <mqtt.h>


/* Function to reverse arr[] from start to end */
/**
  * @brief Function which reverse elements in array.
  * @param array    Array which elemnts will be reversed.
  * @param start    Index of first element in array that will be inverted.
  * @param end      Index of last element in array that will be inverted.
  * @retval void
  */
void rverseArray(uint8_t array[], int start, int end)
{
    uint8_t temp;
    while (start < end)
    {
        temp = array[start];
        array[start] = array[end];
        array[end] = temp;
        start++;
        end--;
    }
}

/**
  * @brief Function for converting decimal number into hexadecimal.
  * @param array    Array in which convertion will be stored.
  * @param num    	Number that will be converted.
  * @retval uint32_t Number of elements in array.
  */
uint32_t convDecToHexchar(uint8_t *array, uint32_t *num)
{
	uint8_t i = 0;
	uint8_t weight = 16;
	uint8_t rest = *num % weight;
	uint8_t result = *num / weight;

	/* Determine the number  and store in buffer */
	while(1)
	{
		switch(rest){
			case 10:
				array[i] = 'a';
				break;
			case 11:
				array[i] = 'b';
				break;
			case 12:
				array[i] = 'c';
				break;
			case 13:
				array[i] = 'd';
				break;
			case 14:
				array[i] = 'e';
				break;
			case 15:
				array[i] = 'f';
				break;
			default:
				array[i] = rest + '0';
				break;
		}
		if(result == 0)
		{
			i++;
			break;
		}
		rest = result % weight;
		result /= weight;
		i++;
	}

	/* When number is less than f(15), then first 4 bits will
	 * be zeros(MSB bits), and second 4 bits will be that number */
	if(*num <= 15)
	{
		array[i++] = '0';
	}
	/* When number is greater than 255, we have to use 2 bytes for
	 * representation of that number, but if number is less than
	 * 4095 we dont use MSB bits of 2nd byte, yet we set zeros
	 * for that bits */
	if(*num >= 256 && *num <= 4095)
	{
		array[i++] = '0';
	}

	return i/2;
}

/**
  * @brief Function that add continuation bit in remaining length if it neccessery.
  * @param array     Array of bytes in decimal format.
  * @param byteNo    Number of bytes in remaining length (number of elements in parameter array).
  * @retval void
  */
void addCB(uint32_t *array, uint8_t byteNo)
{
	if(byteNo == 1) return;

	if(byteNo == 2) array[0] += 128;

	if(byteNo == 3)
	{
		array[0] += 128;
		array[1] += 128;
	}
	if(byteNo == 4)
	{
		array[0] += 128;
		array[1] += 128;
		array[2] += 128;
	}

}

/**
  * @brief Function for converting decimal number into base 128.
  * @param array    Array in which convertion will be stored.
  * @param num    	Number that will be converted.
  * @retval uint32_t Number of elements in array.
  */
uint32_t convDecToBase128(uint32_t *array, uint32_t *num)
{
	uint32_t i = 0;
	uint32_t weight = 128;
	uint32_t rest = *num % weight;
	uint32_t result = *num / weight;
	while(result != 0)
	{
		array[i] = rest;
		rest = result % weight;
		result /= weight;
		i++;
	}
	array[i] = rest;

	return i + 1;
}

/**
  * @brief Set hexadecimal format of sending packets to broker.
  * @param handler      MQTT handle.
  * @retval MQTTState_t status
  */
MQTTState_t MQTT_SetHexFormat(MQTTHandler_t *handler)
{
	uint8_t buffer[100];

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	uint32_t size = 0;

	/* Send command to send data to server */
	DRIVER_GSM_Write(handler->gsmHandler, (const uint8_t*)"at+cipsendhex=1\r", sizeof("at+cipsendhex=1\r"));

	/* Read response from gsm  and set response for user */
	/* Take current time and check if timeout occured */
	uint32_t tickstart = TIME_GetTick();
	uint8_t errorOkTimeout = 2;
	while(TIME_GetTick() - tickstart < 3000)
	{
		DRIVER_GSM_Read(handler->gsmHandler, buffer, &size);

		/* Badly received response from gsm */
		if(strstr((const char*)buffer,(const char*)"ERROR") != NULL)
		{
			errorOkTimeout = 1;
			break;
		}
		else if(strstr((const char*)buffer,(const char*)"OK") != NULL)
		{
			errorOkTimeout = 0;
			/* Successfully received response from gsm */
			break;
		}
	}

	/* Check response from gsm */
	switch(errorOkTimeout){
	case 2:
		return MQTT_TIMEOUT;
	case 1:
		return MQTT_ERROR;
	case 0:
		return MQTT_OK;
	}
	return MQTT_OK;
}

/**
  * @brief Initialize MQTT handler to contains gsm modul and console for use.
  * @param handler      MQTT handle.
  * @param config       Configuration handle.
  * @retval MQTTState_t status
  */
MQTTState_t MQTT_Init(MQTTHandler_t *handler, MQTTConfig_t *config)
{
	/* Check the configuration handle allocation */
	if (config == NULL)
	{
		handler->initState 	= MQTT_NO_INIT;
		return MQTT_ERROR;
	}

	handler->gsmHandler		 	= config->gsmHandler;

	handler->consoleHandler 	= config->consoleHandler;

	handler->initState 			= MQTT_INIT;

	handler->mqttPacket.variableHeader.packetID = 0;

	handler->mqttPacket.payload.topicLen = 0;

	memset(handler->mqttPacket.payload.topicName,0,sizeof(handler->mqttPacket.payload.topicName));

	return MQTT_OK;

}

/**
  * @brief Connect to broker command.
  * @param handler	   	Handle that contains everything about mqtt protocol (which gsm will be used and which console).
  * @retval MQTTState_t status
  */
MQTTState_t MQTT_Connect(MQTTHandler_t *handler)
{
	/* Buffer for reading message */
	uint8_t buffer[500] = {0};

	/* How many characters were received */
	uint32_t size = 0;

	/* Checking if user send correct gsm and console */
	if(handler->gsmHandler == NULL && handler->consoleHandler == NULL )
	{
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(handler->gsmHandler);

	/* Send command to send data to server */
	DRIVER_GSM_Write(handler->gsmHandler, (const uint8_t*)"at+cipsend\r", sizeof("at+cipsend\r"));

	/* Read response from gsm  and set response for user */
	/* Take current time and check if timeout occured */
	uint32_t tickstart = TIME_GetTick();
	uint8_t errorOkTimeout = 2;
	while(TIME_GetTick() - tickstart < 10000)
	{
		DRIVER_GSM_Read(handler->gsmHandler, buffer, &size);

		/* Badly received response from gsm */
		if(strstr((const char*)buffer,(const char*)"ERROR") != NULL)
		{
			errorOkTimeout = 1;
			break;
		}
		else if(strstr((const char*)buffer,(const char*)">") != NULL)
		{
			errorOkTimeout = 0;
			/* Successfully received response from gsm */
			break;
		}
	}

	/* Check response from gsm */
	switch(errorOkTimeout){
	case 2:
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_TIMEOUT;
	case 1:
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_ERROR;
	case 0:
		break;
	}

	/* Send command to send data to server */
	DRIVER_GSM_Write(handler->gsmHandler, (const uint8_t*)"10 0c 00 04 4d 51 54 54 04 02 0f 00 00 00 1a",
			sizeof("10 0c 00 04 4d 51 54 54 04 02 0f 00 00 00 1a"));

	/* Read response from gsm  and set response for user */
	/* Take current time and check if timeout occured */
	tickstart = TIME_GetTick();
	errorOkTimeout = 2;
	while(TIME_GetTick() - tickstart < 3000)
	{
		DRIVER_GSM_Read(handler->gsmHandler, buffer, &size);

		/* Badly received response from gsm */
		if(strstr((const char*)buffer,(const char*)"ERROR") != NULL)
		{
			errorOkTimeout = 1;
			break;
		}
		else if(strstr((const char*)buffer,(const char*)"OK") != NULL)
		{
			errorOkTimeout = 0;
			/* Successfully received response from gsm */
			break;
		}
	}

	/* Check response from gsm */
	switch(errorOkTimeout){
	case 2:
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_TIMEOUT;
	case 1:
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_ERROR;
	case 0:
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nSuccessfully connected to broker! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_OK;
	}
	return MQTT_OK;
}

/**
  * @brief Disconnect to broker.
  * @param handler	   	Handle that contains everything about mqtt protocol (which gsm will be used and which console).
  * @retval MQTTState_t status
  */
MQTTState_t MQTT_Disconnect(MQTTHandler_t *handler)
{
	/* Buffer for reading message */
	uint8_t buffer[500] = {0};

	/* How many characters were received */
	uint32_t size = 0;

	/* Checking if user send correct gsm and console */
	if(handler->gsmHandler == NULL && handler->consoleHandler == NULL )
	{
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(handler->gsmHandler);

	/* Send command to send data to server */
	DRIVER_GSM_Write(handler->gsmHandler, (const uint8_t*)"at+cipsend\r", sizeof("at+cipsend\r"));

	/* Read response from gsm  and set response for user */
	/* Take current time and check if timeout occured */
	uint32_t tickstart = TIME_GetTick();
	uint8_t errorOkTimeout = 2;
	while(TIME_GetTick() - tickstart < 10000)
	{
		DRIVER_GSM_Read(handler->gsmHandler, buffer, &size);

		/* Badly received response from gsm */
		if(strstr((const char*)buffer,(const char*)"ERROR") != NULL)
		{
			errorOkTimeout = 1;
			break;
		}
		else if(strstr((const char*)buffer,(const char*)">") != NULL)
		{
			errorOkTimeout = 0;
			/* Successfully received response from gsm */
			break;
		}
	}

	/* Check response from gsm */
	switch(errorOkTimeout){
	case 2:
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_TIMEOUT;
	case 1:
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_ERROR;
	case 0:
		break;
	}

	/* Send command to send data to server */
	DRIVER_GSM_Write(handler->gsmHandler, (const uint8_t*)"e0 00 1a", sizeof("e0 00 1a"));

	/* Read response from gsm  and set response for user */
	/* Take current time and check if timeout occured */
	tickstart = TIME_GetTick();
	errorOkTimeout = 2;
	while(TIME_GetTick() - tickstart < 3000)
	{
		DRIVER_GSM_Read(handler->gsmHandler, buffer, &size);

		/* Badly received response from gsm */
		if(strstr((const char*)buffer,(const char*)"ERROR") != NULL)
		{
			errorOkTimeout = 1;
			break;
		}
		else if(strstr((const char*)buffer,(const char*)"OK") != NULL)
		{
			errorOkTimeout = 0;
			/* Successfully received response from gsm */
			break;
		}
	}

	/* Check response from gsm */
	switch(errorOkTimeout){
	case 2:
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_TIMEOUT;
	case 1:
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_ERROR;
	case 0:
		if(handler->mqttPacket.variableHeader.packetID != 0) handler->mqttPacket.variableHeader.packetID--;
		else handler->mqttPacket.variableHeader.packetID = 0;
		handler->mqttPacket.payload.topicLen = 0;
		memset(handler->mqttPacket.payload.topicName,0,sizeof(handler->mqttPacket.payload.topicName));
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nSuccessfully disconnected from broker! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_OK;
	}
	return MQTT_OK;
}

/**
  * @brief Publish message to a specific topic in broker.
  * @param handler	   	Handle that contains everything about mqtt protocol (which gsm will be used and which console).
  * @param timeout      Timeout period for console.
  * @param topicName    Name of topic.
  * @param message      Message that will be published to the topic.
  * @retval DRIVERState_t status
  */
MQTTState_t MQTT_Publish(MQTTHandler_t *handler, uint32_t timeout,const uint8_t *topicName, const uint8_t *message){

	/* Buffer for reading message */
	uint8_t buffer[2000] = {0};

	/* How many characters were received */
	uint32_t size = 0;

	if(topicName == NULL || message == NULL)
	{
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\n Error: incorrect input arguments! Please try again with correct arguments!\r\n");
		return DRIVER_ERROR;
	}
	/* Checking if user send correct gsm and console */
	if(handler->gsmHandler == NULL && handler->consoleHandler == NULL )
	{
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(handler->gsmHandler);

	/** Set topic **/

	uint32_t topic[2000] 	= {0};
	uint8_t topicHex[2000] 	= {0};
	uint8_t topicLen[10] 	= {0};
	uint8_t topicHexByteNo 	= 0;
	uint32_t topicLenDec	= strlen((const char*)topicName); // length of topic in decimal number
	uint32_t i 				= 0;
	uint32_t indexHexTopic	= 0;

	if(strchr((char*)topicName,'\r') != NULL )
	{	/* Length of topic characters without '\r' */
		topicLenDec--;
	}

	/* Convert ASCII code to integer value */
	for(; i != topicLenDec; i++)
	{
		topic[i] = topicName[i] - '\0';
	}

	i = 0;
	for(;i != topicLenDec; i++)
	{
		topicHexByteNo = convDecToHexchar(topicHex + indexHexTopic, &topic[i]);

        /* reverse order of characters in array */
        rverseArray(topicHex + indexHexTopic, 0, topicHexByteNo);
        indexHexTopic += 2;
        topicHex[indexHexTopic++] = ' ';
	}

	/* Convert size from integer type to characters, base 16 */
    uint8_t topicByteNo = convDecToHexchar(topicLen, &topicLenDec);

    /* reverse order of characters in array */
    rverseArray(topicLen, 0, topicByteNo*2 - 1);

    /* Set message to publish */

	uint32_t msg[2000] 		= {0};
	uint8_t msgHex[2000] 	= {0};
	uint8_t msgLen[10] 		= {0};
	uint8_t msgHexByteNo 	= 0;
    uint32_t msgLenDec 		= strlen((const char*)message);
	uint32_t indexHexMsg	= 0;
	i 						= 0;

	if(strchr((char*)message,'\r') != NULL )
	{	/* Length of message characters without '\r' */
		msgLenDec--;
	}

	/* Convert ASCII code to integer value */
	for(; i != msgLenDec; i++)
	{
		msg[i] = message[i] - '\0';
	}

	i = 0;
	for(;i != msgLenDec; i++)
	{
		msgHexByteNo = convDecToHexchar(msgHex + indexHexMsg, &msg[i]);

        /* reverse order of characters in array */
        rverseArray(msgHex + indexHexMsg, 0, msgHexByteNo);
        indexHexMsg += 2;
        msgHex[indexHexMsg++] = ' ';
	}

	/* Convert size from integer type to characters, base 16 */
    uint8_t msgByteNo = convDecToHexchar(msgLen,&msgLenDec);

    /* reverse order of characters in array */
    rverseArray(msgLen, 0, msgByteNo*2 - 1);

    /* Set remaining length */
    uint32_t remainLen128[10] = {0};
    uint32_t remainLenDec = topicLenDec + msgLenDec + 2; /* need to be added plus 2 if we are using length of message when we are sending */

    uint8_t byteNo = convDecToBase128(remainLen128, &remainLenDec);
    addCB(remainLen128, byteNo);
    uint8_t index128 = 0;
    uint8_t index16 = 0;
    uint8_t n = 0;
    uint8_t remainLen16[20];
    memset(remainLen16,0,sizeof(remainLen16));
    for(; index128 != byteNo; index128++)
    {
    	n = convDecToHexchar(remainLen16 + index16,&remainLen128[index128]);
        /* reverse order of characters in array */
        rverseArray(remainLen16 + index16, 0, n);
    	index16 += 2;
    	remainLen16[index16++] = ' ';
    }

    /* Set message to send to server */
	uint8_t msgToSend[4000];
	uint32_t msgSize = 0;
	memset(msgToSend,0,sizeof(msgToSend));

	/* Set control field */
	strcat((char*)msgToSend,(const char*)"30 ");
	msgSize += 3;

	/* Set remaining length */
	strcat((char*)msgToSend,(const char*)remainLen16);
	msgSize += index16;

	/* Set topic length */
	if(topicByteNo == 1)
	{
		msgToSend[msgSize++] = '0';
		msgToSend[msgSize++] = '0';
		msgToSend[msgSize++] = ' ';
		strcat((char*)msgToSend,(const char*)topicLen);
		msgSize += 2;
	}
	if(topicByteNo == 2)
	{
		msgToSend[msgSize++] = topicLen[0];
		msgToSend[msgSize++] = topicLen[1];
		msgToSend[msgSize++] = ' ';
		msgToSend[msgSize++] = topicLen[2];
		msgToSend[msgSize++] = topicLen[3];
	}

	msgToSend[msgSize++] = ' ';

	/* Set topic name */
	strcat((char*)msgToSend,(const char*)topicHex);
	msgSize += indexHexTopic;

//	/* Set message length */
//	if(msgByteNo == 1)
//	{
//		msgToSend[msgSize++] = '0';
//		msgToSend[msgSize++] = '0';
//		msgToSend[msgSize++] = ' ';
//		strcat((char*)msgToSend,(const char*)msgLen);
//		msgSize += 2;
//		msgToSend[msgSize++] = ' ';
//	}
//	if(msgByteNo == 2)
//	{
//		msgToSend[msgSize++] = msgLen[0];
//		msgToSend[msgSize++] = msgLen[1];
//		msgToSend[msgSize++] = ' ';
//		msgToSend[msgSize++] = msgLen[2];
//		msgToSend[msgSize++] = msgLen[3];
//		msgToSend[msgSize++] = ' ';
//	}

	/* Set message */
	strcat((char*)msgToSend,(const char*)msgHex);
	msgSize += indexHexMsg;

	msgToSend[msgSize++] = '1';
	msgToSend[msgSize++] = 'a';

	/******   SEND COMMAND FOR TCP/IP SENDING DATA   *****/
	/* Send command to send data to server */
	DRIVER_GSM_Write(handler->gsmHandler, (const uint8_t*)"at+cipsend\r", sizeof("at+cipsend\r"));

	/* Read response from gsm  and set response for user */
	/* Take current time and check if timeout occured */
	uint32_t tickstart = TIME_GetTick();
	uint8_t errorOkTimeout = 2;
	while(TIME_GetTick() - tickstart < 3000)
	{
		DRIVER_GSM_Read(handler->gsmHandler, buffer, &size);

		/* Badly received response from gsm */
		if(strstr((const char*)buffer,(const char*)"ERROR") != NULL)
		{
			errorOkTimeout = 1;
			break;
		}
		else if(strstr((const char*)buffer,(const char*)">") != NULL)
		{
			errorOkTimeout = 0;
			/* Successfully received response from gsm */
			break;
		}
	}

	/* Check response from gsm */
	switch(errorOkTimeout){
	case 2:
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_TIMEOUT;
	case 1:
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_ERROR;
	case 0:
		break;
	}

	/******    SEND PUBLISH COMMAND      ********/
	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	size = 0;

	/* Publish message on the topic to server */
	DRIVER_GSM_Write(handler->gsmHandler, msgToSend, msgSize);
	// 30 13 00 03 67 73 6d 00 05 48 45 4c 4c 4f 1a - -t "gsm" -m "HELLO" -- test!

	/* Read response from gsm  and set response for user */
	/* Take current time and check if timeout occured */
	tickstart = TIME_GetTick();
	errorOkTimeout = 2;
	while(TIME_GetTick() - tickstart < 10000)
	{
		DRIVER_GSM_Read(handler->gsmHandler, buffer, &size);

		/* Badly received response from gsm */
		if(strstr((const char*)buffer,(const char*)"ERROR") != NULL)
		{
			errorOkTimeout = 1;
			break;
		}
		else if(strstr((const char*)buffer,(const char*)"OK") != NULL)
		{
			errorOkTimeout = 0;
			/* Successfully received response from gsm */
			break;
		}
	}

	/* Check response from gsm */
	switch(errorOkTimeout){
	case 2:
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_TIMEOUT;
	case 1:
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_ERROR;
	case 0:
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nMessage published on the specified topic! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_OK;
	}
	return MQTT_OK;

}

/**
  * @brief Subscribe to broker on specified topic.
  * @param handler	   	Handle that contains everything about mqtt protocol (which gsm will be used and which console).
  * @param timeout      Timeout period for console.
  * @param topicName    Name of topic.
  * @retval DRIVERState_t status
  */
MQTTState_t MQTT_Subscribe(MQTTHandler_t *handler, uint32_t timeout, uint8_t *topicName)
{
	/* Buffer for reading message */
	uint8_t buffer[2000] = {0};

	/* How many characters were received */
	uint32_t size = 0;

	/* Checking if user send correct gsm and console */
	if(handler->gsmHandler == NULL && handler->consoleHandler == NULL )
	{
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(handler->gsmHandler);

	uint8_t topicASCII[2000] = {0};
	uint32_t topic[2000] 	= {0};
	uint8_t topicHex[3000] 	= {0};
	uint8_t topicLen[10] 	= {0};
	uint8_t topicHexByteNo 	= 0;
	uint32_t topicLenDec	= strlen((const char*)topicName); // length of topic in decimal number
	uint32_t i 				= 0;
	uint32_t indexHexTopic	= 0;

	if(strchr((char*)topicName,'\r') != NULL )
	{	/* Length of topic characters without '\r' */
		topicLenDec--;
	}

	/* Convert ASCII code to integer value */
	for(; i != topicLenDec; i++)
	{
		topic[i] = topicName[i] - '\0';
		topicASCII[i] = topicName[i];
	}

	i = 0;
	for(;i != topicLenDec; i++)
	{
		topicHexByteNo = convDecToHexchar(topicHex + indexHexTopic, &topic[i]);

        /* reverse order of characters in array */
        rverseArray(topicHex + indexHexTopic, 0, topicHexByteNo);
        indexHexTopic += 2;
        topicHex[indexHexTopic++] = ' ';
	}

	/* Convert size from integer type to characters, base 16 */
    uint8_t topicByteNo = convDecToHexchar(topicLen, &topicLenDec);

    /* reverse order of characters in array */
    rverseArray(topicLen, 0, topicByteNo*2 - 1);

    /* Convert packetID from decimal to hexadecimal format */
	handler->mqttPacket.variableHeader.packetID++;
	uint8_t packeIDHex[10] 		= {0};

	/* Convert size from integer type to characters, base 16 */
    uint8_t PacketIDHexByteNo = convDecToHexchar(packeIDHex, &handler->mqttPacket.variableHeader.packetID);

    /* reverse order of characters in array */
    rverseArray(packeIDHex, 0, PacketIDHexByteNo*2 - 1);

    /* Set remaining length */
    uint32_t remainLen128[10] = {0};
    uint32_t remainLenDec = topicLenDec + 2 + 2 + 1; // first 2 is for packet ID number of bytes and 2nd 2 is for length of topic, plus 1 is for requested  QoS byte

    uint8_t byteNo = convDecToBase128(remainLen128, &remainLenDec);
    addCB(remainLen128, byteNo);
    uint8_t index128 = 0;
    uint8_t index16 = 0;
    uint8_t n = 0;
    uint8_t remainLen16[20];
    memset(remainLen16,0,sizeof(remainLen16));
    for(; index128 != byteNo; index128++)
    {
    	n = convDecToHexchar(remainLen16 + index16,&remainLen128[index128]);
        /* reverse order of characters in array */
        rverseArray(remainLen16 + index16, 0, n);
    	index16 += 2;
    	remainLen16[index16++] = ' ';
    }

    /* Set message to send to server */
	uint8_t msgToSend[1000];
	uint32_t msgSize = 0;
	memset(msgToSend,0,sizeof(msgToSend));

	/* Set control field */
	strcat((char*)msgToSend,(const char*)"82 ");
	msgSize += 3;

	/* Set remaining length */
	strcat((char*)msgToSend,(const char*)remainLen16);
	msgSize += index16;

	/* Set packet ID */
	if(PacketIDHexByteNo == 1)
	{
		msgToSend[msgSize++] = '0';
		msgToSend[msgSize++] = '0';
		msgToSend[msgSize++] = ' ';
		strcat((char*)msgToSend,(const char*)packeIDHex);
		msgSize += 2;
	}
	if(PacketIDHexByteNo == 2)
	{
		msgToSend[msgSize++] = packeIDHex[0];
		msgToSend[msgSize++] = packeIDHex[1];
		msgToSend[msgSize++] = ' ';
		msgToSend[msgSize++] = packeIDHex[2];
		msgToSend[msgSize++] = packeIDHex[3];
	}

	msgToSend[msgSize++] = ' ';

	/* Set topic length */
	if(topicByteNo == 1)
	{
		msgToSend[msgSize++] = '0';
		msgToSend[msgSize++] = '0';
		msgToSend[msgSize++] = ' ';
		strcat((char*)msgToSend,(const char*)topicLen);
		msgSize += 2;
	}
	if(topicByteNo == 2)
	{
		msgToSend[msgSize++] = topicLen[0];
		msgToSend[msgSize++] = topicLen[1];
		msgToSend[msgSize++] = ' ';
		msgToSend[msgSize++] = topicLen[2];
		msgToSend[msgSize++] = topicLen[3];
	}

	msgToSend[msgSize++] = ' ';

	/* Set topic name */
	strcat((char*)msgToSend,(const char*)topicHex);
	msgSize += indexHexTopic;

	/* Set requested QoS byte */
	strcat((char*)msgToSend,(const char*)"00 ");
	msgSize += 3;

	msgToSend[msgSize++] = '1';
	msgToSend[msgSize++] = 'a';

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	size = 0;

	/******   SEND COMMAND FOR TCP/IP SENDING DATA   *****/
	/* Send command to send data to server */
	DRIVER_GSM_Write(handler->gsmHandler, (const uint8_t*)"at+cipsend\r", sizeof("at+cipsend\r"));

	/* Read response from gsm  and set response for user */
	/* Take current time and check if timeout occured */
	uint32_t tickstart = TIME_GetTick();
	uint8_t errorOkTimeout = 2;
	while(TIME_GetTick() - tickstart < 3000)
	{
		DRIVER_GSM_Read(handler->gsmHandler, buffer, &size);

		/* Badly received response from gsm */
		if(strstr((const char*)buffer,(const char*)"ERROR") != NULL)
		{
			errorOkTimeout = 1;
			break;
		}
		else if(strstr((const char*)buffer,(const char*)">") != NULL)
		{
			errorOkTimeout = 0;
			/* Successfully received response from gsm */
			break;
		}
	}

	/* Check response from gsm */
	switch(errorOkTimeout){
	case 2:
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_TIMEOUT;
	case 1:
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_ERROR;
	case 0:
		break;
	}

	/******    SEND PUBLISH COMMAND      ********/
	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	size = 0;

	/* Publish message on the topic to server */
	DRIVER_GSM_Write(handler->gsmHandler, msgToSend, msgSize);
	// 30 13 00 03 67 73 6d 00 05 48 45 4c 4c 4f 1a - -t "gsm" -m "HELLO" -- test!

	/* Read response from gsm  and set response for user */
	/* Take current time and check if timeout occured */
	tickstart = TIME_GetTick();
	errorOkTimeout = 2;
	while(TIME_GetTick() - tickstart < 10000)
	{
		DRIVER_GSM_Read(handler->gsmHandler, buffer, &size);

		/* Badly received response from gsm */
		if(strstr((const char*)buffer,(const char*)"ERROR") != NULL)
		{
			errorOkTimeout = 1;
			break;
		}
		else if(strstr((const char*)buffer,(const char*)"OK") != NULL)
		{
			errorOkTimeout = 0;
			/* Successfully received response from gsm */
			break;
		}
	}

	/* Check response from gsm */
	switch(errorOkTimeout){
	case 2:
		handler->mqttPacket.variableHeader.packetID = 0;
		handler->mqttPacket.payload.topicLen = 0;
		memset(handler->mqttPacket.payload.topicName,0,sizeof(handler->mqttPacket.payload.topicName));
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_TIMEOUT;
	case 1:
		handler->mqttPacket.variableHeader.packetID = 0;
		handler->mqttPacket.payload.topicLen = 0;
		memset(handler->mqttPacket.payload.topicName,0,sizeof(handler->mqttPacket.payload.topicName));
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_ERROR;
	case 0:
		strcpy((char*)handler->mqttPacket.payload.topicName,(const char*)topicASCII);
		handler->mqttPacket.payload.topicLen = topicLenDec;
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nSubscribed successfully on the specified topic! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_OK;
	}
	return MQTT_OK;

}

/**
  * @brief Ping server with PINGREQ Packet. The Server MUST send a PINGRESP Packet in response to a PINGREQ Packet.
  * @param handler	   	Handle that contains everything about mqtt protocol (which gsm will be used and which console).
  * @param timeout      Timeout period for console.
  * @retval DRIVERState_t status
  */
MQTTState_t MQTT_PingReq(MQTTHandler_t *handler, uint32_t timeout)
{
	/* Buffer for reading message */
	uint8_t buffer[500] = {0};

	/* How many characters were received */
	uint32_t size = 0;

	/* Checking if user send correct gsm and console */
	if(handler->gsmHandler == NULL && handler->consoleHandler == NULL )
	{
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(handler->gsmHandler);

	/* Send command to send data to server */
	DRIVER_GSM_Write(handler->gsmHandler, (const uint8_t*)"at+cipsend\r", sizeof("at+cipsend\r"));

	/* Read response from gsm  and set response for user */
	/* Take current time and check if timeout occured */
	uint32_t tickstart = TIME_GetTick();
	uint8_t errorOkTimeout = 2;
	while(TIME_GetTick() - tickstart < timeout)
	{
		DRIVER_GSM_Read(handler->gsmHandler, buffer, &size);

		/* Badly received response from gsm */
		if(strstr((const char*)buffer,(const char*)"ERROR") != NULL)
		{
			errorOkTimeout = 1;
			break;
		}
		else if(strstr((const char*)buffer,(const char*)">") != NULL)
		{
			errorOkTimeout = 0;
			/* Successfully received response from gsm */
			break;
		}
	}

	/* Check response from gsm */
	switch(errorOkTimeout){
	case 2:
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_TIMEOUT;
	case 1:
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_ERROR;
	case 0:
		break;
	}

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	size = 0;

	/* Send PINGReq to broker */
	DRIVER_GSM_Write(handler->gsmHandler, (const uint8_t*)"c0 00 1a",
			sizeof("c0 00 1a"));

	/* Read response from gsm  and set response for user */
	/* Take current time and check if timeout occured */
	tickstart = TIME_GetTick();
	errorOkTimeout = 2;
	while(TIME_GetTick() - tickstart < 10000)
	{
		DRIVER_GSM_Read(handler->gsmHandler, buffer, &size);

		/* Badly received response from gsm */
		if(strstr((const char*)buffer,(const char*)"ERROR") != NULL)
		{
			errorOkTimeout = 1;
			break;
		}
		/* Wait for 208 UTF-8 character that respresent PINGResponse, answer from broker */
		else if(strchr((const char*)buffer,208) != NULL)
		{
			errorOkTimeout = 0;
			/* Successfully received response from gsm */
			break;
		}
	}

	/* Check response from gsm */
	switch(errorOkTimeout){
	case 2:
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_TIMEOUT;
	case 1:
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_ERROR;
	case 0:
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nSuccessfully connected to broker! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_OK;
	}
	return MQTT_OK;

}
