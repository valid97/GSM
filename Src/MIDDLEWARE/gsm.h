/**
  ***********************************************************************************************************
  * @file    gsm.h
  * @author  Valentina Denic
  * @brief   This file contains all the functions prototypes for middleware
  *          layer for comunication between gsm modul and other parts of project(console, network connection).
  ***********************************************************************************************************
  */

#ifndef MIDDLEWARE_GSM_H_
#define MIDDLEWARE_GSM_H_

/* Includes -----------------------------------------------------------------------------------------------*/
#include <driver_console.h>
#include <driver_common.h>
#include <driver_gsm.h>
#include <stdlib.h>
#include <time.h>
#include <mqtt.h>

#define MAX_SOCKET_NUMBER 16
#define PORT_NON 65535
#define CONTEXT_NON 255
/**
  * @brief  GSM ECHO Status structures definition
  */
typedef enum
{
  GSM_ECHO_ON		= 0x00,				/*!< Gsm echo status on								*/
  GSM_ECHO_OFF		= 0x01				/*!< Gsm echo status off 							*/
} GSMEcho_t;

/**
  * @brief  GSM MSG MODE Status structures definition
  */
typedef enum
{
  GSM_PDU_MODE		= 0x00,				/*!< Gsm message mode pdu 							*/
  GSM_TEXT_MODE		= 0x01				/*!< Gsm message mode text 							*/
} GSMMsgFormat_t;

/**
  * @brief  GSM MSG LIST STATUS structures definition
  */
typedef enum
{
  REC_UNREAD		= 0x00,				/*!< Gsm listing status unread messages 			*/
  REC_READ			= 0x01,				/*!< Gsm listing status read messages				*/
  STO_UNSENT		= 0x02,				/*!< Gsm listing status unsent messages				*/
  STO_SENT			= 0x03,				/*!< Gsm listing status sent messages				*/
  ALL				= 0x04				/*!< Gsm listing status all messages				*/

} GSMMsgListStatus_t;

/**
  * @brief  Network Status Structure Definition
  */
typedef enum
{
	NETWORK_CONNECTED 		= 0x00,		/*!< Network connection type status connected		*/
	NETWORK_DISCONNECTED 	= 0x01		/*!< Network connection type status disconnected 	*/
}NetworkStatus_t;

/**
  * @brief  Socket Status Structure Definition
  */
typedef enum
{
	SOCKET_OPEN 			= 0x00,		/*!< Socket status opened							*/
	SOCKET_CLOSE 			= 0x01,		/*!< Socket status closed							*/
	SOCKET_FULL 			= 0x02,		/*!< Socket status full								*/
	SOCKET_ALREADY_OPEN 	= 0x03,		/*!< Socket status already opened					*/
	SOCKET_ERROR		 	= 0x04,		/*!< Socket status error							*/
	SOCKET_SET		 		= 0x05,		/*!< Socket status set								*/
	SOCKET_AVAILABLE	 	= 0x06		/*!< Socket status available						*/
}SocketStatus_t;

/**
  * @brief  Socket Structure definition
  */
typedef struct __Socket_t
{
	uint8_t PDPcontextNo;						/*!< Socket PDP context number						*/

	uint8_t type[4];							/*!< Socket connection type(TCP,UDP)				*/

	SocketStatus_t status;						/*!< Socket status									*/

	uint8_t	IPaddress[16];						/*!< Socket IP address								*/

	uint16_t port;								/*!< Socket port									*/
}Socket_t;

/**
  * @brief  Network Structure definition
  */
typedef struct __Network_t
{
	NetworkStatus_t status;						/*!< Network status									*/

	uint8_t IPaddress[16];						/*!< Network IP address								*/

}Network_t;

/**
  * @brief  GSM handle Structure definition
  */
typedef struct __gsmHandler_t
{
	DRIVERGsmHandler_t *gsm;					/*!< Gsm handler 									*/

	DRIVERConsoleHandler_t *console;			/*!< Console handler 								*/

	MQTTHandler_t *mqtt;						/*!< Mqtt handler 									*/

	Socket_t socket[MAX_SOCKET_NUMBER + 1];		/*!< Socket structure(zero socket is not in use)	*/

	uint8_t activeSocketNo;						/*!< Number of currently active socket 				*/

	uint8_t numSocketOpen;						/*!< Number of opened socket						*/

	Network_t network;							/*!< Network structure 								*/

}gsmHandler_t;

/**
  * @brief  GSM Configuration Structure definition
  */
