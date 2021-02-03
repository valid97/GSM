/**
  ***********************************************************************************************************
  * @file    gsm.h
  * @author  Valentina Denic
  * @brief   This file contains all the functions prototypes for middleware
  *          layer for comunication between gsm modul and other parts of project(console, network conecetion).
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

}gsmConfig_t;

/* Initialization function *******************************************************************************************/
DRIVERState_t GSM_Init(gsmHandler_t *handler, gsmConfig_t *config);

/* IO operation functions ********************************************************************************************/
DRIVERState_t GSM_SetEcho(gsmHandler_t *gsmHandler, uint32_t timeout);
DRIVERState_t GSM_MsgFormat(gsmHandler_t *gsmHandler, uint32_t timeout);
DRIVERState_t GSM_SetMsgStorage(gsmHandler_t *gsmHandler, uint32_t timeout);
DRIVERState_t GSM_TestMsgStorage(gsmHandler_t *gsmHandler, uint32_t timeout);
DRIVERState_t GSM_ListMsg(gsmHandler_t *gsmHandler, uint32_t timeout);
DRIVERState_t GSM_ReadMsg(gsmHandler_t *gsmHandler, uint32_t timeout);
DRIVERState_t GSM_DeleteMsg(gsmHandler_t *gsmHandler, uint32_t timeout);
DRIVERState_t GSM_SendStoreMsg(gsmHandler_t *gsmHandler, uint32_t timeout);

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
DRIVERState_t GSM_SetPDPContext(gsmHandler_t *gsmHandler, uint32_t timeout);
DRIVERState_t GSM_CheckSettedPDPContext(gsmHandler_t *gsmHandler);
DRIVERState_t GSM_CheckActivePDPContext(gsmHandler_t *gsmHandler);
DRIVERState_t GSM_ShowPDPIP(gsmHandler_t *gsmHandler);
DRIVERState_t GSM_ActivePDPContext(gsmHandler_t *gsmHandler, uint32_t timeout);
DRIVERState_t GSM_DeactiveGPRSPDPContext(gsmHandler_t *gsmHandler);
DRIVERState_t GSM_DeactivePDPContext(gsmHandler_t *gsmHandler, uint32_t timeout);
DRIVERState_t GSM_SetAutoSendingTimerIP(gsmHandler_t *gsmHandler, uint32_t timeout);

/* Functions for TCPIP support */
DRIVERState_t GSM_ConnectToServer(gsmHandler_t *gsmHandler, uint32_t timeout);
DRIVERState_t GSM_DisconnectFromServer(gsmHandler_t *gsmHandler);
DRIVERState_t GSM_CheckConnection(gsmHandler_t *gsmHandler);
DRIVERState_t GSM_SendToServer(gsmHandler_t *gsmHandler, uint32_t timeout);

DRIVERState_t EstablishTCPClientConnection(gsmHandler_t *gsmHandler, uint32_t timeout);

#endif /* MIDDLEWARE_GSM_H_ */
