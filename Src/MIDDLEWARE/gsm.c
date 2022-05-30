/**
//  ********************************************************************************************************************
  * @file    gsm.c
  * @author  Valentina Denic
  * @brief   MIDDLEWARE for gsm and console modules drivers.
  *          This file provides firmware functions to manage the following
  *          functionalities of the gsm and console.
  *
  *
  @verbatim
 ======================================================================================================================
                        ##### How to use this driver #####
 ======================================================================================================================
  [..]
    The MIDDLEWARE gsm can be used as follows:

    (#) Declare a gsmHandler_t handle structure (eg. gsmHandler_t gsmHandler).
	(#) Initialize the gsm low level resources by implementing the GSM_Init() function
    (#) Set echo mode in gsm module
    (#) Set message format (pdu or text(default mode))
    (#) Set message storage for all type of messages(reading,deleting,sending,writing,receiving)
    (#) Test which storage we have (currently available is SIM storage)
	(#) List messages from storage
	(#) Read particular message using index of message parameter
	(#) Delete specific message or all messages of particular type
	(#) Send message using gsm or store message on storage and later send

	(#) Set socket in defined gsmHandler when you open new socket
	(#) Close socket in defined gsmHandler when you close opened socket
	(#) Check how many are opened sockets currently

	(#) Registers gsm module to mobile network station in Serbia region
	(#) Deregisters gsm module from mobile network
	(#) Check if it is gsm module registered on the network
	(#) Set Access Point Name (APN) for current region (Serbia)
	(#) Check setted Access Point Name
	(#) Set wireless connection to GPRS service
	(#) Get assigned IP address
	(#) Attach to GPRS service
	(#) Dettach from GPRS service
	(#) Set PDP context
	(#) Active currently setted PDP context
	(#) Check setted PDP contexts
	(#) Check active PDP contexts
	(#) Deactive currently active PDP context
	(#) Connect to server with his IP address, opened port and specifed type of protocol
	(#) Disconnect from server
	(#) Check server connection
	(#) Send data to server
	(#) Establish TCP\IP connection(calling 4 function for this implementation)

	(#) onlyPutNumber() is function that checks users input and demanding only to put number
	(#) waitUntil() function is used to wait response from gsm by using TIME_Delay() function
	 	which uses timer for timing
  @endverbatim
  *
  **********************************************************************************************************************
  */

#include <gsm.h>

/* Set default format of messages */
GSMMsgFormat_t formatOfMsg = GSM_TEXT_MODE;

/**
  * @brief Demand input to be only number.
  * @param console      Console handle.
  * @param buffer       Storage of received characters.
  * @param size         Number of readed characters.
  * @param bufSize      Size of buffer.
  * @param timeout      Function timeout.
  * @retval DRIVERState_t
  */
DRIVERState_t onlyPutNumber(DRIVERConsoleHandler_t *console, uint8_t *buffer, uint32_t *size, uint32_t bufSize, uint32_t timeout)
{
	/* Take current time and check if timeout occured */
	uint32_t tickstart = TIME_GetTick();
	while(TIME_GetTick() - tickstart < timeout)
	{
		switch(DRIVER_CONSOLE_Get(console, buffer, size, timeout)){
		case DRIVER_TIMEOUT:
			return DRIVER_TIMEOUT;
		case DRIVER_ERROR:
			return DRIVER_ERROR;
		case DRIVER_OK:
			 /* if escape code <ESC> occurs, end function */
			if(strstr((char*)buffer,(const char*)"\e") != NULL)
			{
				return DRIVER_OK;
			}
			break;
		}

		uint8_t i = 0;
		/* Check all received characters in buffer, to see if theres */
		for(i = 0; i < *size - 1; i++)
		{
			if((buffer[i] < '0') || (buffer[i] > '9'))
			{
				DRIVER_CONSOLE_Put(console,(const uint8_t*)"\r\nError! Enter only NUMBER!\r\n");
				break;
			}
		}

		/* If all characters are good, leave function, otherwise demand new input */
		if(i == *size - 1 && *buffer != '\r') return DRIVER_OK;
		else
		{
			DRIVER_CONSOLE_Put(console,(const uint8_t*)">>");
			memset(buffer,0,sizeof(bufSize));
			*size = 0;
		}

	}
	return DRIVER_TIMEOUT;
}

/**
  * @brief Wait response from gsm until timeout.
  * @param gsm          GSM handle.
  * @param buffer       Storage of received characters.
  * @param size         Number of characters readed.
  * @param timeout      Function timeout.
  * @param string       Required string in buffer.
  * @retval DRIVERState_t status
  */
DRIVERState_t waitUntil(DRIVERGsmHandler_t *gsm,uint8_t *buffer,uint32_t *size, uint32_t timeout, const uint8_t *string)
{
	/* Take current time and check if timeout occured */
	uint32_t tickstart = TIME_GetTick();
	while(TIME_GetTick() - tickstart < timeout)
	{
		DRIVER_GSM_Read(gsm, buffer, size);

		/* Badly received response from gsm */
		if(strstr((const char*)buffer,(const char*)"ERROR") != NULL)
			return DRIVER_ERROR;
		else if(strstr((const char*)buffer,(const char*)string) != NULL)
		{
			/* Successfully received response from gsm */
			return DRIVER_OK;
		}
	}
	/* Haven't received response from gsm */
	return DRIVER_TIMEOUT;
}

DRIVERState_t GSM_Init(gsmHandler_t *handler, gsmConfig_t *config)
{
	/* When i don't have any handler to initalize current handle, exit and return error */
	if(handler == NULL || config == NULL || config->gsm == NULL || config->console == NULL)
		return DRIVER_ERROR;

	handler->gsm 		= config->gsm;

	handler->console 	= config->console;

//	handler->mqtt 		= config->mqtt;

	handler->activeSocketNo = 0;

	handler->numSocketOpen 	= 0;

	uint8_t i = 0;
	for(; i != MAX_SOCKET_NUMBER; i++)
	{
		handler->socket[i].status = SOCKET_CLOSE;
		memset(handler->socket[i].IPaddress,0,sizeof(handler->socket[i].IPaddress));
		memset(handler->socket[i].type,0,4);
		handler->socket[i].PDPcontextNo = CONTEXT_NON;
		handler->socket[i].port = PORT_NON;
	}

	handler->network.status = NETWORK_DISCONNECTED;

	memset(handler->network.IPaddress,0,sizeof(handler->socket[i].IPaddress));

	return DRIVER_OK;
}

/**
  * @brief Set echo using AT command.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @param timeout      Timeout period for console.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_SetEcho(gsmHandler_t *gsmHandler, uint32_t timeout, GSMEcho_t echoOnOFF, OutputStruct_t *outputStruct)
{
	/* Buffer for putting answer from gsm */
	uint8_t buffer[100] = {0};

	/* How many characters are received from gsm-it's importent to be zero initialize */
	uint32_t size = 0;

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	size = 0;

	/* Set command for echo mode */
	uint8_t echo[5] = {'A','T','E',echoOnOFF == GSM_ECHO_ON? '1':'0','\r'};

	/* Send command to gsm */
	DRIVER_GSM_Write(gsmHandler->gsm, echo,sizeof(echo));

	/* Read response from gsm  and set response for user*/
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 3000, (const uint8_t*)"OK\r\n")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		strcpy((char*)outputStruct->gsmRsp,(const char*)buffer);
		if(echoOnOFF == GSM_ECHO_ON) DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\nEcho is now ON!\r\n");
		else DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\nEcho is now OFF!\r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;

	}
	return DRIVER_OK;
}

/**
  * @brief Set message format, for sending and receiving message, using AT command.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @param timeout      Timeout period for console.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_MsgFormat(gsmHandler_t *gsmHandler, uint32_t timeout, GSMMsgFormat_t format,OutputStruct_t *outputStruct)
{
	/* Buffer for putting answer from gsm */
	uint8_t buffer[100] = {0};

	/* How many characters are received from gsm */
	uint32_t size = 0;

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Set command for formating message */
	uint8_t msgFormat[] = {'a','t','+','c','m','g','f','=', format == GSM_TEXT_MODE? '1':'0','\r'};

	/* Send command to gsm */
	DRIVER_GSM_Write(gsmHandler->gsm, msgFormat,sizeof(msgFormat));

	/* Read response from gsm  and set response for user*/
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 1000,(const uint8_t*)"OK\r\n")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		strcpy((char*)outputStruct->gsmRsp,(const char*)buffer);
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n");
		if(format == GSM_TEXT_MODE) DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"SMS text mode now is set!");
		else DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"SMS pdu mode now is set!");
		formatOfMsg = format;
		DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;

	}
	return DRIVER_OK;
}