typedef struct __gsmConfig_t
{
	DRIVERGsmHandler_t *gsm;					/*!< Gsm handler 									*/

	DRIVERConsoleHandler_t *console;			/*!< Console handler 								*/

	MQTTHandler_t *mqtt;						/*!< Mqtt handler 									*/

}gsmConfig_t;


/**
  * @brief  GSM LIST MESSAGE INPUT Structure definition
  */
typedef struct
{
	char typeOfMsgChar;					/*!< Type of message to list when pdu format of message is setted		*/

	uint8_t typeOfMsgStr[11];			/*!< Type of message to list when text format of message is setted		*/

	uint8_t sizeOfTypeOfMsgStr;			/*!< Size of message to list when text format of message is setted		*/

}ListMsgInputStruct_t;

/**
  * @brief  GSM LIST MESSAGE OUTPUT Structure definition
  */
typedef struct
{
	uint32_t index[20];				/*!< Index's of listed messages														*/

	uint8_t *typeOfMsg[20];			/*!< Type of message (received read,received unread,sotred sent, stored unsent) 	*/

	uint8_t *number[20];			/*!< Telephone number for specified message											*/

	uint8_t *timeReceived[20];		/*!< Time when message was received													*/

	uint8_t *message[20];			/*!< Message																		*/

	uint32_t *msgNoStruct;			/*!< Number of message received 													*/

}ListMsgOutputStruct_t;


/**
  * @brief  GSM READ MESSAGE INPUT Structure definition
  */
typedef struct
{
	uint8_t msgIndex[4];		/*!< Index of message to read	*/

}ReadMsgInputStruct_t;

/**
  * @brief  GSM READ MESSAGE OUTPUT Structure definition
  */
typedef struct
{
	uint8_t *typeOfMsg;			/*!< Type of readed message (received read,received unread,sotred sent, stored unsent)	*/

	uint8_t *number;			/*!< Telephone number for specified message												*/

	uint8_t *timeReceived;		/*!< time when message was received														*/

	uint8_t *message;			/*!< message																			*/

}ReadMsgOutputStruct_t;

/**
  * @brief  GSM DELETE MESSAGE INPUT Structure definition
  */
typedef struct
{
	 uint8_t deleteType;		/*!< Delete certain message or all messages */

	 uint8_t *userRsp;			/*!< Index of message to delete 			*/

}DeleteMsgInputStruct_t;

/**
  * @brief  GSM SEND OR STORE MESSAGE INPUT Structure definition
  */
typedef struct
{
	uint8_t *index;						/*!< Index of message to send 																			*/

	uint8_t sendOrStoreFlag;			/*!< Flag that says to store or to send message															*/

	uint8_t storeOrSendDirectFlag;		/*!< Flag that says, if we choose to send message, to store first and later send or to send directly	*/

	uint8_t *number;					/*!< Number whom to send message 																		*/

	uint8_t *message;					/*!< Message to send or store																			*/

}SendOrStoreInputStruct_t;

/**
  * @brief  GSM SET MESSAGE STORAGE INPUT Structure definition
  */
typedef struct
{
	uint8_t memMsgReadDelate;		/*!< set storage for reading and deleting message (phone memory or SIM card)	*/

	uint8_t memMsgWriteSend;		/*!< set storage for writing and sending message (phone memory or SIM card)		*/

	uint8_t memMsgReceive;			/*!< set storage for receiving message (phone memory or SIM card)				*/

}SetMsgStrgInputStruct_t;

/**
  * @brief  GSM SET PDP CONTEXT INPUT Structure definition
  */
typedef struct
{
	uint8_t *PDPNo;				/*!< PDP context number to set			*/

	uint8_t *PDPTypeFlag;		/*!< PDP type (IP, IPV6, PPP)			*/

	uint8_t *APNType;			/*!< Access oint name to connect with	*/

}SetPDPInputStruct_t;

/**
  * @brief  GSM CONNECT TO SERVER INPUT Structure definition
  */
typedef struct
{
	uint8_t connectType;		/*!< Connection type(TCP,UDP)	*/

	uint8_t *ipAddr;			/*!< IP address of server		*/

	uint8_t *port;				/*!< Opened port of server		*/

}ConnectSrvrInputStruct_t;

/**
  * @brief  GSM UNIVERSAL OUTPUT Structure definition
  */
typedef struct
{
	uint8_t *gsmRsp;		/*!< Universal buffer that contains response from gsm in functions	*/

}OutputStruct_t;

/* Initialization function *******************************************************************************************/
DRIVERState_t GSM_Init(gsmHandler_t *handler, gsmConfig_t *config);

