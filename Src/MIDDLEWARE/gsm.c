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
    (#) Set echo mode in gsm modul
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

	(#) Registers gsm modul to mobile network station in Serbia region
	(#) Deregisters gsm modul from mobile network
	(#) Check if it is gsm modul registered on the network
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

	(#) onlyPutNumber() is function that checks users input and demanding only to put number
	(#) waitUntil() function is used to wait response from gsm by using TIME_Delay() function
	 	which uses timer for timing
  @endverbatim
  *
  **********************************************************************************************************************
  */

#include <gsm.h>

/* Set default format of messages */
GSMMsgFormat_t format = GSM_TEXT_MODE;

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
	if(handler == NULL || config == NULL) return DRIVER_ERROR;

	handler->gsm = config->gsm;

	handler->console = config->console;

	uint8_t i = 0;
	for(; i != MAX_SOCKET_NUMBER; i++)
	{
		handler->socket[i].status = SOCKET_CLOSE;
		memset(handler->socket[i].IPaddress,0,sizeof(handler->socket[i].IPaddress));
		memset(handler->socket[i].type,0,4);
		handler->socket[i].PDPcontextNo = CONTEXT_NON;
		handler->socket[i].port = PORT_NON;
	}

	handler->activeSocketNo = 0;

	handler->numSocketOpen = 0;

	handler->network.status = NETWORK_DISCONNECTED;

	memset(handler->network.IPaddress,0,sizeof(handler->socket[i].IPaddress));

	return DRIVER_OK;
}


/**
  * @brief Set echo using AT command.
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
  * @param timeout      Timeout period for console.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_SetEcho(gsmHandler_t *gsmHandler, uint32_t timeout)
{
	/* Buffer for putting answer from gsm */
	uint8_t buffer[100] = {0};

	/* How many characters are received from gsm-it's importent to be zero initialize */
	uint32_t size = 0;

	/* Buffer of response from console about echo mode */
	GSMEcho_t echoOnOFF = GSM_ECHO_ON;

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError: incorrect console and gsm modul!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Ask user witch mode of echo he wants */
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"Enter 1 or 2 :\r\n 1: echo ON \r\n 2: echo OFF \r\n");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");


	/* Response from user */
	uint8_t incorrectInput = 1;
	while(incorrectInput != 3)
	{
		/* function that request from user only number for answer */
		switch(onlyPutNumber(gsmHandler->console, buffer, &size, sizeof(buffer), timeout)){
		case DRIVER_TIMEOUT:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_TIMEOUT;
		case DRIVER_ERROR:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_ERROR;
		case DRIVER_OK:
			if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_OK;
			}
			break;
		}

		if(*buffer == '1') { echoOnOFF = GSM_ECHO_ON; break;}
		else if(*buffer == '2') {echoOnOFF = GSM_ECHO_OFF; break;}
		else
		{

			if(incorrectInput == 3)
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_ERROR;
			}
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Enter number 1 or 2!\r\n ");
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");
			memset(buffer,0,sizeof(buffer));
			size = 0;
			incorrectInput++;
		}
	}

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
		if(echoOnOFF == GSM_ECHO_ON) DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\nEcho is now ON!\r\n");
		else DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\nEcho is now OFF!\r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;

	}
	return DRIVER_OK;
}

/**
  * @brief Set message format, for sending and receiving message, using AT command.
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
  * @param timeout      Timeout period for console.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_MsgFormat(gsmHandler_t *gsmHandler, uint32_t timeout)
{
	/* Buffer for putting answer from gsm */
	uint8_t buffer[100] = {0};

	/* How many characters are received from gsm */
	uint32_t size = 0;

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Ask user witch mode he wants */
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nEnter 1 or 2 :\r\n 1: SMS text mode \r\n 2: SMS pdu mode \r\n ");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");

	/* Response from user */
	uint8_t incorrectInput = 1;
	while(incorrectInput != 3)
	{
		/* function that request from user only number for answer */
		switch(onlyPutNumber(gsmHandler->console, buffer, &size, sizeof(buffer), timeout)){
		case DRIVER_TIMEOUT:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_TIMEOUT;
		case DRIVER_ERROR:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_ERROR;
		case DRIVER_OK:
			if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_OK;
			}
			break;
		}

		if(*buffer == '1') {format = GSM_TEXT_MODE; break;}
		else if(*buffer == '2') {format = GSM_PDU_MODE; break;}
		else
		{

			if(incorrectInput == 3)
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_ERROR;
			}
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Enter number 1 or 2!\r\n ");
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");
			memset(buffer,0,sizeof(buffer));
			size = 0;
			incorrectInput++;
		}
	}

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	size = 0;

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
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n");
		if(format == GSM_TEXT_MODE) DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"SMS text mode now is set!");
		else DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"SMS pdu mode now is set!");
		DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;

	}
	return DRIVER_OK;
}