/**
  * @brief Set message storage using AT command.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @param timeout      Timeout period for console.
  * @param inputStruct  Structure that contains needed variables to set storages.
  * @param outputStruct Structure that contains answer from gsm module.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_SetMsgStorage(gsmHandler_t *gsmHandler, uint32_t timeout, const SetMsgStrgInputStruct_t inputStruct,OutputStruct_t *outputStruct)
{

	/* the phone message storage area,"ME" = 1 */
	/* the SIM message storage area,"SM" = 2 */
	/* the phone message storage area and SIM message storage area,"MT" = 3 */

	/* Buffer for putting answer from gsm */
	uint8_t buffer[100] = {0};

	/* How many characters are received from gsm */
	uint32_t size = 0;

	/* Iterators for copying */
	uint8_t i = 0;
	uint8_t j = 0;

	/* Reading answer correctly */
	uint8_t twoSlesh = 0;

	/* Buffer for copying number of used memory and storable memory aviable in gsm */
	uint8_t usedAndStorableMem[10] = {0};

	/* How many times I have to copy to buffer for each memory */
	uint8_t circle = 0;

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Possible set of command for the type of storage */
	if(inputStruct.memMsgReadDelate == 2 && inputStruct.memMsgWriteSend == 2 && inputStruct.memMsgReceive == 2)
		DRIVER_GSM_Write(gsmHandler->gsm, (const uint8_t*)"at+cpms=\"SM\",\"SM\",\"SM\"\r",sizeof("at+cpms=\"SM\",\"SM\",\"SM\"\r"));
	else if(inputStruct.memMsgReadDelate == 2 && inputStruct.memMsgWriteSend == 2 && inputStruct.memMsgReceive == 1)
		DRIVER_GSM_Write(gsmHandler->gsm, (const uint8_t*)"at+cpms=\"SM\",\"SM\",\"ME\"\r",sizeof("at+cpms=\"SM\",\"SM\",\"ME\"\r"));
	else if(inputStruct.memMsgReadDelate == 2 && inputStruct.memMsgWriteSend == 1 && inputStruct.memMsgReceive == 2)
		DRIVER_GSM_Write(gsmHandler->gsm, (const uint8_t*)"at+cpms=\"SM\",\"ME\",\"SM\"\r",sizeof("at+cpms=\"SM\",\"ME\",\"SM\"\r"));
	else if(inputStruct.memMsgReadDelate == 2 && inputStruct.memMsgWriteSend == 1 && inputStruct.memMsgReceive == 1)
		DRIVER_GSM_Write(gsmHandler->gsm, (const uint8_t*)"at+cpms=\"SM\",\"ME\",\"ME\"\r",sizeof("at+cpms=\"SM\",\"ME\",\"ME\"\r"));
	else if(inputStruct.memMsgReadDelate == 1 && inputStruct.memMsgWriteSend == 2 && inputStruct.memMsgReceive == 2)
		DRIVER_GSM_Write(gsmHandler->gsm, (const uint8_t*)"at+cpms=\"ME\",\"SM\",\"SM\"\r",sizeof("at+cpms=\"ME\",\"SM\",\"SM\"\r"));
	else if(inputStruct.memMsgReadDelate == 1 && inputStruct.memMsgWriteSend == 2 && inputStruct.memMsgReceive == 1)
		DRIVER_GSM_Write(gsmHandler->gsm, (const uint8_t*)"at+cpms=\"ME\",\"SM\",\"ME\"\r",sizeof("at+cpms=\"ME\",\"SM\",\"ME\"\r"));
	else if(inputStruct.memMsgReadDelate == 1 && inputStruct.memMsgWriteSend == 1 && inputStruct.memMsgReceive == 2)
		DRIVER_GSM_Write(gsmHandler->gsm, (const uint8_t*)"at+cpms=\"ME\",\"ME\",\"SM\"\r",sizeof("at+cpms=\"ME\",\"ME\",\"SM\"\r"));
	else if(inputStruct.memMsgReadDelate == 1 && inputStruct.memMsgWriteSend == 1 && inputStruct.memMsgReceive == 1)
		DRIVER_GSM_Write(gsmHandler->gsm, (const uint8_t*)"at+cpms=\"ME\",\"ME\",\"ME\"\r",sizeof("at+cpms=\"ME\",\"ME\",\"ME\"\r"));

	/* Read response from gsm  and set response for user*/
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 2000, (const uint8_t*)"OK\r\n")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		strcpy((char*)outputStruct->gsmRsp,(const char*)buffer);
		while(buffer[i] != ':') {i++;}
					while(circle != 3)
					{
						circle++;
						switch(circle){
						case 1:
							i = i + 2;
							DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"Memory for reading and deleting messages: \r\n used space : aviable space\r\n");
							break;
						case 2:
							DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"Memory for writing and sending messages: \r\n used space : aviable space\r\n");
							break;
						case 3:
							DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"Memory for receiving messages: \r\n used space : aviable space \r\n");
							break;
						}
						twoSlesh = 0;
						j = 0;
						while(twoSlesh != 2)
						{
							if(buffer[i] != ',' && buffer[i] != '\r') usedAndStorableMem[j++] = buffer[i++];
							else
							{
								if(twoSlesh == 0)
								{
									usedAndStorableMem[j++] = ' ';
									usedAndStorableMem[j++] = ':';
									usedAndStorableMem[j++] = ' ';
								}
								i++;
								twoSlesh++;
							}
						}
						usedAndStorableMem[j]= '\0';
						DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*) "          ");
						DRIVER_CONSOLE_Put(gsmHandler->console, usedAndStorableMem);
						DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n");
					}

					DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n");
					DRIVER_GSM_Flush(gsmHandler->gsm);
					return DRIVER_OK;

	}
	return DRIVER_OK;
}

/**
  * @brief Test message storage using AT command.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @param timeout      Timeout period for console.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_TestMsgStorage(gsmHandler_t *gsmHandler, uint32_t timeout)
{
	/* Buffer for reading message */
	uint8_t buffer[100] = {0};

	/* How many characters were received */
	uint32_t size = 0;

	/* Buffer for sending  appropriate response to console*/
	char possibleStorages[200] = {0};

	/* Pointers of found types of memory */
	char* foundSM;
	char* foundME;

	char phoneStrg[] = "The phone message storage area"; // "ME"
	char simStrg[] 	 = "The SIM message storage area"; 	// "SM"
	/* char bothStrg[]  = "3 :both the phone message storage area and SIM message storage area"; //"MT" */
	char newLineStr[] = "\r\n";

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Send command to gsm */
	DRIVER_GSM_Write(gsmHandler->gsm, (const uint8_t*)"at+cpms=?\r",sizeof("at+cpms=?\r"));

	/* Read response from gsm  and set response for user*/
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 1000, (const uint8_t*)"OK\r\n")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		foundSM = strstr((const char*)buffer,(const char*)"SM");
		foundME = strstr((const char*)buffer,(const char*)"ME");

		if(foundSM != NULL && foundME != NULL)
		{
			strcat(possibleStorages,phoneStrg);
			strcat(possibleStorages,newLineStr);
			strcat(possibleStorages,simStrg);
		}
		else if(foundSM != NULL &&  foundME == NULL ) strcat(possibleStorages,simStrg);
		else if(foundSM == NULL &&  foundME != NULL ) strcat(possibleStorages,phoneStrg);
		else strcat(possibleStorages,(const char*)"Error: reading a command. Please try again!");

		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*) "\r\n");
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*) possibleStorages);
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*) "\r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;

	}
	return DRIVER_OK;
}