/* IO operation functions ********************************************************************************************/
DRIVERState_t GSM_SetEcho(gsmHandler_t *gsmHandler, uint32_t timeout, GSMEcho_t echoOnOFF, OutputStruct_t *outputStruct);
DRIVERState_t GSM_MsgFormat(gsmHandler_t *gsmHandler, uint32_t timeout, GSMMsgFormat_t format, OutputStruct_t *outputStruct);
DRIVERState_t GSM_SetMsgStorage(gsmHandler_t *gsmHandler, uint32_t timeout,const SetMsgStrgInputStruct_t inputStruct,OutputStruct_t *outputStruct);
DRIVERState_t GSM_TestMsgStorage(gsmHandler_t *gsmHandler, uint32_t timeout);
DRIVERState_t GSM_ListMsg(gsmHandler_t *gsmHandler, uint32_t timeout, const ListMsgInputStruct_t inputStruct, ListMsgOutputStruct_t *outputStruct);
DRIVERState_t GSM_ReadMsg(gsmHandler_t *gsmHandler, uint32_t timeout,const ReadMsgInputStruct_t inputStruct, ReadMsgOutputStruct_t *outputStruct);
DRIVERState_t GSM_DeleteMsg(gsmHandler_t *gsmHandler, uint32_t timeout, DeleteMsgInputStruct_t inputStruct, OutputStruct_t *outputStruct);
DRIVERState_t GSM_SendStoreMsg(gsmHandler_t *gsmHandler, uint32_t timeout,const SendOrStoreInputStruct_t inputStruct,OutputStruct_t *outputStruct);

/* Network operation functions ***************************************************************************************/
DRIVERState_t GSM_NetworkRegistered(gsmHandler_t *gsmHandler);
DRIVERState_t GSM_NetworkDeregistered(gsmHandler_t *gsmHandler);
DRIVERState_t GSM_CheckNetworkRegistered(gsmHandler_t *gsmHandler);

/* PDP - Packet Data Protocol, APN - Access Point Name */
/* Functions for GPRS support */
DRIVERState_t GSM_SetAPN(gsmHandler_t *gsmHandler);
DRIVERState_t GSM_CheckAPN(gsmHandler_t *gsmHandler);
DRIVERState_t GSM_SetWirelessConnectionGPRS(gsmHandler_t *gsmHandler);
DRIVERState_t GSM_GetLocalIPAddress(gsmHandler_t *gsmHandler);
DRIVERState_t GSM_AttachToGPRSService(gsmHandler_t *gsmHandler);
DRIVERState_t GSM_DetachFromGPRSService(gsmHandler_t *gsmHandler);
DRIVERState_t GSM_SetPDPContext(gsmHandler_t *gsmHandler, uint32_t timeout, SetPDPInputStruct_t inputStruct);
DRIVERState_t GSM_CheckSettedPDPContext(gsmHandler_t *gsmHandler);
DRIVERState_t GSM_CheckActivePDPContext(gsmHandler_t *gsmHandler);
DRIVERState_t GSM_ShowPDPIP(gsmHandler_t *gsmHandler);
DRIVERState_t GSM_ActivePDPContext(gsmHandler_t *gsmHandler, uint32_t timeout, const uint8_t *PDP);
DRIVERState_t GSM_DeactiveGPRSPDPContext(gsmHandler_t *gsmHandler);
DRIVERState_t GSM_DeactivePDPContext(gsmHandler_t *gsmHandler, uint32_t timeout, const uint8_t *PDP);
DRIVERState_t GSM_SetAutoSendingTimerIP(gsmHandler_t *gsmHandler, uint32_t timeout, uint8_t status, uint8_t *time);
DRIVERState_t GSM_SetSendingIPFormat(gsmHandler_t *gsmHandler, uint32_t timeout, uint8_t format);

/* Functions for TCPIP support */
DRIVERState_t GSM_ConnectToServer(gsmHandler_t *gsmHandler, uint32_t timeout,ConnectSrvrInputStruct_t inputStruct);
DRIVERState_t GSM_DisconnectFromServer(gsmHandler_t *gsmHandler);
DRIVERState_t GSM_CheckConnection(gsmHandler_t *gsmHandler);
DRIVERState_t GSM_SendToServer(gsmHandler_t *gsmHandler, uint32_t timeout, uint8_t *message);

DRIVERState_t EstablishTCPClientConnection(gsmHandler_t *gsmHandler, uint32_t timeout);

DRIVERState_t onlyPutNumber(DRIVERConsoleHandler_t *console, uint8_t *buffer, uint32_t *size, uint32_t bufSize, uint32_t timeout);

#endif /* MIDDLEWARE_GSM_H_ */