/**
  * @brief Set message storage using AT command.
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
  * @param timeout      Timeout period for console.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_SetMsgStorage(gsmHandler_t *gsmHandler, uint32_t timeout)
{

	/* the phone message storage area,"ME" = 1 */
	/* the SIM message storage area,"SM" = 2 */
	/* the phone message storage area and SIM message storage area,"MT" = 3 */

	/* Memory for reading and delating messages */
	uint8_t memMsgReadDelate;

	/* Memory for writing and sending messages */
	uint8_t memMsgWriteSend;

	/* Memory for receiving messages */
	uint8_t memMsgReceive;

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
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Ask user witch memory he wants to use */
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Enter number for storage memory for reading and deleting messages :"
		  "\r\n 1 phone memory \r\n 2 SIM memory \r\n");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");

	/* Response from user */
	uint8_t incorrectInput = 1;
	while(incorrectInput != 3)
	{
		/* function that request from user only number for answer */
		switch(onlyPutNumber(gsmHandler->console, buffer, &size, sizeof(buffer), timeout)){
		case DRIVER_TIMEOUT:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_TIMEOUT;
		case DRIVER_ERROR:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_ERROR;
		case DRIVER_OK:
			if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_OK;
			}
			break;
		}

		if(*buffer == '1') {memMsgReadDelate = 1; break;}
		else if(*buffer == '2') {memMsgReadDelate = 2; break;}
		else
		{

			if(incorrectInput == 3)
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_ERROR;
			}
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Enter number 1 or 2!\r\n ");
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");
			memset(buffer,0,sizeof(buffer));
			size = 0;
			incorrectInput++;
		}
	}

	memset(buffer,0,sizeof(buffer));
	size = 0;

	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n  Enter number for storage memory for writing and sending messages :"
		  "\r\n 1 phone memory \r\n 2 SIM memory \r\n");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");

	/* Response from user */
	incorrectInput = 1;
	while(incorrectInput != 3)
	{
		/* function that request from user only number for answer */
		switch(onlyPutNumber(gsmHandler->console, buffer, &size, sizeof(buffer), timeout)){
		case DRIVER_TIMEOUT:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_TIMEOUT;
		case DRIVER_ERROR:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_ERROR;
		case DRIVER_OK:
			if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_OK;
			}
			break;
		}

		/* If buffer is good, leave while loop */
		if(*buffer == '1') {memMsgWriteSend = 1; break;}
		else if(*buffer == '2') {memMsgWriteSend = 2; break;}
		else
		{

			if(incorrectInput == 3)
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_ERROR;
			}
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Enter number 1 or 2!\r\n ");
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");
			memset(buffer,0,sizeof(buffer));
			size = 0;
			incorrectInput++;
		}
	}

	memset(buffer,0,sizeof(buffer));
	size = 0;

	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n  Enter number for storage memory for receiving messages :"
		  "\r\n 1 phone memory \r\n 2 SIM memory \r\n ");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");

	/* Response from user */
	uint32_t tickstart = TIME_GetTick();
	while(TIME_GetTick() - tickstart < timeout)
	{
		/* function that request from user only number for answer */
		switch(onlyPutNumber(gsmHandler->console, buffer, &size, sizeof(buffer), timeout)){
		case DRIVER_TIMEOUT:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
			return DRIVER_TIMEOUT;
		case DRIVER_ERROR:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
			return DRIVER_ERROR;
		case DRIVER_OK:
			if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_OK;
			}
			break;
		}

		if(*buffer == '1') {memMsgReceive = 1; break;}
		else if(*buffer == '2') {memMsgReceive = 2; break;}
		else
		{
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Enter number 1 or 2!\r\n ");
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");
			memset(buffer,0,sizeof(buffer));
			size = 0;
		}
	}

	memset(buffer,0,sizeof(buffer));
	size = 0;

	/* Possible set of command for the type of storage */
	if(memMsgReadDelate == 2 && memMsgWriteSend == 2 && memMsgReceive == 2)
		DRIVER_GSM_Write(gsmHandler->gsm, (const uint8_t*)"at+cpms=\"SM\",\"SM\",\"SM\"\r",sizeof("at+cpms=\"SM\",\"SM\",\"SM\"\r"));
	else if(memMsgReadDelate == 2 && memMsgWriteSend == 2 && memMsgReceive == 1)
		DRIVER_GSM_Write(gsmHandler->gsm, (const uint8_t*)"at+cpms=\"SM\",\"SM\",\"ME\"\r",sizeof("at+cpms=\"SM\",\"SM\",\"ME\"\r"));
	else if(memMsgReadDelate == 2 && memMsgWriteSend == 1 && memMsgReceive == 2)
		DRIVER_GSM_Write(gsmHandler->gsm, (const uint8_t*)"at+cpms=\"SM\",\"ME\",\"SM\"\r",sizeof("at+cpms=\"SM\",\"ME\",\"SM\"\r"));
	else if(memMsgReadDelate == 2 && memMsgWriteSend == 1 && memMsgReceive == 1)
		DRIVER_GSM_Write(gsmHandler->gsm, (const uint8_t*)"at+cpms=\"SM\",\"ME\",\"ME\"\r",sizeof("at+cpms=\"SM\",\"ME\",\"ME\"\r"));
	else if(memMsgReadDelate == 1 && memMsgWriteSend == 2 && memMsgReceive == 2)
		DRIVER_GSM_Write(gsmHandler->gsm, (const uint8_t*)"at+cpms=\"ME\",\"SM\",\"SM\"\r",sizeof("at+cpms=\"ME\",\"SM\",\"SM\"\r"));
	else if(memMsgReadDelate == 1 && memMsgWriteSend == 2 && memMsgReceive == 1)
		DRIVER_GSM_Write(gsmHandler->gsm, (const uint8_t*)"at+cpms=\"ME\",\"SM\",\"ME\"\r",sizeof("at+cpms=\"ME\",\"SM\",\"ME\"\r"));
	else if(memMsgReadDelate == 1 && memMsgWriteSend == 1 && memMsgReceive == 2)
		DRIVER_GSM_Write(gsmHandler->gsm, (const uint8_t*)"at+cpms=\"ME\",\"ME\",\"SM\"\r",sizeof("at+cpms=\"ME\",\"ME\",\"SM\"\r"));
	else if(memMsgReadDelate == 1 && memMsgWriteSend == 1 && memMsgReceive == 1)
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
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
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
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
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
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
  * @param timeout      Timeout period for console.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_ListMsg(gsmHandler_t *gsmHandler, uint32_t timeout)
{
	/* Buffer for reading message */
	uint8_t buffer[1000] = {0};

	/* How many characters were received */
	uint32_t size = 0;

	/* First variable is for pdu format, second is for text format*/
	uint8_t status[] = {0};
	uint8_t statusStr[11] = {0};
	uint8_t statusSize = 0;

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Set command for formating message */
	uint8_t msgFormat[] = {'a','t','+','c','m','g','f','=', format == GSM_TEXT_MODE? '1':'0','\r'};

	/* Send command to gsm */
	DRIVER_GSM_Write(gsmHandler->gsm, msgFormat,sizeof(msgFormat));

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
		break;
	}

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	size = 0;

	/* Request to user */
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Which type of message would you like to list?\r\n");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)" 1 Received unread message\r\n 2 Received read message\r\n");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)" 3 Stored unsent message\r\n 4 Stored sent message\r\n");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)" 5 All messages \r\n ");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"Enter number: \r\n ");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");

	/* Response from user */
	uint8_t incorrectInput = 1;
	while(incorrectInput != 3)
	{
		/* function that request from user only number for answer */
		switch(onlyPutNumber(gsmHandler->console, buffer, &size, sizeof(buffer), timeout)){
		case DRIVER_TIMEOUT:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again command! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_TIMEOUT;
		case DRIVER_ERROR:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_ERROR;
		case DRIVER_OK:
			if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_OK;
			}
			break;
		}

		if(*buffer == '1') {status[0] = '1'; strcat((char*)statusStr,(const char*)"REC UNREAD");statusSize = 10;break;}
		else if(*buffer == '2') {status[0] = '2';strcat((char*)statusStr,(const char*)"REC READ");statusSize = 8;break;}
		else if(*buffer == '3') {status[0] = '3';strcat((char*)statusStr,(const char*)"STO UNSENT");statusSize = 10;break;}
		else if(*buffer == '4') {status[0] = '4';strcat((char*)statusStr,(const char*)"STO SENT");statusSize = 8;break;}
		else if(*buffer == '5') {status[0] = '5';strcat((char*)statusStr,(const char*)"ALL");statusSize = 3;break;}
		else
		{

			if(incorrectInput == 3)
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_ERROR;
			}
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)("\r\n Put correct number!\r\n "));
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");
			memset(buffer,0,sizeof(buffer));
			size = 0;
			incorrectInput++;
		}
	}

	if(incorrectInput == 3)
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	}
	else incorrectInput = 1;

	/* Set command about formating command for gsm */
	uint8_t msgToSend[50] = {0};
	strcat((char*)msgToSend,(const char*)"at+cmgl=");
	uint8_t msgSize = 8;
	if(format == GSM_TEXT_MODE)
	{
		msgToSend[8]= '\"';
		strcat((char*)msgToSend,(const char*)statusStr);
		msgToSend[8 + statusSize + 1] = '\"';
		msgToSend[8 + statusSize + 2]= '\r';
		msgSize = 8 + 3 + statusSize;
	}
	else
	{
		msgToSend[msgSize++] = status[0];
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
    uint32_t endofMsg = 0;
    uint32_t startofMsg = 0;
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
			uint8_t msgtoDisplay[1000] = {0};
			for(uint32_t i = startofMsg; i < endofMsg; i++)
			{
				msgtoDisplay[i - startofMsg] = buffer[i];
			}
			DRIVER_CONSOLE_Put(gsmHandler->console, msgtoDisplay);
			DRIVER_GSM_Flush(gsmHandler->gsm);
		}
		else if(startOK != NULL && startCMGL == NULL )
		{
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"Storage empty, no messages this type!\r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
		}
		return DRIVER_OK;
	}
	return DRIVER_OK;
}