/**
  * @brief List messages using at command.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @param timeout      Timeout period for console.
  * @param inputStruct  Structure that contains needed variables to list messages.
  * @param outputStruct Structure that contains answer from gsm module.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_ListMsg(gsmHandler_t *gsmHandler, uint32_t timeout, const ListMsgInputStruct_t inputStruct, ListMsgOutputStruct_t *outputStruct)
{
	/* Buffer for reading message */
	uint8_t buffer[LENGTH_ALL_MSG] = {0};

	/* How many characters were received */
	uint32_t size = 0;

	/* Set structure for response from gsm when using msg format function*/
	uint8_t buffFormat[100] = {0};
	OutputStruct_t outputStructMsgFormat ={.gsmRsp = buffFormat};

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Set format of message */
	switch(GSM_MsgFormat(gsmHandler, timeout, formatOfMsg, &outputStructMsgFormat)){
	case DRIVER_TIMEOUT:
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		return DRIVER_ERROR;
	case DRIVER_OK:
		break;
	}

	/* Set command about formating command for gsm */
	uint8_t msgToSend[50] 	= {0};
	uint8_t msgSize 		= 8;
	strcat((char*)msgToSend,(const char*)"at+cmgl=");
	if(formatOfMsg == GSM_TEXT_MODE)
	{
		msgToSend[8]= '\"';
		strcat((char*)msgToSend,(const char*)inputStruct.typeOfMsgStr);
		msgToSend[8 + inputStruct.sizeOfTypeOfMsgStr + 1] = '\"';
		msgToSend[8 + inputStruct.sizeOfTypeOfMsgStr + 2]= '\r';
		msgSize = 8 + 3 + inputStruct.sizeOfTypeOfMsgStr;
	}
	else
	{
		msgToSend[msgSize++] = inputStruct.typeOfMsgChar;
		msgToSend[msgSize++]= '\r';
	}

	/*** Wait to respond to command for listing messages! ***/

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	size = 0;

	/* Send command to gsm about list of messages*/
	DRIVER_GSM_Write(gsmHandler->gsm, msgToSend, msgSize);

	uint8_t *startOK;
	uint8_t *startCMGL;
	uint8_t *startOfMsg;
    uint32_t endofMsg = 0;
    uint32_t startofMsg = 0;
    uint32_t msgNo = 0;
    uint8_t firstCopy = 0;
	/* Read response from gsm  and set response for user*/
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 20000, (const uint8_t*)"OK\r\n")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		startOK = (uint8_t*)strstr((const char*)buffer,(const char*)"OK\r\n");
		startCMGL =  (uint8_t*)strstr((char*)buffer,(const char*)"+CMGL:");
		if(startOK != NULL && startCMGL != NULL)
		{
			startofMsg = startCMGL - buffer;
			endofMsg = startOK - buffer;
			uint8_t msgtoDisplay[LENGTH_ALL_MSG] = {0};
			uint32_t i = 0;
			for(i = startofMsg; i < endofMsg; i++)
			{
				msgtoDisplay[i - startofMsg] = buffer[i];
			}
			i -= startofMsg;
			DRIVER_CONSOLE_Put(gsmHandler->console, msgtoDisplay);

			while(1)
			{
				if(firstCopy == 0)
				{
					/* Point to first index of message */
					startOfMsg =  (uint8_t*)strstr((char*)buffer,(const char*)"+CMGL:");
					firstCopy = 1;
				}
				else
					startOfMsg =  (uint8_t*)strstr((char*)startOfMsg,(const char*)"+CMGL:");
				if(startOfMsg != NULL)
				{
					/* Find start of index */
					startOfMsg = startOfMsg + 7;

					/* Set index of message */
					if(*(startOfMsg + 1) == ',')
						outputStruct->index[msgNo] = *startOfMsg - '0';
					else
					{
						startOfMsg++;
						outputStruct->index[msgNo] = 10 + (*startOfMsg - '0');
					}

					/* Find first '\"' for begining of type of msg  */
					startOfMsg += 2;

					/* Set type of message */
					for(uint8_t i = 0; *startOfMsg != ','; i++)
					{
						outputStruct->typeOfMsg[msgNo][i] = *startOfMsg;
						startOfMsg++;
					}

					/* Find '\"' for begining of number of msg */
					startOfMsg = startOfMsg + 1;

					/* Set number */
					for(uint8_t i = 0; *startOfMsg != ','; i++)
					{
						(outputStruct->number[msgNo])[i] = *startOfMsg;
						startOfMsg++;
					}

					/* If we have '\"' char we have time to copy, otherwise we are switching to copy message */
					startOfMsg += 2;
					if(*startOfMsg != '"')
					{
						while(*startOfMsg != '\n') startOfMsg++;
					}
					else
					{
						/* Set time if we have it */
						for(uint8_t i = 0; *startOfMsg != '\r'; i++)
						{
							(outputStruct->timeReceived[msgNo])[i] = *startOfMsg;
							startOfMsg++;
						}

						/* Set pointer to '\n' char */
						startOfMsg++;
					}

					/* Point to first character of message */
					startOfMsg++;

					/* Set message */
					for(uint8_t i = 0; *startOfMsg != '\r'; i++)
					{
						(outputStruct->message[msgNo])[i] = *startOfMsg;
						startOfMsg++;
					}

					/*Go to next message */
					msgNo++;
				}
				else
				{	/* We copied all messages */
					*outputStruct->msgNoStruct = msgNo;
					break;
				}
			}
			DRIVER_GSM_Flush(gsmHandler->gsm);
		}
		else if(startOK != NULL && startCMGL == NULL )
		{
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"Storage empty, no messages of this type!\r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
		}
		return DRIVER_OK;
	}
	return DRIVER_OK;
}

/**
  * @brief Read message using at command.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @param timeout      Timeout period for console.
  * @param inputStruct  Structure that contains needed variables to read message.
  * @param outputStruct Structure that contains answer from gsm module.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_ReadMsg(gsmHandler_t *gsmHandler, uint32_t timeout, const ReadMsgInputStruct_t inputStruct, ReadMsgOutputStruct_t *outputStruct)
{
	/* Buffer for reading message */
	uint8_t buffer[1000] = {0};

	/* How many characters were received */
	uint32_t size = 0;

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Set command reading message from gsm */
	uint8_t msgToSend[50] = {0};
	strcat((char*)msgToSend,(const char*)"at+cmgr=");
	uint8_t msgSize = 8;
	uint8_t i = 0;
	for(i = 0; inputStruct.msgIndex[i] != '\r';i++)
	{
		msgToSend[8 + i] = inputStruct.msgIndex[i];
	}
	msgToSend[8 + i] = '\r';
	msgSize += i + 1;

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	size = 0;

	/* Send command to gsm to read message at entered index */
	DRIVER_GSM_Write(gsmHandler->gsm, msgToSend, msgSize);

	uint8_t *startOK;
	uint8_t *startCMGR;
	uint8_t *startOfMsg;
    uint32_t endofMsg = 0;
    uint32_t startofMsg = 0;
	/* Read response from gsm  and set response for user*/
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 2000, (const uint8_t*)"OK\r\n")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		startOK =  (uint8_t*)strstr((const char*)buffer,"OK\r\n");
		startCMGR =  (uint8_t*)strstr((const char*)buffer,"+CMGR:");
	    if(startCMGR != NULL)
		{
			startofMsg = startCMGR - buffer;
			endofMsg = startOK - buffer;
			uint8_t msgtoDisplay[1000] = {0};
			uint32_t i = 0;
			for(i = startofMsg; i < endofMsg; i++)
			{
				msgtoDisplay[i - startofMsg] = buffer[i];
			}
			DRIVER_CONSOLE_Put(gsmHandler->console, msgtoDisplay);
			DRIVER_GSM_Flush(gsmHandler->gsm);

			/* Set output structure */

			startOfMsg =  (uint8_t*)strstr((char*)buffer,(const char*)"+CMGR:");
			/* Find first '\"' for begining of type of msg  */
			startOfMsg = startOfMsg + 7;

			/* Set type of message */
			for(uint8_t i = 0; *startOfMsg != ','; i++)
			{
				outputStruct->typeOfMsg[i] = *startOfMsg;
				startOfMsg++;
			}

			/* Find '\"' for begining of number of msg */
			startOfMsg = startOfMsg + 1;

			/* Set number */
			for(uint8_t i = 0; *startOfMsg != ','; i++)
			{
				outputStruct->number[i] = *startOfMsg;
				startOfMsg++;
			}

			/* If we have '\"' char we have time to copy, otherwise we are switching to copy message */
			startOfMsg += 2;
			if(*startOfMsg != '"')
			{
				while(*startOfMsg != '\n') startOfMsg++;
			}
			else
			{
				/* Set time if we have it */
				for(uint8_t i = 0; *startOfMsg != '\r'; i++)
				{
					outputStruct->timeReceived[i] = *startOfMsg;
					startOfMsg++;
				}

				/* Set pointer to '\n' char */
				startOfMsg++;
			}

			/* Point to first character of message */
			startOfMsg++;

			/* Set message */
			for(uint8_t i = 0; *startOfMsg != '\r'; i++)
			{
				outputStruct->message[i] = *startOfMsg;
				startOfMsg++;
			}

			return DRIVER_OK;
		}
		else if(startCMGR == NULL)
		{
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nStorage empty, no messages!\r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_OK;
		}
	}
	return DRIVER_OK;
}

