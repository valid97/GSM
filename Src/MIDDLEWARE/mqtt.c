/*
 * mqtt.c
 *
 *  Created on: Jan 27, 2021
 *      Author: valentinad
 */

#include <mqtt.h>

/* Function to reverse arr[] from start to end */
void rverseArray(uint8_t arr[], int start, int end)
{
    uint8_t temp;
    while (start < end)
    {
        temp = arr[start];
        arr[start] = arr[end];
        arr[end] = temp;
        start++;
        end--;
    }
}

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
/* Add continuation bit */
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

	handler->initState 	= MQTT_INIT;

	return MQTT_OK;

}

MQTTState_t MQTT_Connect(MQTTHandler_t *handler, uint32_t timeout)
{
//	handler->mqttPacket.commandType = CONNECT; // 0x1
//	handler->mqttPacket.controlFlag = CONTROL_FLAG_CONNECT; //0x0
//
//	handler->mqttPacket.variableHeader.protocolNameLen.MSBByte = 0x00;
//	handler->mqttPacket.variableHeader.protocolNameLen.LSBByte = 0x04; // This isn't setted
////	handler->mqttPacket.variableHeader.protocolName = MQTT_PROTOCOL_NAME(M_HEX,Q_HEX,T_HEX,T_HEX);
//	handler->mqttPacket.variableHeader.protocolLevel = MQTT_VERSION;
//	handler->mqttPacket.variableHeader.connectFlagByte.userNameFlag = USER_NAME_NO_SET;
//	handler->mqttPacket.variableHeader.connectFlagByte.passwordFlag = PASSWORD_NO_SET;
//	handler->mqttPacket.variableHeader.connectFlagByte.willRetainFlag = WILL_RETAIN_NO_SET;
//	handler->mqttPacket.variableHeader.connectFlagByte.willQoSFlag = WILL_QOS_ONE;
//	handler->mqttPacket.variableHeader.connectFlagByte.willFlag = WILL_NO_SET;
//	handler->mqttPacket.variableHeader.connectFlagByte.cleanSessionFlag = CLEAN_SESSION_SET;
//	handler->mqttPacket.variableHeader.keepAlive.MSBByte = 0x0F;
//	handler->mqttPacket.variableHeader.keepAlive.LSBByte = 0x00;
//
//	handler->mqttPacket.payload.clientIDLen.MSBByte = 0x00;
//	handler->mqttPacket.payload.clientIDLen.LSBByte = 0x00;
//	handler->mqttPacket.payload.clientID = 0x00; //don't using in sending

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
	memset(buffer,0,sizeof(buffer));
	size = 0;

	/* Set Hexadecimal format for sending packets to broker */
	 switch(MQTT_SetHexFormat(handler)){
		case DRIVER_TIMEOUT:
			DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
			DRIVER_GSM_Flush(handler->gsmHandler);
			return DRIVER_TIMEOUT;
		case DRIVER_ERROR:
			DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
			DRIVER_GSM_Flush(handler->gsmHandler);
			return DRIVER_ERROR;
		case DRIVER_OK:
			DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\n Hexadecimal format for sending packet to server setted!\r\n");
			break;
	 }

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

	/* Send command to send data to server */
	DRIVER_GSM_Write(handler->gsmHandler, (const uint8_t*)"10 0c 00 04 4d 51 54 54 04 02 0f 00 00 00 1a",
			sizeof("01 0c 00 04 4d 51 54 54 04 02 0f 00 00 00 1a"));

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

MQTTState_t MQTT_Publish(MQTTHandler_t *handler, uint32_t timeout){

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

	/* Request to user */
	DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\n Enter topic name: \r\n ");
	DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)">>");

	/* Receive from user */
	switch(DRIVER_CONSOLE_Get(handler->consoleHandler, buffer, &size, timeout)){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError received, not enough space for receiving characters! Please update your buffer! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_ERROR;
	case DRIVER_OK:
		if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end sending message */
		{
			DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\n Leaving command... \r\nMessage unsent!\r\n");
			DRIVER_GSM_Flush(handler->gsmHandler);
			return MQTT_OK;
		}
		break;
	}

	uint32_t topic[1000] 	= {0};
	uint8_t topicHex[1000] 	= {0};
	uint8_t topicLen[10] 	= {0};
	uint8_t topicHexByteNo 	= 0;
	uint32_t topicLenDec	= size - 1; // length of topic in decimal number
	uint32_t i 				= 0;
	uint32_t indexHexTopic	= 0;

	/* Convert ASCII code to integer value */
	for(; buffer[i] != '\r'; i++)
	{
		topic[i] = buffer[i] - '\0';
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


	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	size = 0;

	/* Request to user */
	DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\n Enter message to publish: \r\n ");
	DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)">>");

	/* Receive from user */
	switch(DRIVER_CONSOLE_Get(handler->consoleHandler, buffer, &size, timeout)){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nError received, not enough space for receiving characters! Please update your buffer! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return DRIVER_ERROR;
	case DRIVER_OK:
		if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end sending message */
		{
			DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\n Leaving command... \r\nMessage unsent!\r\n");
			DRIVER_GSM_Flush(handler->gsmHandler);
			return DRIVER_OK;
		}
		break;
	}

	uint32_t msg[1000] 		= {0};
	uint8_t msgHex[1000] 	= {0};
	uint8_t msgLen[10] 		= {0};
	uint8_t msgHexByteNo 	= 0;
    uint32_t msgLenDec 		= size - 1;
	uint32_t indexHexMsg	= 0;
	i 						= 0;

	/* Convert ASCII code to integer value */
	for(; buffer[i] != '\r'; i++)
	{
		msg[i] = buffer[i] - '\0';
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
    uint32_t remainLenDec = topicLenDec + msgLenDec + 4;

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

	/* Set message length */
	if(msgByteNo == 1)
	{
		msgToSend[msgSize++] = '0';
		msgToSend[msgSize++] = '0';
		msgToSend[msgSize++] = ' ';
		strcat((char*)msgToSend,(const char*)msgLen);
		msgSize += 2;
		msgToSend[msgSize++] = ' ';
	}
	if(msgByteNo == 2)
	{
		msgToSend[msgSize++] = msgLen[0];
		msgToSend[msgSize++] = msgLen[1];
		msgToSend[msgSize++] = ' ';
		msgToSend[msgSize++] = msgLen[2];
		msgToSend[msgSize++] = msgLen[3];
		msgToSend[msgSize++] = ' ';
	}


	/* Set message */
	strcat((char*)msgToSend,(const char*)msgHex);
	msgSize += indexHexMsg;

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
		DRIVER_CONSOLE_Put(handler->consoleHandler,(const uint8_t*)"\r\nMessage published on the specified topic! \r\n");
		DRIVER_GSM_Flush(handler->gsmHandler);
		return MQTT_OK;
	}
	return MQTT_OK;

}