/**
  * @brief Read message using at command.
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
  * @param timeout      Timeout period for console.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_ReadMsg(gsmHandler_t *gsmHandler, uint32_t timeout)
{
	/* Buffer for reading message */
	uint8_t buffer[500] = {0};

	/* How many characters were received */
	uint32_t size = 0;

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Enter index of message to read (from 1 to number of messages): \r\n ");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");

	/* Response from user */
	uint8_t incorrectInput = 1;
	while(incorrectInput != 3)
	{
		/* function that request from user only number for answer */
		switch(onlyPutNumber(gsmHandler->console, buffer, &size, sizeof(buffer), timeout)){
		case DRIVER_TIMEOUT:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_TIMEOUT;
		case DRIVER_ERROR:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_ERROR;
		case DRIVER_OK:
			if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_OK;
			}
			break;
		}
		if(*buffer != '0') break;
		else
		{
			if(incorrectInput == 3)
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_ERROR;
			}
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Enter number greater then 0!\r\n ");
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");
			memset(buffer,0,sizeof(buffer));
			size = 0;
		}

	}

	/* Set command reading message from gsm */
	uint8_t msgToSend[50] = {0};
	strcat((char*)msgToSend,(const char*)"at+cmgr=");
	uint8_t msgSize = 8;
	uint8_t i = 0;
	for(i = 0; buffer[i] != '\r';i++)
	{
		msgToSend[8 + i] = buffer[i];
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
			for(uint32_t i = startofMsg; i < endofMsg; i++)
			{
				msgtoDisplay[i - startofMsg] = buffer[i];
			}
			DRIVER_CONSOLE_Put(gsmHandler->console, msgtoDisplay);
			DRIVER_GSM_Flush(gsmHandler->gsm);
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
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
  * @param timeout      Timeout period for console.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_DeleteMsg(gsmHandler_t *gsmHandler, uint32_t timeout)
{
	/* Buffer for reading message */
	uint8_t buffer[500] = {0};

	/* How many characters were received */
	uint32_t size = 0;

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Request to user */
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Enter which type of messagees would you like to delete: \r\n ");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"1 Certain message \r\n 2 All messages of a particular type \r\n ");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");

	/* Response from user */
	uint8_t incorrectInput = 1;
	while(incorrectInput != 3)
	{
		/* function that request from user only number for answer */
		switch(onlyPutNumber(gsmHandler->console, buffer, &size, sizeof(buffer), timeout)){
		case DRIVER_TIMEOUT:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_TIMEOUT;
		case DRIVER_ERROR:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_ERROR;
		case DRIVER_OK:
			if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_OK;
			}
			break;
		}

		if(*buffer == '1') break;
		else if(*buffer == '2') break;
		else
		{

			if(incorrectInput == 3)
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_ERROR;
			}
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Enter number 1 or 2!\r\n ");
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");
			memset(buffer,0,sizeof(buffer));
			size = 0;
			incorrectInput++;
		}
	}

	/* Set type of preveous response and reset bufer and his size */
	uint8_t deleteType = *buffer;
	memset(buffer,0,sizeof(buffer));
	size = 0;

	/* Request to user */
	if(deleteType == '1')
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nWhich is the index of the message you want to delete? Enter number:\r\n ");
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");

		/* Response from user */
		uint32_t tickstart = TIME_GetTick();
		while(TIME_GetTick() - tickstart < timeout)
		{
			/* function that request from user only number for answer */
			switch(onlyPutNumber(gsmHandler->console, buffer, &size, sizeof(buffer), timeout)){
			case DRIVER_TIMEOUT:
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_TIMEOUT;
			case DRIVER_ERROR:
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_ERROR;
			case DRIVER_OK:
				if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
				{
					DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
					DRIVER_GSM_Flush(gsmHandler->gsm);
					return DRIVER_OK;
				}
				break;
			}
			break;
		}
	}
	else
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nWhich type of message you want to delete?\r\n ");
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"1 Delete all received read messages?\r\n ");
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"2 Delete all received read and stored sent messages?\r\n ");
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"3 Delete all received read, stored sent messages and stored unsent messages?\r\n ");
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"4 Delete all messages of any type?\r\n ");
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");

		/* Response from user */
		incorrectInput = 1;
		while(incorrectInput != 3)
		{
			/* function that request from user only number for answer */
			switch(onlyPutNumber(gsmHandler->console, buffer, &size, sizeof(buffer), timeout)){
			case DRIVER_TIMEOUT:
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_TIMEOUT;
			case DRIVER_ERROR:
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_ERROR;
			case DRIVER_OK:
				if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
				{
					DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
					DRIVER_GSM_Flush(gsmHandler->gsm);
					return DRIVER_OK;
				}
				break;
			}

			if(*buffer == '1') break;
			else if(*buffer == '2') break;
			else if(*buffer == '3') break;
			else if(*buffer == '4') break;
			else
			{

				if(incorrectInput == 3)
				{
					DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
					DRIVER_GSM_Flush(gsmHandler->gsm);
					return DRIVER_ERROR;
				}
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Enter number 1, 2, 3 or 4! \r\n ");
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");
				memset(buffer,0,sizeof(buffer));
				size = 0;
				incorrectInput++;
			}
		}
	}

	/* Set gsm command for deleting message*/
	uint8_t msgToSend[50] = {0};
	strcat((char*)msgToSend,"at+cmgd=");
	uint32_t msgSize = 8;

	if(deleteType == '1')
	{
		strcat((char*)msgToSend,(const char*)buffer);
		msgSize += size;
	}
	else
	{
		/* It's necessary to put index in first place, then to put flag(buffer) on second*/
		msgToSend[msgSize] = '1';
		msgSize++;
		msgToSend[msgSize] = ',';
		msgSize++;
		strcat((char*)msgToSend,(const char*)buffer);
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
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Message(s) are deleted correctly!\r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;

	}
	return DRIVER_OK;
}