/**
  * @brief Delete messages using at command.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @param timeout      Timeout period for console.
  * @param inputStruct  Structure that contains needed variables to delete message(s).
  * @param outputStruct Structure that contains answer from gsm module.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_DeleteMsg(gsmHandler_t *gsmHandler, uint32_t timeout, DeleteMsgInputStruct_t inputStruct, OutputStruct_t *outputStruct)
{
	/* Buffer for reading message */
	uint8_t buffer[500] = {0};

	/* How many characters were received */
	uint32_t size = 0;

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Set gsm command for deleting message*/
	uint8_t msgToSend[50] = {0};
	strcat((char*)msgToSend,"at+cmgd=");
	uint32_t msgSize = 8;

	if(inputStruct.deleteType == '1')
	{
		strcat((char*)msgToSend,(const char*)inputStruct.userRsp);
		msgSize += size;
	}
	else
	{
		/* It's necessary to put index in first place, then to put flag(buffer) on second*/
		msgToSend[msgSize] = '1';
		msgSize++;
		msgToSend[msgSize] = ',';
		msgSize++;
		strcat((char*)msgToSend,(const char*)inputStruct.userRsp);
		msgSize += 2;
	}

	/* Reset bufer and his size */
	memset(buffer,0,sizeof(buffer));
	size = 0;

	/* Send command to gsm to read message at entered index */
	DRIVER_GSM_Write(gsmHandler->gsm, msgToSend, msgSize);

	/* Read response from gsm  and set response for user*/
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 5000, (const uint8_t*)"OK")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		strcpy((char*)outputStruct,(const char*)buffer);
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Message(s) are deleted correctly!\r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;
	}
	return DRIVER_OK;
}

/**
  * @brief Send message using at command.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @param timeout      Timeout period for console.
  * @param inputStruct  Structure that contains needed variables to send or store message.
  * @param outputStruct Structure that contains answer from gsm module.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_SendStoreMsg(gsmHandler_t *gsmHandler, uint32_t timeout,const SendOrStoreInputStruct_t inputStruct,OutputStruct_t *outputStruct)
{
	/* Buffer for reading message */
	uint8_t buffer[500] = {0};

	/* How many characters were received */
	uint32_t size = 0;

	/* Set structure for response from gsm when using msg format function*/
	uint8_t buffFormat[100] = {0};
	OutputStruct_t outputStructMsgFormat ={.gsmRsp = buffFormat};

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Set format of message */
	switch(GSM_MsgFormat(gsmHandler, timeout, formatOfMsg,&outputStructMsgFormat)){
	case DRIVER_TIMEOUT:
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		return DRIVER_ERROR;
	case DRIVER_OK:
		break;
	}

	/* Message to send to gsm */
	uint8_t msgToSend[50] = {0};
	uint8_t sendOrStore[1] = {inputStruct.sendOrStoreFlag}; /* Variable for writing on the end of function if message is sent or stored to console */
	uint8_t msgSize = 0;

	/* Ask if user want to send message from storage or directly send message we have to ask user that
	 * and set command from gsm according to his response */
	if(*sendOrStore == '1')
	{
		if(inputStruct.storeOrSendDirectFlag == '1')
		{
			strcat((char*)msgToSend,"at+cmss=");	/* Send message from storage 	*/
			msgSize = 8;
			uint32_t i = 0;
			strcat((char*)msgToSend,(const char*)inputStruct.index);
			for(; inputStruct.index[i] != '\r';i++);
			msgSize += i;
			msgToSend[msgSize++] = ',';
			msgToSend[msgSize++] = '\"';
		}
		else if(inputStruct.storeOrSendDirectFlag == '2')
		{
			strcat((char*)msgToSend,"at+cmgs=\"");	/* Send message directly		*/
			msgSize = 9;
		}
	}
	/* If we had users response to store message we have to set that command */
	else if(*sendOrStore == '2')
	{
		strcat((char*)msgToSend,"at+cmgw=\"");		/* Store message 				*/
		msgSize = 9;
	}

	/** We need set size of message **/
	for(size = 0; inputStruct.number[size] != '\r';size++);
	size++;

	/* Set number in buffer(msgToSend) for seding number to gsm  */
	uint8_t countryCode[] = "+381";
	switch(*inputStruct.number){
	case '+':
		strcat((char*)msgToSend,(const char*)inputStruct.number);
		msgSize += size;
		break;
	case '0':
		strcat((char*)msgToSend,(const char*)countryCode);
		msgSize += sizeof(countryCode) - 1;
		uint8_t strNum[20] = {0};
		uint8_t i = 1;
		for(i = 1;i < size; i++) strNum[i - 1] = inputStruct.number[i];
		strcat((char*)msgToSend, (const char*)strNum);
		msgSize += i - 1;
		break;
	default:
		strcat((char*)msgToSend, (const char*)countryCode);
		strcat((char*)msgToSend, (const char*)inputStruct.number);
		msgSize += sizeof(countryCode) + size - 1;
		break;
	}
	msgToSend[msgSize - 1] = '\"';
	/* Set last character '\r' 'cause gsm command needs it */
	msgToSend[msgSize] = '\r';
	msgSize++;

	/** If we don't have to send from storage we must first write number to gsm
	 * and wait for his response witch is going to be character ">" that mean
	 * we have to set him message that we want to send **/
	if(inputStruct.storeOrSendDirectFlag != '1')
	{

		/* Reset buffer and his size */
		memset(buffer,0,sizeof(buffer));
		size = 0;

		/* Send command to gsm to accept number for sending message */
		DRIVER_GSM_Write(gsmHandler->gsm, msgToSend, msgSize);

		/* Read response from gsm  and set response for user*/
		switch(waitUntil(gsmHandler->gsm, buffer, &size, 10000, (const uint8_t*)">")){
		case DRIVER_TIMEOUT:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_TIMEOUT;
		case DRIVER_ERROR:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_ERROR;
		case DRIVER_OK:
			if( strstr((char*)buffer,(const char*)">") == NULL)
			{
				DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\nError: reading a command. Please try again!\r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_ERROR;
			}
			else break;
		}

		memset(msgToSend,0, sizeof(msgToSend));
		msgSize = 0;
		for(size = 0; inputStruct.message[size] != '\r';size++);
		strcat((char*)msgToSend,(const char*)inputStruct.message);
		msgToSend[size] = 26; /* <CTRL-Z> character */
		msgSize = size + 1;
	}

	/* Reset buffer and hos size */
	memset(buffer,0,sizeof(buffer));
	size = 0;

	/* Set command to gsm to send written message or
	 * to send a message from the storage */
	DRIVER_GSM_Write(gsmHandler->gsm, msgToSend, msgSize);

	/* Read response from gsm  and set response for user*/
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 6000, (const uint8_t*)"OK")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		strcpy((char*)outputStruct,(const char*)buffer);
		if(*sendOrStore == '1') DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n Message sent!\r\n");
		else DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n Message stored!\r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;
	}
	return DRIVER_OK;
}

/***************************************************************************************************************************************************/
/**************************************************** FROM HERE STARTS FUNCTIONS FOR NETWORK *******************************************************/
/***************************************************************************************************************************************************/

// IP ADDRESS AND TYPE MUST TO FINISH WITH '\0' CHARACTER!!
/**
  * @brief Open socket and set his state with parameters defined in socket variable.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @param socket       Socket handle.
  * @retval SocketStatus_t status
  */