/**
  * @brief Send message using at command.
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
  * @param timeout      Timeout period for console.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_SendStoreMsg(gsmHandler_t *gsmHandler, uint32_t timeout)
{
	/* Buffer for reading message */
	uint8_t buffer[500] = {0};

	/* How many characters were received */
	uint32_t size = 0;

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Set command for formating message */
	uint8_t msgFormat[] = {'a','t','+','c','m','g','f','=', format == GSM_TEXT_MODE? '1':'0','\r'};

	/* Send command to gsm */
	DRIVER_GSM_Write(gsmHandler->gsm, msgFormat,sizeof(msgFormat));

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
		break;
	}

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	size = 0;

	/* Request to user */
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Enter number whether you want the message to be send immediately or first stored then later sent: \r\n ");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"1 Immediately send\r\n 2 First store\r\n");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");

	/* Response from user */
	uint8_t incorrectInput = 1;
	while(incorrectInput != 3)
	{
		/* function that request from user only number for answer */
		switch(onlyPutNumber(gsmHandler->console, buffer, &size, sizeof(buffer), timeout)){
		case DRIVER_TIMEOUT:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_TIMEOUT;
		case DRIVER_ERROR:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_ERROR;
		case DRIVER_OK:
			if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_OK;
			}
			break;
		}

		if(*buffer == '1') break;
		else if(*buffer == '2') break;
		else
		{

			if(incorrectInput == 3)
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_ERROR;
			}

			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Enter number 1 or 2!\r\n ");
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");
			memset(buffer,0,sizeof(buffer));
			size = 0;
			incorrectInput++;
		}
	}

	incorrectInput = 1;

	/* Message to send to gsm */
	uint8_t msgToSend[50] = {0};
	uint8_t sendOrStore[1] = {*buffer}; /* Variable for writing on the end of function if message is sent or stored to console */
	uint8_t msgSize = 0;

	/* Ask if user want to send message from storage or directly send message we have to ask user that
	 * and set command from gsm according to his response */
	if(*buffer == '1')
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"Do you want to send message from storage or directly send? Enter number:\r\n");
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"1 Send from storage\r\n2 Send directly\r\n");
		memset(buffer,0,sizeof(buffer));
		size = 0;

		/* Response from user */
		while(incorrectInput != 3)
		{
			/* function that request from user only number for answer */
			switch(onlyPutNumber(gsmHandler->console, buffer, &size, sizeof(buffer), timeout)){
			case DRIVER_TIMEOUT:
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_TIMEOUT;
			case DRIVER_ERROR:
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_ERROR;
			case DRIVER_OK:
				if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
				{
					DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
					DRIVER_GSM_Flush(gsmHandler->gsm);
					return DRIVER_OK;
				}
				break;
			}

			if(*buffer == '1') break;
			else if(*buffer == '2') break;
			else
			{

				if(incorrectInput == 3)
				{
					DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
					DRIVER_GSM_Flush(gsmHandler->gsm);
					return DRIVER_ERROR;
				}

				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Enter number 1 or 2!\r\n ");
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");
				memset(buffer,0,sizeof(buffer));
				size = 0;
				incorrectInput++;
			}
		}

		if(*buffer == '1')
		{
			strcat((char*)msgToSend,"at+cmss=");	/* Send message from storage 	*/
			msgSize = 8;
		}
		else if(*buffer == '2')
		{
			strcat((char*)msgToSend,"at+cmgs=\"");	/* Send message directly		*/
			msgSize = 9;
		}
	}
	/* If we had users response to store message we have to set that command */
	else if(*buffer == '2')
	{
		strcat((char*)msgToSend,"at+cmgw=\"");		/* Store message 				*/
		msgSize = 9;
	}

	incorrectInput = 1;
	memset(buffer,0,sizeof(buffer));
	size = 0;


	/** If message need to be sent from storage we have to demand index of that message **/

	if(strstr((char*)msgToSend,"at+cmss=") != NULL)
	{

		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Which is index of message you want to send?\r\n");

		/* Response from user */
		while(incorrectInput != 3)
		{
			/* function that request from user only number for answer */
			switch(onlyPutNumber(gsmHandler->console, buffer, &size, sizeof(buffer), timeout)){
			case DRIVER_TIMEOUT:
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_TIMEOUT;
			case DRIVER_ERROR:
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_ERROR;
			case DRIVER_OK:
				if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
				{
					DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
					DRIVER_GSM_Flush(gsmHandler->gsm);
					return DRIVER_OK;
				}
				break;
			}

			if(*buffer == '0')
			{

				if(incorrectInput == 3)
				{
					DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
					DRIVER_GSM_Flush(gsmHandler->gsm);
					return DRIVER_ERROR;
				}

				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Enter greater then 0!\r\n ");
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");
				memset(buffer,0,sizeof(buffer));
				size = 0;
				incorrectInput++;
			}
			else break;
		}

		strcat((char*)msgToSend,(const char*)buffer);
		msgSize++;
		msgToSend[msgSize] = ',';
		msgSize++;
		msgToSend[msgSize] = '\"';
		msgSize++;
	}

	/** We need to ask user for number whom he want to send message **/

	/* Request to user */
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Enter mobile phone number whom you want to send message: \r\n ");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");

	/* Response from user */
	while(incorrectInput != 3)
	{
		/* function that request from user only number for answer */
		switch(DRIVER_CONSOLE_Get(gsmHandler->console, buffer, &size, timeout)){
		case DRIVER_TIMEOUT:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_TIMEOUT;
		case DRIVER_ERROR:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_ERROR;
		case DRIVER_OK:
			if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_OK;
			}
			break;
		}
		uint8_t i = 0;
		for(i = 0; i < size - 1; i++)
		{
			if((buffer[i] < '0') || (buffer[i] > '9'))
			{
				if(buffer[i] != '+')
				{

					if(incorrectInput == 3)
					{
						DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
						DRIVER_GSM_Flush(gsmHandler->gsm);
						return DRIVER_ERROR;
					}
					DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Enter only NUMBER greater then 0!\r\n");
					incorrectInput++;
					break;
				}
			}
		}

		/* If all characters are good, leave while, otherwise demand new input */
		if(i == size - 1 && *buffer != '\r') break;
		else
		{
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");
			memset(buffer,0,sizeof(buffer));
			size = 0;
		}
	}

	/* Set number in buffer(msgToSend) for seding number to gsm  */
	uint8_t countryCode[] = "+381";
	switch(*buffer){
	case '+':
		strcat((char*)msgToSend,(const char*)buffer);
		msgSize += size;
		break;
	case '0':
		strcat((char*)msgToSend,(const char*)countryCode);
		msgSize += sizeof(countryCode) - 1;
		uint8_t strNum[20] = {0};
		uint8_t i = 1;
		for(i = 1;i < size; i++) strNum[i - 1] = buffer[i];
		strcat((char*)msgToSend, (const char*)strNum);
		msgSize += i - 1;
		break;
	default:
		strcat((char*)msgToSend, (const char*)countryCode);
		strcat((char*)msgToSend, (const char*)buffer);
		msgSize += sizeof(countryCode) + size - 1;
		break;
	}
	msgToSend[msgSize - 1] = '\"';
	/* Set last character '\r' 'cause gsm command needs it */
	msgToSend[msgSize] = '\r';
	msgSize++;


	/** If we don't have to send from storage we must frist write number to gsm
	 * and wait for his response witch is going to be character ">" that mean
	 * we have to set him message that we want to send **/

	if(strstr((char*)msgToSend,"at+cmss=") == NULL)
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

		/* Reset buffer and his size */
		memset(buffer,0,sizeof(buffer));
		size = 0;

		/* Request to user */
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Enter message to send: \r\n ");
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");

		/* Receive from user */
		switch(DRIVER_CONSOLE_Get(gsmHandler->console, buffer, &size, timeout)){
		case DRIVER_TIMEOUT:
			*buffer = ESCAPE; /* <ESC> character */
			DRIVER_GSM_Write(gsmHandler->gsm, buffer, 1);
			waitUntil(gsmHandler->gsm, buffer, &size, 5000, (const uint8_t*)"OK");
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_TIMEOUT;
		case DRIVER_ERROR:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received, not enough space for receiving characters! Please update your buffer! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_ERROR;
		case DRIVER_OK:
			if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end sending message */
			{
				*buffer = ESCAPE;
				DRIVER_GSM_Write(gsmHandler->gsm, buffer, 1);
				waitUntil(gsmHandler->gsm, buffer, &size, 5000, (const uint8_t*)"OK");
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Leaving command... \r\nMessage unsent!\r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_OK;
			}
			break;
		}

		memset(msgToSend,0, sizeof(msgToSend));
		msgSize = 0;
		buffer[size - 1] = 26; /* <CTRL-Z> character */
		strcat((char*)msgToSend,(const char*)buffer);
		msgSize = size;
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

// IPADDRESS AND TYPE HAVE TO FINISH WITH '\0' CHARACTER!!
/**
  * @brief Open socket and set his state with parameters defined in socket variable.
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
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
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
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
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
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
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
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
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
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
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
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
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
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
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
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
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
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
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
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
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
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
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
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
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
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
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
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
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
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
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
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
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
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
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
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
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
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
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
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
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
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
  * @brief Set Packet Data Protocol context for gsm modul.
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
  * @param timeout      Timeout period for console.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_SetPDPContext(gsmHandler_t *gsmHandler, uint32_t timeout)
{
	uint8_t buffer[100];
	uint32_t size = 0;

	/* Reset buffer */
	memset(buffer,0,sizeof(buffer));

	/* Check if we have space to open new socket */
	if(GSM_CheckOpenSocketNo(gsmHandler) == SOCKET_FULL)
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: No available sockets for opening! Try closing some socket!\r\n");
		return DRIVER_ERROR;
	}

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);


	/* Request to user */
	uint8_t maxSocNumStr[3];
	memset(maxSocNumStr,0,sizeof(maxSocNumStr));
	itoa(MAX_SOCKET_NUMBER,(char*) maxSocNumStr, 10);
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Enter number of which PDP context to use (range from 1 to ");
	DRIVER_CONSOLE_Put(gsmHandler->console, maxSocNumStr);
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"): \r\n ");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");

	/* Response from user */
	uint8_t incorrectInput = 1;
	while(incorrectInput != 3)
	{
		/* function that request from user only number for answer */
		switch(onlyPutNumber(gsmHandler->console, buffer, &size, sizeof(buffer), timeout)){
		case DRIVER_TIMEOUT:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
			return DRIVER_TIMEOUT;
		case DRIVER_ERROR:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
			return DRIVER_ERROR;
		case DRIVER_OK:
			if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_OK;
			}
			break;
		}

		if( (buffer[0] == '1' && buffer[1] == '\r') || (buffer[0] == '2' && buffer[1] == '\r') || (buffer[0] == '3' && buffer[1] == '\r') ||
		    (buffer[0] == '4' && buffer[1] == '\r') || (buffer[0] == '5' && buffer[1] == '\r') || (buffer[0] == '6' && buffer[1] == '\r') ||
			(buffer[0] == '7' && buffer[1] == '\r') || (buffer[0] == '8' && buffer[1] == '\r') || (buffer[0] == '9' && buffer[1] == '\r') ||
			(buffer[0] == '1' && buffer[1] == '0')  || (buffer[0] == '1' && buffer[1] == '1')  || (buffer[0] == '1' && buffer[1] == '2')  ||
			(buffer[0] == '1' && buffer[1] == '3')  || (buffer[0] == '1' && buffer[1] == '4')  || (buffer[0] == '1' && buffer[1] == '5')  ||
			(buffer[0] == '1' && buffer[1] == '6') )
		{
			break;
		}
		else
		{
			if(incorrectInput == 3)
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError, failed input attempt! Please try again command!!\r\n ");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_ERROR;
			}
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)("\r\n Put correct number!\r\n "));
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");
			memset(buffer,0,sizeof(buffer));
			size = 0;
			incorrectInput++;
		}
	}

	/* Make pointer for storing message to send to gsm modul */
	uint8_t msgToSend[200] = {0};
	uint32_t msgSize = 0;

	/* Set command for echo mode */
	uint8_t PDPContextID[14];
	memset(PDPContextID,0,sizeof(PDPContextID));
	strcat((char*)PDPContextID,(const char*)"at+cgdcont=");
	PDPContextID[11] = *buffer;
	if(buffer[1] == '\r')
	{
		PDPContextID[12] = ',';
		PDPContextID[13] = '\0';
		msgSize = 13;
	}
	else
	{
		PDPContextID[12] = buffer[1];
		PDPContextID[13] = ',';
		PDPContextID[14] = '\0';
		msgSize = 14;
	}


	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	size = 0;

	/* Request to user */
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Enter number of witch packet data protocol you will be using:\r\n 1 IP(Internet Protocol)\r\n "
			"2 IPV6(Internet Protocol, version 6)\r\n 3 PPP(Point to Point Protocol)\r\n ");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");

	/* Response from user */
	incorrectInput = 1;
	while(incorrectInput != 3)
	{
		/* function that request from user only number for answer */
		switch(onlyPutNumber(gsmHandler->console, buffer, &size, sizeof(buffer), timeout)){
		case DRIVER_TIMEOUT:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
			return DRIVER_TIMEOUT;
		case DRIVER_ERROR:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
			return DRIVER_ERROR;
		case DRIVER_OK:
			if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_OK;
			}
			break;
		}

		if(*buffer == '1' || *buffer == '2' || *buffer == '3') break;
		else
		{
			if(incorrectInput == 3)
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError, failed input attempt! Please try again command!!\r\n ");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_ERROR;
			}
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)("\r\n Put correct number!\r\n "));
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");
			memset(buffer,0,sizeof(buffer));
			size = 0;
			incorrectInput++;
		}
	}

	/* Set Packet Data Protocol type and
	 * set size of message for sending command to gsm modul */
	uint8_t PDPType[10];
	memset((char*)PDPType,0,sizeof(PDPType));
	if( *buffer == '1') {strcpy((char*)PDPType,(const char*)"\"IP\","); msgSize += 5;}
	else if(*buffer == '2') {strcpy((char*)PDPType,(const char*)"\"IPV6\","); msgSize += 7;}
	else if(*buffer == '3') {strcpy((char*)PDPType,(const char*)"\"PPP\","); msgSize += 6;}

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	size = 0;

	/* Request to user */
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Enter access point name:\r\n ");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");

	/* Response from user */
	switch(DRIVER_CONSOLE_Get(gsmHandler->console, buffer, &size, timeout)){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
		return DRIVER_ERROR;
	case DRIVER_OK:
		if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
		{
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_OK;
		}
		break;
	}

	/* Set access point name */
	uint8_t APNType[100];
	memset(APNType,0,sizeof(APNType));
	uint32_t i = 0;
	while(buffer[i] != '\r'){ APNType[i] = buffer[i]; i++; }

	/* Set size of message for sending command to gsm modul */
	msgSize += size + 2;

	/* Set buffer for sending command to gsm modul to contains message  */
	memset(msgToSend,0, msgSize);
	strcat((char*)msgToSend,(const char*)PDPContextID);
	strcat((char*)msgToSend,(const char*)PDPType);
	strcat((char*)msgToSend,(const char*)"\"");
	strcat((char*)msgToSend,(const char*)APNType);
	strcat((char*)msgToSend,(const char*)"\"\r");

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
  * @brief Check setted Packet Data Protocol context for gsm modul.
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_CheckSettedPDPContext(gsmHandler_t *gsmHandler)
{
	uint8_t buffer[1000];

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	uint32_t size = 0;

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
  * @brief Check active Packet Data Protocol context for gsm modul.
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_CheckActivePDPContext(gsmHandler_t *gsmHandler)
{
	uint8_t buffer[100];

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	uint32_t size = 0;

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
  * @brief Check active Packet Data Protocol context for gsm modul.
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_ShowPDPIP(gsmHandler_t *gsmHandler)
{
	uint8_t buffer[1000];

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	uint32_t size = 0;

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
  * @brief Active Packet Data Protocol context for gsm modul.
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
  * @param timeout      Timeout period for console.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_ActivePDPContext(gsmHandler_t *gsmHandler, uint32_t timeout)
{
	uint8_t buffer[100];
	uint32_t size = 0;
	Socket_t socketInit;

	/* Reset buffer */
	memset(buffer,0,sizeof(buffer));

	/* Check if we have space to open new socket */
	if(GSM_CheckOpenSocketNo(gsmHandler) == SOCKET_FULL)
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: No available sockets for opening! Try closing some socket!\r\n");
		return DRIVER_ERROR;
	}

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Request to user */
	uint8_t maxSocNumStr[3];
	memset(maxSocNumStr,0,sizeof(maxSocNumStr));
	itoa(MAX_SOCKET_NUMBER,(char*) maxSocNumStr, 10);
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Enter number of PDP context (range from 1 to ");
	DRIVER_CONSOLE_Put(gsmHandler->console, maxSocNumStr);
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"): \r\n ");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");

	/* Response from user */
	uint8_t incorrectInput = 1;
	while(incorrectInput != 3)
	{
		/* function that request from user only number for answer */
		switch(onlyPutNumber(gsmHandler->console, buffer, &size, sizeof(buffer), timeout)){
		case DRIVER_TIMEOUT:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
			return DRIVER_TIMEOUT;
		case DRIVER_ERROR:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
			return DRIVER_ERROR;
		case DRIVER_OK:
			if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_OK;
			}
			break;
		}

		if( (buffer[0] == '1' && buffer[1] == '\r') || (buffer[0] == '2' && buffer[1] == '\r') || (buffer[0] == '3' && buffer[1] == '\r') ||
		    (buffer[0] == '4' && buffer[1] == '\r') || (buffer[0] == '5' && buffer[1] == '\r') || (buffer[0] == '6' && buffer[1] == '\r') ||
			(buffer[0] == '7' && buffer[1] == '\r') || (buffer[0] == '8' && buffer[1] == '\r') || (buffer[0] == '9' && buffer[1] == '\r') ||
			(buffer[0] == '1' && buffer[1] == '0')  || (buffer[0] == '1' && buffer[1] == '1')  || (buffer[0] == '1' && buffer[1] == '2')  ||
			(buffer[0] == '1' && buffer[1] == '3')  || (buffer[0] == '1' && buffer[1] == '4')  || (buffer[0] == '1' && buffer[1] == '5')  ||
			(buffer[0] == '1' && buffer[1] == '6') )
		{
			break;
		}
		else
		{
			if(incorrectInput == 3)
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError, failed input attempt! Please try again command!!\r\n ");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_ERROR;
			}
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)("\r\n Put correct number!\r\n "));
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");
			memset(buffer,0,sizeof(buffer));
			size = 0;
			incorrectInput++;
		}
	}

	/* Make pointer for storing message to send to gsm modul */
	uint32_t msgSize = 0;

	/* Set command for activating PDP context */

	/* Set which one PDP context will be open (only set PDPcontextNO)! When
	 * connecting with server set other parameters */
	uint8_t activePDP[14];
	memset(activePDP,0,sizeof(activePDP));
	strcat((char*)activePDP,(const char*)"at+cgact=1,");
	activePDP[11] = *buffer;
	if(buffer[1] == '\r')
	{
		activePDP[12] = '\r';
		msgSize = 13;
		socketInit.PDPcontextNo = *buffer - '0';
	}
	else
	{
		activePDP[12] = buffer[1];
		activePDP[13] = '\r';
		msgSize = 14;
		socketInit.PDPcontextNo = 10 + (buffer[1] - '0');
	}

	memset(socketInit.IPaddress,0, sizeof(socketInit.IPaddress));
	socketInit.port = PORT_NON;
	socketInit.status = SOCKET_SET;
	memset(socketInit.type,0, sizeof(socketInit.type));

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	size = 0;

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
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
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
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
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
  * @brief Deactive Packet Data Protocol context for gsm modul.
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_DeactivePDPContext(gsmHandler_t *gsmHandler, uint32_t timeout)
{
	uint8_t buffer[100];
	uint32_t size = 0;
	Socket_t socketInit;

	/* Reset buffer */
	memset(buffer,0,sizeof(buffer));

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/* Request to user */
	uint8_t maxSocNumStr[3];
	memset(maxSocNumStr,0,sizeof(maxSocNumStr));
	itoa(MAX_SOCKET_NUMBER,(char*) maxSocNumStr, 10);
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Enter number of PDP context (range from 1 to ");
	DRIVER_CONSOLE_Put(gsmHandler->console, maxSocNumStr);
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"): \r\n ");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");

	/* Response from user */
	uint8_t incorrectInput = 1;
	while(incorrectInput != 3)
	{
		/* function that request from user only number for answer */
		switch(onlyPutNumber(gsmHandler->console, buffer, &size, sizeof(buffer), timeout)){
		case DRIVER_TIMEOUT:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
			return DRIVER_TIMEOUT;
		case DRIVER_ERROR:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
			return DRIVER_ERROR;
		case DRIVER_OK:
			if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_OK;
			}
			break;
		}

		if( (buffer[0] == '1' && buffer[1] == '\r') || (buffer[0] == '2' && buffer[1] == '\r') || (buffer[0] == '3' && buffer[1] == '\r') ||
		    (buffer[0] == '4' && buffer[1] == '\r') || (buffer[0] == '5' && buffer[1] == '\r') || (buffer[0] == '6' && buffer[1] == '\r') ||
			(buffer[0] == '7' && buffer[1] == '\r') || (buffer[0] == '8' && buffer[1] == '\r') || (buffer[0] == '9' && buffer[1] == '\r') ||
			(buffer[0] == '1' && buffer[1] == '0')  || (buffer[0] == '1' && buffer[1] == '1')  || (buffer[0] == '1' && buffer[1] == '2')  ||
			(buffer[0] == '1' && buffer[1] == '3')  || (buffer[0] == '1' && buffer[1] == '4')  || (buffer[0] == '1' && buffer[1] == '5')  ||
			(buffer[0] == '1' && buffer[1] == '6') )
		{
			break;
		}
		else
		{
			if(incorrectInput == 3)
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError, failed input attempt! Please try again command!!\r\n ");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_ERROR;
			}
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)("\r\n Put correct number!\r\n "));
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");
			memset(buffer,0,sizeof(buffer));
			size = 0;
			incorrectInput++;
		}
	}

	/* Make pointer for storing message to send to gsm modul */
	uint32_t msgSize = 0;

	/* Set command for activating PDP context */

	/* Set which one PDP context will be closed (only set PDPcontextNO)! When
	 * connecting with server set other parameters */
	uint8_t activePDP[14];
	memset(activePDP,0,sizeof(activePDP));
	strcat((char*)activePDP,(const char*)"at+cgact=0,");
	activePDP[11] = *buffer;
	if(buffer[1] == '\r')
	{
		activePDP[12] = '\r';
		msgSize = 13;
		socketInit.PDPcontextNo = *buffer - '0';
	}
	else
	{
		activePDP[12] = buffer[1];
		activePDP[13] = '\r';
		msgSize = 14;
		socketInit.PDPcontextNo = 10 + (buffer[1] - '0');
	}

	memset(socketInit.IPaddress,0, sizeof(socketInit.IPaddress));
	socketInit.port = PORT_NON;
	socketInit.status = SOCKET_CLOSE;
	memset(socketInit.type,0, sizeof(socketInit.type));

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	size = 0;

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
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
  * @param timeout      Timeout period for console.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_SetAutoSendingTimerIP(gsmHandler_t *gsmHandler, uint32_t timeout)
{
	uint8_t buffer[100];

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	uint32_t size = 0;

	/* Request to user */
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Enter option: \r\n 1 Not set timer \r\n 2 Set timer \r\n ");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");

	/* Response from user */
	uint8_t incorrectInput = 1;
	while(incorrectInput != 3)
	{
		/* function that request from user only number for answer */
		switch(onlyPutNumber(gsmHandler->console, buffer, &size, sizeof(buffer), timeout)){
		case DRIVER_TIMEOUT:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_TIMEOUT;
		case DRIVER_ERROR:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_ERROR;
		case DRIVER_OK:
			if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_OK;
			}
			break;
		}

		if(*buffer == '1' || *buffer == '2') break;
		else
		{

			if(incorrectInput == 3)
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_ERROR;
			}
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Enter number 1 or 2!\r\n ");
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");
			memset(buffer,0,sizeof(buffer));
			size = 0;
			incorrectInput++;
		}
	}

	/* Set command for setting timer mode */
	uint8_t timer[12] = {'a','t','+','c','i','p','a','t','s','=',*buffer == '2' ? '1':'0','\0'};
	uint8_t msgToSend[20] = {0};
	uint8_t msgSize = 11;
	strcat((char*)msgToSend,(const char*)timer);

	if(*buffer == '2')
	{
		/* Reset buffer and his size */
		memset(buffer,0,sizeof(buffer));
		size = 0;

		/* Request to user */
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Enter the seconds after which the data will be sent to server\r\n ");
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");

		/* Response from user */
		incorrectInput = 1;
		while(incorrectInput != 3)
		{
			/* function that request from user only number for answer */
			switch(onlyPutNumber(gsmHandler->console, buffer, &size, sizeof(buffer), timeout)){
			case DRIVER_TIMEOUT:
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_TIMEOUT;
			case DRIVER_ERROR:
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_ERROR;
			case DRIVER_OK:
				if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
				{
					DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
					DRIVER_GSM_Flush(gsmHandler->gsm);
					return DRIVER_OK;
				}
				break;
			}
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
				strcat((char*)msgToSend,(const char*)",");
				strcat((char*)msgToSend,(const char*)buffer);
				msgSize += size + 1;
				break;
			}
			else
			{
				if(incorrectInput == 3)
				{
					DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
					DRIVER_GSM_Flush(gsmHandler->gsm);
					return DRIVER_ERROR;
				}
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Enter number greater then 0!\r\n ");
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");
				memset(buffer,0,sizeof(buffer));
				size = 0;
			}
		}
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
		if(timer[10] == '1') DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\nTimer is now ON!\r\n");
		else DRIVER_CONSOLE_Put(gsmHandler->console, (const uint8_t*)"\r\nTimer is now OFF!\r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_OK;

	}
	return DRIVER_OK;
}