SocketStatus_t GSM_SetSocket(gsmHandler_t *gsmHandler, Socket_t *socket)
{

	/* If port is different from PORT_NON(65535) then you can set it, otherewise you can't */
	if(socket->port != PORT_NON)
	{
		gsmHandler->socket[socket->PDPcontextNo].port = socket->port;
	}

	/* If type contains TCP and UDP string then you can set it, otherewise you can't */
	if(strstr((char*)socket->type,(const char*)"TCP") != NULL || strstr((char*)socket->type,(const char*)"UDP"))
	{
		strcpy((char*)gsmHandler->socket[socket->PDPcontextNo].type,(const char*)socket->type);
	}

	/* If IPaddress does contain dot in, then you can set IP address, otherewise you can't */
	if( strstr((char*)socket->IPaddress,(const char*)".") != NULL)
	{
		strcpy((char*)gsmHandler->socket[socket->PDPcontextNo].IPaddress,(const char*)socket->IPaddress);
	}

	/* If PDPcontextNo is different from CONTEXT_NON(255) then you can set it, otherewise you can't */
	if(socket->PDPcontextNo != CONTEXT_NON)
	{
		if(gsmHandler->socket[socket->PDPcontextNo].status == SOCKET_CLOSE)
		{
			if(gsmHandler->numSocketOpen < MAX_SOCKET_NUMBER)
				gsmHandler->numSocketOpen++;
		}
		gsmHandler->socket[socket->PDPcontextNo].PDPcontextNo = socket->PDPcontextNo;
		gsmHandler->socket[socket->PDPcontextNo].status = SOCKET_OPEN;

	}

	return SOCKET_OPEN;

}

/**
  * @brief Close socket and set his state to default.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @param socket       Socket handle.
  * @retval SocketStatus_t status
  */
SocketStatus_t GSM_CloseSocket(gsmHandler_t *gsmHandler, Socket_t *socket)
{
	/* Decrease number of opened socket */
	if(gsmHandler->socket[socket->PDPcontextNo].status == SOCKET_OPEN)
	{
		if(gsmHandler->numSocketOpen > 0)
			gsmHandler->numSocketOpen--;
	}

	/* Set default state for socket */
	gsmHandler->socket[socket->PDPcontextNo].status = SOCKET_CLOSE;
	memset(gsmHandler->socket[socket->PDPcontextNo].IPaddress,0,16);
	memset(gsmHandler->socket[socket->PDPcontextNo].type,0,4);
	gsmHandler->socket[socket->PDPcontextNo].PDPcontextNo = CONTEXT_NON;
	gsmHandler->socket[socket->PDPcontextNo].port = PORT_NON;

	return SOCKET_CLOSE;
}

/**
  * @brief Check how many are open sockets.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @retval SocketStatus_t status
  */
SocketStatus_t GSM_CheckOpenSocketNo(gsmHandler_t *gsmHandler)
{
	/* If we opened all context of PDP we cannot open another socket context */
	if(gsmHandler->numSocketOpen == MAX_SOCKET_NUMBER) return SOCKET_FULL;
	else return SOCKET_AVAILABLE;
}

/**
  * @brief Turn on network connection.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_NetworkRegistered(gsmHandler_t *gsmHandler)
{
	uint8_t buffer[100];
	uint32_t size = 0;

	/* Reset buffer */
	memset(buffer,0,sizeof(buffer));

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Set command checking network connection */
	DRIVER_GSM_Write(gsmHandler->gsm,(const uint8_t*) "at+creg=1\r", sizeof("at+creg=1\r"));

	/* Read response from gsm  and set response for user*/
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 2000, (const uint8_t*)"OK")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		gsmHandler->network.status = NETWORK_CONNECTED;
		DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n Network is now on!\r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;
	}
	return DRIVER_OK;
}

/**
  * @brief Turn off network connection.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_NetworkDeregistered(gsmHandler_t *gsmHandler)
{
	uint8_t buffer[100];
	uint32_t size = 0;

	/* Reset buffer */
	memset(buffer,0,sizeof(buffer));

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Set command checking network connection */
	DRIVER_GSM_Write(gsmHandler->gsm,(const uint8_t*) "at+creg=0\r", sizeof("at+creg=0\r"));

	/* Read response from gsm  and set response for user*/
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 2000, (const uint8_t*)"OK")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		gsmHandler->network.status = NETWORK_DISCONNECTED;
		DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n Network is now off!\r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;
	}
	return DRIVER_OK;
}

/**
  * @brief Check network connection.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_CheckNetworkRegistered(gsmHandler_t *gsmHandler)
{
	uint8_t buffer[100];
	uint32_t size = 0;

	/* Reset buffer */
	memset(buffer,0,sizeof(buffer));

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Set command checking network connection */
	DRIVER_GSM_Write(gsmHandler->gsm,(const uint8_t*) "at+creg?\r", sizeof("at+creg?\r"));

	/* Read response from gsm  and set response for user*/
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 2000, (const uint8_t*)"OK")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		if(strchr((char*)buffer,'0') != NULL)
		{
			gsmHandler->network.status = NETWORK_DISCONNECTED;
			DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n Mobile isn't registered to network! Please try to set network connection first!\r\n");
		}
		else
		{
			gsmHandler->network.status = NETWORK_CONNECTED;
			DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n Mobile is network registered!\r\n");
		}
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;
	}
	return DRIVER_OK;
}

/**
  * @brief Set APN for current region.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_SetAPN(gsmHandler_t *gsmHandler)
{
	uint8_t buffer[100];
	uint32_t size = 0;

	/* Reset buffer */
	memset(buffer,0,sizeof(buffer));

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Send command for setting Access Point Names(APN's) */
	DRIVER_GSM_Write(gsmHandler->gsm,(const uint8_t*) "at+cstt=\"gprsinternet\"\r", sizeof("at+cstt=\"gprsinternet\"\r"));

	/* Read response from gsm  and set response for user*/
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 15000, (const uint8_t*)"OK")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n APN is setted for Serbia, B&H and Montenegro regions!\r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;
	}
	return DRIVER_OK;

}

/**
  * @brief Check registed APN.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_CheckAPN(gsmHandler_t *gsmHandler)
{
	uint8_t buffer[100];
	uint32_t size = 0;

	/* Reset buffer */
	memset(buffer,0,sizeof(buffer));

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Send command for setting Access Point Names(APN's) */
	DRIVER_GSM_Write(gsmHandler->gsm,(const uint8_t*) "at+cstt?\r", sizeof("at+cstt?r"));

	/* Read response from gsm  and set response for user*/
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 4000, (const uint8_t*)"OK")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		DRIVER_CONSOLE_Put(gsmHandler->console, buffer);
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;
	}
	return DRIVER_OK;

}


/**
  * @brief Set wireless connection with GPRS. Before this function, function for setting APN should be called.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_SetWirelessConnectionGPRS(gsmHandler_t *gsmHandler)
{
	uint8_t buffer[100];
	uint32_t size = 0;

	/* Reset buffer */
	memset(buffer,0,sizeof(buffer));

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Send command for setting Access Point Names(APN's) */
	DRIVER_GSM_Write(gsmHandler->gsm,(const uint8_t*) "at+ciicr\r", sizeof("at+ciicr\r"));

	/* Read response from gsm  and set response for user*/
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 3000, (const uint8_t*)"OK")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		gsmHandler->network.status = NETWORK_CONNECTED;
		DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n Wireless connection with GPRS service established!\r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;
	}
	return DRIVER_OK;

}

/**
  * @brief Get local IP adress from provider server.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_GetLocalIPAddress(gsmHandler_t *gsmHandler)
{
	uint8_t buffer[100];
	uint32_t size = 0;

	/* Reset buffer */
	memset(buffer,0,sizeof(buffer));

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Send command for setting Access Point Names(APN's) */
	DRIVER_GSM_Write(gsmHandler->gsm,(const uint8_t*) "at+cifsr\r", sizeof("at+cifsr\r"));

	uint32_t i = 0;
	uint32_t startMsg = 0;

	/* Read response from gsm  and set response for user*/

	/* Take current time and check if timeout occured */
	uint32_t tickstart = TIME_GetTick();
	DRIVERState_t StateReadCmd = DRIVER_OK;
	while(TIME_GetTick() - tickstart < 3000)
	{
		DRIVER_GSM_Read(gsmHandler->gsm, buffer, &size);
	}

	/* Badly received response from gsm */
	if(strstr((const char*)buffer,(const char*)"ERROR") != NULL)
		StateReadCmd = DRIVER_ERROR;

	switch(StateReadCmd){
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		if(strstr((char*)buffer,(const char*)"at+cifsr\r") != NULL)
		{
			startMsg = 11;
		}
		else
		{
			startMsg = 2;
		}

		for(;buffer[i + startMsg] != '\r';i++) gsmHandler->network.IPaddress[i] = buffer[i + startMsg];
		DRIVER_CONSOLE_Put(gsmHandler->console, gsmHandler->network.IPaddress);
		DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;
	default:
		break;
	}
	return DRIVER_OK;
}
/**
  * @brief Attach to Packet Domain service.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_AttachToGPRSService(gsmHandler_t *gsmHandler)
{
	uint8_t buffer[100];
	uint32_t size = 0;

	/* Reset buffer */
	memset(buffer,0,sizeof(buffer));

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Set command for network activation */
	DRIVER_GSM_Write(gsmHandler->gsm, (const uint8_t*) "at+cgatt=1\r", sizeof("at+cgatt=1\r"));

	/* Read response from gsm  and set response for user*/
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 7000, (const uint8_t*)"OK")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n Network attached!\r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;
	}
	return DRIVER_OK;

}

/**
  * @brief Detach from Packet Domain service.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_DetachFromGPRSService(gsmHandler_t *gsmHandler)
{
	uint8_t buffer[100];
	uint32_t size = 0;

	/* Reset buffer */
	memset(buffer,0,sizeof(buffer));

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Set command for network deactivation */
	DRIVER_GSM_Write(gsmHandler->gsm, (const uint8_t*) "at+cgatt=0\r", sizeof("at+cgatt=0\r"));

	/* Read response from gsm  and set response for user*/
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 7000, (const uint8_t*)"OK")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n Network detached!\r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;
	}
	return DRIVER_OK;

}

/**
  * @brief Set Packet Data Protocol context for gsm module.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @param timeout      Timeout period for console.
  * @param inputStruct  Structure that contains needed variables to set PDP context.
  * @param outputStruct Structure that contains answer from gsm module.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_SetPDPContext(gsmHandler_t *gsmHandler, uint32_t timeout, SetPDPInputStruct_t inputStruct)
{
	uint8_t buffer[100] = {0};
	uint32_t size = 0;

	/* Check if we have space to open new socket */
	if(GSM_CheckOpenSocketNo(gsmHandler) == SOCKET_FULL)
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: No available sockets for opening! Try closing some socket!\r\n");
		return DRIVER_ERROR;
	}

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* set message that  will be sent to gsm */
	uint8_t msgToSend[200] = {0};
	uint32_t msgSize = 0;

	/* Set command for seting pdp context on gsm module */
	uint8_t PDPContextID[14];
	memset(PDPContextID,0,sizeof(PDPContextID));
	strcat((char*)PDPContextID,(const char*)"at+cgdcont=");
	PDPContextID[11] = *inputStruct.PDPNo;
	if(inputStruct.PDPNo[1] == '\r')
	{
		PDPContextID[12] = ',';
		PDPContextID[13] = '\0';
		msgSize = 13;
	}
	else
	{
		PDPContextID[12] = inputStruct.PDPNo[1];
		PDPContextID[13] = ',';
		PDPContextID[14] = '\0';
		msgSize = 14;
	}

	/* Set Packet Data Protocol type and
	 * set size of message for sending command to gsm module */
	uint8_t PDPType[10] = {0};
	if( *inputStruct.PDPTypeFlag == '1') {strcpy((char*)PDPType,(const char*)"\"IP\","); msgSize += 5;}
	else if(*inputStruct.PDPTypeFlag == '2') {strcpy((char*)PDPType,(const char*)"\"IPV6\","); msgSize += 7;}
	else if(*inputStruct.PDPTypeFlag == '3') {strcpy((char*)PDPType,(const char*)"\"PPP\","); msgSize += 6;}

	/* Find size of APNType */
	uint32_t i = 0;
	for(; inputStruct.APNType[i] != '\r';i++);
	msgSize += i;

	/* Set buffer for sending command to gsm module to contains message  */
	memset(msgToSend,0, msgSize);
	strcat((char*)msgToSend,(const char*)PDPContextID);
	strcat((char*)msgToSend,(const char*)PDPType);
	strcat((char*)msgToSend,(const char*)"\"");
	strcat((char*)msgToSend,(const char*)inputStruct.APNType);
	msgSize++;
	msgToSend[msgSize++] = '\"';
	msgToSend[msgSize++] = '\r';

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	size = 0;

	/* Set command for setting PDP context */
	DRIVER_GSM_Write(gsmHandler->gsm, msgToSend, msgSize);

	/* Read response from gsm  and set response for user*/
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 2000, (const uint8_t*)"OK")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n Packet Data Protocol(PDP) is now setted!\r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;
	}
	return DRIVER_OK;

}

/**
  * @brief Check setted Packet Data Protocol context for gsm module.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_CheckSettedPDPContext(gsmHandler_t *gsmHandler)
{
	uint8_t buffer[1000];

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	uint32_t size = 0;

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Send command to dissconnect from server */
	DRIVER_GSM_Write(gsmHandler->gsm, (const uint8_t*)"at+cgdcont?\r", sizeof("at+cgdcont?\r"));

	uint8_t *startStr;
	uint8_t *endStr;
	uint8_t consoleMsg[1000];
	uint32_t i = 0;
	/* Read response from gsm  and set response for user */
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 4000, (const uint8_t*)"OK")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		startStr = (uint8_t *)strstr((char *)buffer,(const char *)"+CGDCONT");
		endStr = (uint8_t *)strstr((char *)buffer,(const char *)"OK");
		memset(consoleMsg,0,sizeof(consoleMsg));
		for(;startStr != endStr; i++)
		{
			consoleMsg[i] = *startStr;
			startStr++;
		}
		DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n");
		DRIVER_CONSOLE_Put(gsmHandler->console, consoleMsg);
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;
	}
	return DRIVER_OK;
}

/**
  * @brief Check active Packet Data Protocol context for gsm module.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_CheckActivePDPContext(gsmHandler_t *gsmHandler)
{
	uint8_t buffer[100];

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	uint32_t size = 0;

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Send command to check activation type of defined PDP contexts */
	DRIVER_GSM_Write(gsmHandler->gsm, (const uint8_t*)"at+cgact?\r", sizeof("at+cgact?\r"));

	uint8_t *startStr;
	uint8_t *endStr;
	uint8_t consoleMsg[1000];
	uint32_t i = 0;
	/* Read response from gsm  and set response for user */
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 4000, (const uint8_t*)"OK")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		startStr = (uint8_t *)strstr((char *)buffer,(const char *)"+CGACT");
		endStr = (uint8_t *)strstr((char *)buffer,(const char *)"OK");
		memset(consoleMsg,0,sizeof(consoleMsg));
		for(;startStr != endStr; i++)
		{
			consoleMsg[i] = *startStr;
			startStr++;
		}
		DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n");
		DRIVER_CONSOLE_Put(gsmHandler->console, consoleMsg);
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;
	}
	return DRIVER_OK;
}

/**
  * @brief Check active Packet Data Protocol context for gsm module.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_ShowPDPIP(gsmHandler_t *gsmHandler)
{
	uint8_t buffer[1000];

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	uint32_t size = 0;

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Send command to check activation type of defined PDP contexts */
	DRIVER_GSM_Write(gsmHandler->gsm, (const uint8_t*)"at+cgpaddr\r", sizeof("at+cgpaddr\r"));

	uint8_t *startStr;
	uint8_t *endStr;
	uint8_t consoleMsg[1000];
	uint32_t i = 0;
	/* Read response from gsm  and set response for user */
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 4000, (const uint8_t*)"OK")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		startStr = (uint8_t *)strstr((char *)buffer,(const char *)"+CGPADDR");
		endStr = (uint8_t *)strstr((char *)buffer,(const char *)"OK");
		memset(consoleMsg,0,sizeof(consoleMsg));
		for(;startStr != endStr; i++)
		{
			consoleMsg[i] = *startStr;
			startStr++;
		}
		DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n");
		DRIVER_CONSOLE_Put(gsmHandler->console, consoleMsg);
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;
	}
	return DRIVER_OK;
}