/**
  * @brief Connect gsm modul to specified server.
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
  * @param timeout      Timeout period for console.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_ConnectToServer(gsmHandler_t *gsmHandler, uint32_t timeout)
{
	uint8_t buffer[100];
	uint32_t size = 0;
	Socket_t socketInit;

	/* Reset buffer*/
	memset(buffer,0,sizeof(buffer));

	/* Checking if user send correct gsm and console */
	if(gsmHandler->gsm == NULL && gsmHandler->console == NULL )
	{
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
		return DRIVER_ERROR;
	}
	DRIVER_GSM_Flush(gsmHandler->gsm);

	/** Set type of connection **/

	/* Request to user */
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Do you want to use TCP or UDP type of connection? \r\n Enter number : \r\n 1 TCP \r\n 2 UDP\r\n ");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");

	/* Response from user */
	uint8_t incorrectInput = 1;
	while(incorrectInput != 3)
	{
		/* function that request from user only number for answer */
		switch(onlyPutNumber(gsmHandler->console, buffer, &size, sizeof(buffer), timeout)){
		case DRIVER_TIMEOUT:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
			return DRIVER_TIMEOUT;
		case DRIVER_ERROR:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
			return DRIVER_ERROR;
		case DRIVER_OK:
			if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_OK;
			}
			break;
		}

		if(*buffer == '1' || *buffer == '2' ) break;
		else
		{
			if(incorrectInput == 3)
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_ERROR;
			}
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Enter number 1 or 2!\r\n ");
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");
			memset(buffer,0,sizeof(buffer));
			size = 0;
			incorrectInput++;
		}
	}

    /* Set socket for initialization with his type */
	if(*buffer == '1') strcpy((char*)socketInit.type,(const char*)"TCP\0" );
	else strcpy((char*)socketInit.type,(const char*)"UDP\0" );

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	size = 0;

	/** Set IP address **/

	/* Request to user */
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Insert IP address of server with whom you will connect to:\r\n ");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");

	/* Response from user */
	incorrectInput = 1;
	while(incorrectInput != 3)
	{
		/* Get IP address that contains numbers(0-9) and dots(.) Example: 125.2.0.129 */
		switch(DRIVER_CONSOLE_Get(gsmHandler->console, buffer, &size, timeout)){
		case DRIVER_TIMEOUT:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_TIMEOUT;
		case DRIVER_ERROR:
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_ERROR;
		case DRIVER_OK:
			if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
			{
				DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
				DRIVER_GSM_Flush(gsmHandler->gsm);
				return DRIVER_OK;
			}
			break;
		}
		uint8_t i = 0;
		for(i = 0; i < size - 1; i++)
		{
			/* Check if characters are only numbers from 0 to 9 or dots */
			if((buffer[i] < '0') || (buffer[i] > '9'))
			{
				if(buffer[i] != '.')
				{
					if(incorrectInput == 3)
					{
						DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error, failed input attempt! Please try again command! \r\n");
						DRIVER_GSM_Flush(gsmHandler->gsm);
						return DRIVER_ERROR;
					}
					DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Insert only numbers and dots in form of n.n.n.n , when n is number! \r\n");
					incorrectInput++;
					break;
				}
			}
		}

		/* If all characters are good, leave while, otherwise demand new input */
		if(i == size - 1 && *buffer != '\r') break;
		else
		{
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");
			memset(buffer,0,sizeof(buffer));
			size = 0;
		}
	} // end of while

	/* Set socket for initialization with his IP address */
	uint8_t i = 0;
	for(; buffer[i] != '\r'; i++)
	{
		socketInit.IPaddress[i] =  buffer[i];
	}
	socketInit.IPaddress[i] = '\0';

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	size = 0;

	/** Set port **/

	/* Request to user */
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Insert server port: \r\n");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");

	/* Response from user */
	/* function that request from user only number for answer */
	switch(onlyPutNumber(gsmHandler->console, buffer, &size, sizeof(buffer), timeout)){
	case DRIVER_TIMEOUT:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for input has expired! Please try again comand! \r\n");
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Not enough space for receiving characters from gsm! Increase size of buffer! \r\n");
		return DRIVER_ERROR;
	case DRIVER_OK:
		if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end function */
		{
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Returning to waiting command... \r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_OK;
		}
		break;
	}

	/* Convert port number from characters value stored in buffer
	 * to be integer type, base 16 */
	uint32_t portNum = 0;
	i = 0;
	uint32_t weight = 1;
	while(buffer[i] != '\r') i++;
	while(i>0)
	{
		i--;
		portNum += (buffer[i] - '0')*weight;
		weight *= 10;
	}

	/* Save port number in characters and set socket port */
	uint8_t portChar[6];
	memset(portChar,0,sizeof(portChar));
	for(uint8_t i = 0; buffer[i] != '\r'; i++)
	{
		portChar[i] =  buffer[i];
	}
	socketInit.port = portNum;

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	size = 0;

	/* Set message for activating connection with server for gsm modul */
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

	/* Set size of message to send to gsm modul */
	while(msgToSend[msgSize] != '\r') msgSize++;
	msgSize++;

	/* Send command to gsm modul for connecting to server */
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
  * @brief Disonnect gsm modul to specified server.
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
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
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Error: incorrect console and gsm modul!\r\n");
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
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_CheckConnection(gsmHandler_t *gsmHandler)
{
	uint8_t buffer[100];

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	uint32_t size = 0;

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
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
  * @param timeout      Timeout period for console.
  * @retval DRIVERState_t status
  */
DRIVERState_t GSM_SendToServer(gsmHandler_t *gsmHandler, uint32_t timeout)
{
	uint8_t buffer[100];

	/* Reset buffer and his size */
	memset(buffer,0,sizeof(buffer));
	uint32_t size = 0;

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

	/* Request to user */
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Enter message to send: \r\n ");
	DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)">>");

	/* Receive from user */
	switch(DRIVER_CONSOLE_Get(gsmHandler->console, buffer, &size, timeout)){
	case DRIVER_TIMEOUT:
		*buffer = ESCAPE; /* <ESC> character */
		DRIVER_GSM_Write(gsmHandler->gsm, buffer, 1);
		waitUntil(gsmHandler->gsm, buffer, &size, 5000, (const uint8_t*)"OK");
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError! Time for receiving response from gsm has expired! Please try again command! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_TIMEOUT;
	case DRIVER_ERROR:
		DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\nError received, not enough space for receiving characters! Please update your buffer! \r\n");
		DRIVER_GSM_Flush(gsmHandler->gsm);
		return DRIVER_ERROR;
	case DRIVER_OK:
		if(strstr((char*)buffer,(const char*)"\e") != NULL) /* if escape code <ESC> occurs end sending message */
		{
			*buffer = ESCAPE; /* <ESC> character (decimal 27) */
			DRIVER_GSM_Write(gsmHandler->gsm, buffer, 1);
			waitUntil(gsmHandler->gsm, buffer, &size, 5000, (const uint8_t*)"OK");
			DRIVER_CONSOLE_Put(gsmHandler->console,(const uint8_t*)"\r\n Leaving command... \r\nData not send to server!\r\n");
			DRIVER_GSM_Flush(gsmHandler->gsm);
			return DRIVER_OK;
		}
		break;
	}

	uint8_t msgToSend[100];
	uint32_t msgSize = 0;
	memset(msgToSend,0, sizeof(msgToSend));
	buffer[size - 1] = 26; /* <CTRL-Z> character */
	strcat((char*)msgToSend,(const char*)buffer);
	msgSize = size;

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
  * @param gsmHandler   GSM handle that contains everything about gsm modul.
  * @param timeout      Timeout period for console.
  * @retval DRIVERState_t status
  */
DRIVERState_t EstablishTCPClientConnection(gsmHandler_t *gsmHandler, uint32_t timeout)
{
  if(GSM_NetworkRegistered(gsmHandler) == DRIVER_OK)
  {
	  if(GSM_ActivePDPContext(gsmHandler,timeout) == DRIVER_OK)
	  {
		  if(GSM_ConnectToServer(gsmHandler,timeout) == DRIVER_OK)
		  {
			  return DRIVER_OK;
		  }
		  else return DRIVER_ERROR;
	  }
	  else return DRIVER_ERROR;
  }
  else return DRIVER_ERROR;

}