/**
  * @brief Active Packet Data Protocol context for gsm module.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @param timeout      Timeout period for console.
  * @param PDP 			Index of Packet Data Protocol format that will be activated
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_ActivePDPContext(gsmHandler_t *gsmHandler, uint32_t timeout, const uint8_t *PDP)
{
	uint8_t buffer[100];
	uint32_t size = 0;
	Socket_t socketInit;

	/* Reset buffer */
	memset(buffer,0,sizeof(buffer));

	if(strchr((char*)PDP,'\r') == NULL) {
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: Set correct format of PDP argument!\r\n "
				"Paramater PDP needs to finish with '\r' char! \r\n");
		return DRIVER_ERROR;
	}

	/* Check if we have space to open new socket */
	if(GSM_CheckOpenSocketNo(gsmHandler) == SOCKET_FULL)
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: No available sockets for opening! Try closing some socket!\r\n");
		return DRIVER_ERROR;
	}

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Make pointer for storing message to send to gsm module */
	uint32_t msgSize = 0;

	/* Set command for activating PDP context */

	/* Set which one PDP context will be open (only set PDPcontextNO)! When
	 * connecting with server set other parameters */
	uint8_t activePDP[14];
	memset(activePDP,0,sizeof(activePDP));
	strcat((char*)activePDP,(const char*)"at+cgact=1,");
	activePDP[11] = *PDP;
	if(PDP[1] == '\r')
	{
		activePDP[12] = '\r';
		msgSize = 13;
		socketInit.PDPcontextNo = *PDP - '0';
	}
	else
	{
		activePDP[12] = PDP[1];
		activePDP[13] = '\r';
		msgSize = 14;
		socketInit.PDPcontextNo = 10 + (PDP[1] - '0');
	}

	memset(socketInit.IPaddress,0, sizeof(socketInit.IPaddress));
	socketInit.port = PORT_NON;
	socketInit.status = SOCKET_SET;
	memset(socketInit.type,0, sizeof(socketInit.type));

	/* Set command for activation PDP context */
	DRIVER_GSM_Write(gsmHandler->gsm, activePDP, msgSize);

	/* Read response from gsm  and set response for user*/
	/* Set number of opened socket context and set currently opened socket! */
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 7000, (const uint8_t*)"OK")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n Packet Data Protocol(PDP) is activated!\r\n");
		gsmHandler->activeSocketNo = socketInit.PDPcontextNo; /* set active PDP context in gsm handler */
		GSM_SetSocket(gsmHandler, &socketInit);
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;
	}
	return DRIVER_OK;
}

/**
  * @brief Deactive Packet Data Protocol context for GPRS connection.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_DeactiveGPRSPDPContext(gsmHandler_t *gsmHandler)
{
	uint8_t buffer[100];
	uint32_t size = 0;

	/* Reset buffer */
	memset(buffer,0,sizeof(buffer));

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Set command for deactivating PDP context */
	DRIVER_GSM_Write(gsmHandler->gsm,(const uint8_t*) "at+cipshut\r", sizeof("at+cipshut\r"));

	/* Read response from gsm  and set response for user*/
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 4000, (const uint8_t*)"OK")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n Packet Data Protocol(PDP) is deactivated!\r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;
	}
	return DRIVER_OK;
}

/**
  * @brief Deactive Packet Data Protocol context for gsm module.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @param PDP 			Index of Packet Data Protocol format that will be deactivated
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_DeactivePDPContext(gsmHandler_t *gsmHandler, uint32_t timeout, const uint8_t *PDP)
{
	uint8_t buffer[100];
	uint32_t size = 0;
	Socket_t socketInit;

	/* Reset buffer */
	memset(buffer,0,sizeof(buffer));

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Make pointer for storing message to send to gsm module */
	uint32_t msgSize = 0;

	/* Set command for activating PDP context */

	/* Set which one PDP context will be closed (only set PDPcontextNO)! When
	 * connecting with server set other parameters */
	uint8_t activePDP[14];
	memset(activePDP,0,sizeof(activePDP));
	strcat((char*)activePDP,(const char*)"at+cgact=0,");
	activePDP[11] = *PDP;
	if(PDP[1] == '\r')
	{
		activePDP[12] = '\r';
		msgSize = 13;
		socketInit.PDPcontextNo = *PDP - '0';
	}
	else
	{
		activePDP[12] = PDP[1];
		activePDP[13] = '\r';
		msgSize = 14;
		socketInit.PDPcontextNo = 10 + (PDP[1] - '0');
	}

	memset(socketInit.IPaddress,0, sizeof(socketInit.IPaddress));
	socketInit.port = PORT_NON;
	socketInit.status = SOCKET_CLOSE;
	memset(socketInit.type,0, sizeof(socketInit.type));

	/* Set command for deactivating PDP context */
	DRIVER_GSM_Write(gsmHandler->gsm, activePDP, msgSize);

	/* Read response from gsm  and set response for user*/
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 6000, (const uint8_t*)"OK")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n Packet Data Protocol(PDP) is deactivated!\r\n");
		gsmHandler->activeSocketNo = 0; /* set parameter to "non of sockets are active" */
		GSM_CloseSocket(gsmHandler,&socketInit);
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;
	}
	return DRIVER_OK;
}

/**
  * @brief Set auto sending timer - the seconds after which the data will be sent to server
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @param timeout      Timeout period for console.
  * @param status  		Hexadecimal or decimal choice (can be only '1' or '2' character!).
  * @param time			Can be parameter in range from 1 to 101.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_SetAutoSendingTimerIP(gsmHandler_t *gsmHandler, uint32_t timeout, uint8_t status, uint8_t *time)
{
	uint8_t buffer[100];

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	uint32_t size = 0;

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Set command for setting timer mode */
	uint8_t timer[12] = {'a','t','+','c','i','p','a','t','s','=',status == '2' ? '1':'0','\0'};
	uint8_t msgToSend[20] = {0};
	uint8_t msgSize = 11;
	strcat((char*)msgToSend,(const char*)timer);

	if(status == '2')
	{
		strcat((char*)msgToSend,(const char*)",");
		strcat((char*)msgToSend,(const char*)time);
		msgSize += strlen((const char*)time) + 1;
	}
	else
	{
		strcat((char*)msgToSend,(const char*)"\r");
		msgSize++;
	}

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	size = 0;

	/* Send command to gsm */
	DRIVER_GSM_Write(gsmHandler->gsm, msgToSend, msgSize);

	/* Read response from gsm  and set response for user*/
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 4000, (const uint8_t*)"OK\r\n")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		if(timer[10] == '1') DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\nTimer is now ON!\r\n");
		else DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\nTimer is now OFF!\r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;

	}
	return DRIVER_OK;
}


/**
  * @brief Set format of sending packets in TCPIP protocol (hexadecimal or decimal format).
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @param timeout      Timeout period for console.
  * @param format		Format of sending packet via TCPIP(hexadecimal-'1' or decimal-'2').
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_SetSendingIPFormat(gsmHandler_t *gsmHandler, uint32_t timeout, uint8_t format)
{
	/* Buffer for putting answer from gsm */
	uint8_t buffer[100] = {0};

	/* How many characters are received from gsm-it's importent to be zero initialize */
	uint32_t size = 0;

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Set command for type of sending format */
	uint8_t msgToSend[20] = {0};
	uint8_t msgSize = 14;
	strcat((char*)msgToSend, "at+cipsendhex=");

	if(format == '1') strcat((char*)msgToSend,"1\r");
	else if(format == '2') strcat((char*)msgToSend,"0\r");
	msgSize += 2;

	/* Send command to gsm */
	DRIVER_GSM_Write(gsmHandler->gsm, msgToSend,msgSize);

	/* Read response from gsm  and set response for user */
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 3000, (const uint8_t*)"OK\r\n")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		if(msgToSend[14] == '1') DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\nHexadecimal format is now ON!\r\n");
		else DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\nDecimal format is now ON!\r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;

	}
	return DRIVER_OK;
}

/**
  * @brief Connect gsm module to specified server.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @param timeout      Timeout period for console.
  * @param inputStruct  Structure that contains needed variables to set PDP context:
  * connectType- type of connection(it can be TCP-'1' or UDP-'2'),
  * ipAddr - IP address of server, port - Port number of server.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_ConnectToServer(gsmHandler_t *gsmHandler, uint32_t timeout,ConnectSrvrInputStruct_t inputStruct)
{
	uint8_t buffer[100] = {0};
	uint32_t size = 0;
	Socket_t socketInit;

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/** Set type of connection **/

    /* Set socket for initialization with his type */
	if(inputStruct.connectType == '1') strcpy((char*)socketInit.type,(const char*)"TCP\0" );
	else if(inputStruct.connectType == '2') strcpy((char*)socketInit.type,(const char*)"UDP\0" );

	/** Set IP address **/

	/* Set socket for initialization with his IP address */
	uint8_t i = strlen((const char*)inputStruct.ipAddr);
	if(strchr((char*)inputStruct.ipAddr,'\r') != NULL )
	{	/* Length of IP address characters without '\r' */
		i--;
	}
	strcpy((char*)socketInit.IPaddress,(const char*)inputStruct.ipAddr);
	socketInit.IPaddress[i] = '\0';

	/** Set port **/

	uint32_t portNum = 0;
	uint32_t weight = 1;
	i = strlen((const char*)inputStruct.port);
	if(strchr((char*)inputStruct.port,'\r') != NULL )
	{	/* Length of port characters without '\r' */
		i--;
	}

	/* Save port number in characters and set socket port */
	uint8_t portChar[6] = {0};
	strcpy((char*)portChar,(const char*)inputStruct.port);
	portChar[i] = '\0';

	/* Convert port number from characters value stored in buffer
	 * to be integer type, base 16 */
	while(i>0)
	{
		i--;
		portNum += (inputStruct.port[i] - '0')*weight;
		weight *= 10;
	}
	socketInit.port = portNum;


	/* Set message for activating connection with server for gsm module */
	uint8_t msgToSend[100];
	uint8_t msgSize = 0;
	memset(msgToSend,0, sizeof(msgToSend));
	strcat((char*) msgToSend,(const char*) "at+cipstart=");
	strcat((char*) msgToSend,(const char*) "\"");
	strcat((char*) msgToSend,(const char*) socketInit.type);
	strcat((char*) msgToSend,(const char*) "\"");
	strcat((char*) msgToSend,(const char*) ",");
	strcat((char*) msgToSend,(const char*) "\"");
	strcat((char*) msgToSend,(const char*) socketInit.IPaddress);
	strcat((char*) msgToSend,(const char*) "\"");
	strcat((char*) msgToSend,(const char*) ",");
	strcat((char*) msgToSend,(const char*) portChar);
	strcat((char*) msgToSend,(const char*) "\r");

	socketInit.PDPcontextNo = gsmHandler->activeSocketNo;

	/* Set size of message to send to gsm module */
	while(msgToSend[msgSize] != '\r') msgSize++;
	msgSize++;

	/* Send command to gsm module for connecting to server */
	DRIVER_GSM_Write(gsmHandler->gsm, msgToSend, msgSize);

	/* Read response from gsm  and set response for user */
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 7000, (const uint8_t*)"OK")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n Connection with server started!\r\n");
		GSM_SetSocket(gsmHandler,&socketInit);
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;
	}
	return DRIVER_OK;
}

/**
  * @brief Disonnect gsm module to specified server.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_DisconnectFromServer(gsmHandler_t *gsmHandler)
{
	uint8_t buffer[100];

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	uint32_t size = 0;

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Send command to dissconnect from server */
	DRIVER_GSM_Write(gsmHandler->gsm, (const uint8_t*)"at+cipclose\r", sizeof("at+cipclose\r"));

	/* Read response from gsm  and set response for user */
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 5000, (const uint8_t*)"OK")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n Connection with server ended!\r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;
	}
	return DRIVER_OK;
}

/**
  * @brief Check IP connection for all channels(number of channels is preveously defined with macro MAX_SOCKET_NUMBER).
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_CheckConnection(gsmHandler_t *gsmHandler)
{
	uint8_t buffer[100];

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	uint32_t size = 0;

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Send command check connection with server */
	DRIVER_GSM_Write(gsmHandler->gsm, (const uint8_t*)"at+cipstatus\r", sizeof("at+cipstatus\r"));

	/* Take current time and check if timeout occured */
	uint32_t tickstart = TIME_GetTick();
	DRIVERState_t StateReadCmd = DRIVER_OK;
	while(TIME_GetTick() - tickstart < 2000)
	{
		DRIVER_GSM_Read(gsmHandler->gsm, buffer, &size);
	}

	/* Badly received response from gsm */
	if(strstr((const char*)buffer,(const char*)"ERROR") != NULL)
		StateReadCmd = DRIVER_ERROR;

	uint8_t * startMsg = (uint8_t*)strstr((char*)buffer,"STATE:");
	switch(StateReadCmd){
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n");
		DRIVER_CONSOLE_Put(gsmHandler->console, startMsg);
		DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;
	default:
		break;
	}
	return DRIVER_OK;
}

/**
  * @brief Send data to server.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @param timeout      Timeout period for console.
  * @param message 		String that contains message that will be sent.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_SendToServer(gsmHandler_t *gsmHandler, uint32_t timeout, uint8_t *message)
{
	uint8_t buffer[100] = {0};

	uint32_t size = 0;

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Send command to send data to server */
	DRIVER_GSM_Write(gsmHandler->gsm, (const uint8_t*)"at+cipsend\r", sizeof("at+cipsend\r"));

	/* Read response from gsm  and set response for user */
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 6000, (const uint8_t*)">")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		break;
	}

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	size = 0;

	uint8_t msgToSend[1000] = {0};
	uint32_t msgSize = 0;
	size = strlen((const char*)message);
	strcat((char*)msgToSend,(const char*)message);
	msgSize = size;
	/* If we send message with \r endning character, then we need to set control-z in that place */
	if(strchr((char*)message,'\r') != NULL )
	{
		msgToSend[size -1] = 26; /* <CTRL-Z> character */
	}
	else
	{
		msgToSend[size] = 26; /* <CTRL-Z> character */
		msgSize++;
	}
	/* Reset buffer and hos size */
	memset(buffer,0,sizeof(buffer));
	size = 0;

	/* Send message to server */
	DRIVER_GSM_Write(gsmHandler->gsm, msgToSend, msgSize);

	/* Read response from gsm  and set response for user */
	switch(waitUntil(gsmHandler->gsm, buffer, &size, 6000, (const uint8_t*)"OK")){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received from gsm! Try again or restart system! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n Data sent to server!\r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;
	}
	return DRIVER_OK;
}


/**
  * @brief Establish TCP Client connection with specified server.
  * @param gsmHandler   GSM handle that contains everything about gsm module.
  * @param timeout      Timeout period for console.
  * @retval DRIVERState_t status
  */
DRIVERState_t EstablishTCPClientConnection(gsmHandler_t *gsmHandler, uint32_t timeout)
{
	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm module!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Set mobile network, active pdp context, connect to server and set sending format of TCPIP protocol */
	if(GSM_NetworkRegistered(gsmHandler) == DRIVER_OK)
	{
	  if(GSM_ActivePDPContext(gsmHandler,timeout,(const uint8_t*)"1\r") == DRIVER_OK)
	  {
			/* Set input structure */
			ConnectSrvrInputStruct_t inputStruct;

			inputStruct.connectType = '1';
			inputStruct.ipAddr = (uint8_t*)"5.196.95.208";
			inputStruct.port = (uint8_t*)"1883";
			if(GSM_ConnectToServer(gsmHandler,timeout,inputStruct) == DRIVER_OK)
			{
				if(GSM_SetSendingIPFormat(gsmHandler,timeout,'1') == DRIVER_OK)
					return DRIVER_OK;
				else
					return DRIVER_ERROR;
			}
		  else return DRIVER_ERROR;
	  }
	  else return DRIVER_ERROR;
	}
	else return DRIVER_ERROR;

}
