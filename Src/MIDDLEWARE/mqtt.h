/**
  ***************************************************************************************************
  * @file    mqtt.h
  * @author  Valentina Denic
  * @brief   This file contains all the functions prototypes for the MQTT protocol. It requires
  * the existence of a TCP IP protocol implementation and modul which communicate using AT command
  * set.
  *
  ***************************************************************************************************
  */

#ifndef MIDDLEWARE_MQTT_H_
#define MIDDLEWARE_MQTT_H_

/* Includes -----------------------------------------------------------------------------------------------*/
#include <driver_console.h>
#include <driver_common.h>
#include <driver_gsm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define M_HEX 0x4d
#define Q_HEX 0x51
#define T_HEX 0x54
#define MQTT_VERSION 0x04
#define MQQT_PROTOCOL_NAME(M_HEX,Q_HEX,T_HEX) (0x000000 |= ((M_HEX) << 6) & ((Q_HEX) << 4) & ((T_HEX) << 2) & (T_HEX))
#define CTRLFLAGPUB(DUP,QOS,RETAIN) (0b0000 |= ((DUP) << 3) & ((QOS) << 1) & (RETAIN))

/**
  * @brief  MQTT Status structures definition
  */
typedef enum
{
	MQTT_OK     	= 0x00,				/*!< MQTT status ok			*/
	MQTT_TIMEOUT   	= 0x01,				/*!< MQTT status timeout	*/
	MQTT_ERROR	    = 0x02				/*!< MQTT status error		*/
} MQTTState_t;


/**
  * @brief  MQTT INIT Status structures definition
  */
typedef enum
{
  MQTT_INIT		= 0x00,					/*!< MQTT initialization status initialization ok		 */
  MQTT_NO_INIT	= 0x01					/*!< MQTT initialization status no initialization 		 */
} MQTTInit_t;

/**
  * @brief  MQTT PACKET COMMAND TYPE Status structures definition
  */
typedef enum
{
	CONNECT      = 0b0001,			/*!< Command type connect			 		 				  	 */
	CONNACK      = 0b0010,			/*!< Command type connect acknowledgment	 					 */
	PUBLISH      = 0b0011,			/*!< Command type publish message			 					 */
	PUBACK       = 0b0100,			/*!< Command type publish acknowledgment						 */
	PUBREC       = 0b0101,			/*!< Command type publish received (assured delivery part 1)	 */
	PUBREL       = 0b0110,			/*!< Command type publish release (assured delivery part 2)		 */
	PUBCOMP      = 0b0111,			/*!< Command type publish complete (assured delivery part 3)	 */
	SUBSCRIBE    = 0b1000,			/*!< Command type client subscribe request					  	 */
	SUBACK       = 0b1001,			/*!< Command type subscribe acknowledgment			 		  	 */
	UNSUBSCRIBE  = 0b1010,			/*!< Command type unsubscribe request	 						 */
	UNSUBACK     = 0b1011,			/*!< Command type Unsubscribe acknowledgment			 		 */
	PINGREQ      = 0b1100,			/*!< Command type PING request									 */
	PINGRESP     = 0b1101,			/*!< Command type PING response									 */
	DISCONNECT   = 0b1110			/*!< Command type Client is disconnecting		 				 */
} MQTTCommand_t;


/**
  * @brief  MQTT PACKET CONTROL FLAG Status structures definition
  */
typedef enum
{
	CONTROL_FLAG_CONNECT      = 0b0000,			/*!< Control flag for connect command type			 		 				  	 */
	CONTROL_FLAG_CONNACK      = 0b0000,			/*!< Control flag for connect acknowledgment command type	 					 */
	CONTROL_FLAG_PUBLISH      = 0b0011,			/*!< Control flag for publish message command type			 					 */
	CONTROL_FLAG_PUBACK       = 0b0000,			/*!< Control flag for publish acknowledgment command type						 */
	CONTROL_FLAG_PUBREC       = 0b0000,			/*!< Control flag for publish received (assured delivery part 1) command type	 */
	CONTROL_FLAG_PUBCOMP      = 0b0000,			/*!< Control flag for publish complete (assured delivery part 3) command type	 */
	CONTROL_FLAG_SUBSCRIBE    = 0b0010,			/*!< Control flag for client subscribe request command type					  	 */
	CONTROL_FLAG_SUBACK       = 0b0000,			/*!< Control flag for subscribe acknowledgment command type			 		  	 */
	CONTROL_FLAG_UNSUBSCRIBE  = 0b0010,			/*!< Control flag for unsubscribe request command type	 						 */
	CONTROL_FLAG_UNSUBACK     = 0b0000,			/*!< Control flag for unsubscribe acknowledgment command type			 		 */
	CONTROL_FLAG_PINGREQ      = 0b0000,			/*!< Control flag for PING request command type									 */
	CONTROL_FLAG_PINGRESP     = 0b0000,			/*!< Control flag for PING response command type								 */
	CONTROL_FLAG_DISCONNECT   = 0b0000			/*!< Control flag for Client is disconnecting command type		 				 */
} MQTTCtrlFlag_t;

/**
  * @brief  MQTT PACKET CONTROL FLAG  FOR PUBLISH COMMAND Status structures definition
  */
typedef enum
{
	DUP_FIRST_OCCASION  	 = 0b0,			/*!< First occasion that Client or Server has attempted to send this MQTT PUBLISH Packet 													*/
	DUP_NEXT_OCCASION  	 	 = 0b1,			/*!< Client or Server attempts to re-deliver a PUBLISH Packet 	 																			*/
	QoS_0     			 	 = 0b00,		/*!< Level of assurance for delivery of an App. Message - at most once delivery																*/
	QoS_1      			 	 = 0b01,		/*!< Level of assurance for delivery of an App. Message - at least once delivery															*/
	QoS_2      			 	 = 0b10,		/*!< Level of assurance for delivery of an App. Message - exactly once delivery																*/
	RETAIN 				 	 = 0b1,			/*!< Control flag PUBLISH Retain flag - Server MUST store App. Message and its QoS	    													*/
	NO_RETAIN	  			 = 0b0			/*!< Control flag PUBLISH Retain flag - Server MUST NOT store the message and MUST NOT remove or replace any existing retained message    	*/
} MQTTCtrlFlagPUBLISH_t;

/***  BITS OF CONNECT FLAGS   ***/

/**
  * @brief  MQTT USER NAME FLAG Status structures definition
  */
typedef enum
{
	USER_NAME_NO_SET     = 0b0,			/*!< User name MUST NOT be present in the payload 	*/
	USER_NAME_SET      	 = 0b1			/*!< User name MUST be present in the payload 		*/
} MQTTUserNameFlag_t;

/**
  * @brief  MQTT PASSWORD FLAG Status structures definition
  *
  *  If User Name Flag is 0, the Password Flag MUST be set to 0
  */
typedef enum
{
	PASSWORD_NO_SET      = 0b0,			/*!< Password MUST NOT be present in the payload.	*/
	PASSWORD_SET      	 = 0b1			/*!< Password MUST be present in the payload		*/
} MQTTPasswordFlag_t;

/**
  * @brief  MQTT WILL RETAIN FLAG Status structures definition
  *
  *  If Will Retain is set to 0, the Server MUST publish the Will Message as a non-retained message.
  *  If Will Retain is set to 1, the Server MUST publish the Will Message as a retained message.
  *  If the Will Flag is set to 0, then the Will Retain Flag MUST be set to 0.
  */
typedef enum
{
	WILL_RETAIN_NO_SET   = 0b0,			/*!< Retain Flag set to zero - publish message as non-retained message		*/
	WILL_RETAIN_SET      = 0b1			/*!< Retain Flag set to one - publish message as retained message			*/
} MQTTWillRetainFlag_t;

/**
  * @brief  MQTT WILL QOS FLAG Status structures definition
  *
  * If the Will Flag is set to 0, then the Will QoS MUST be set to 0 (0x00).
  * If the Will Flag is set to 1, the value of Will QoS can be 0 (0x00), 1 (0x01),
  * or 2 (0x02). It MUST NOT be 3(0x03).
  */
typedef enum
{
	WILL_QOS_ZERO   = 0b00,			/*!< Level of assurance - at most once delivery		*/
	WILL_QOS_ONE    = 0b01,			/*!< Level of assurance - at least once delivery	*/
	WILL_QOS_TWO    = 0b10			/*!< Level of assurance - exactly once delivery 	*/
} MQTTWillQoSFlag_t;

/**
  * @brief  MQTT WILL FLAG Status structures definition
  *
  * If the Will Flag is set to 0 the Will QoS and Will Retain fields in the Connect Flags MUST be set to zero
  * and the Will Topic and Will Message fields MUST NOT be present in the payload.If the Will Flag is set to 1,
  * the Will QoS and Will Retain fields in the Connect Flags will be used by the
  * Server, and the Will Topic and Will Message fields MUST be present in the payload
  *
  */
typedef enum
{
	WILL_NO_SET   = 0b0,		/*!< Will Message MUST be stored on the Server	and MUST be published when Network Connection is subsequently closed	*/
	WILL_SET      = 0b1			/*!< Will Message MUST NOT be published when this Network Connection ends 												*/
} MQTTWillFlag_t;

/**
  * @brief  MQTT WILL FLAG Status structures definition
  *
  */
typedef enum
{
	CLEAN_SESSION_NO_SET   = 0b0,		/*!< Server MUST resume communications with Client based on state from the current Session (as identified by the Client identifier)		*/
	CLEAN_SESSION_SET      = 0b1		/*!< Client and Server MUST discard any previous Session and start a new one															*/
} MQTTCleanSessionFlag_t;


/**
  * @brief  MQTT PROTOCOL NAME LENGTH Structure definition
  */
typedef struct __MQTTLen_t
{
	int	MSBByte;
	int LSBByte;
} MQTTLen_t;

/**
  * @brief  MQTT CONNECT FLAG BYTE Structure definition
  */
typedef struct __MQTTConnectFlagByte_t
{
	MQTTUserNameFlag_t		userNameFlag;
	MQTTPasswordFlag_t		passwordFlag;
	MQTTWillRetainFlag_t	willRetainFlag;
	MQTTWillQoSFlag_t		willQoSFlag;
	MQTTWillFlag_t			willFlag;
	MQTTCleanSessionFlag_t	cleanSessionFlag;
} MQTTConnectFlagByte_t;

/**
  * @brief  MQTT KEEP ALIVE Structure definition
  */
typedef struct __MQTTKeepAlive_t
{
	int	MSBByte;
	int LSBByte;
} MQTTKeepAlive_t;

/**
  * @brief  MQTT VARIABLE HEADER Structure definition
  */
typedef struct __MQTTVariableHeader_t
{
	/* Fields for CONNECT command type 	 */
	MQTTLen_t 				protocolNameLen;
	int 					protocolName;
	int 					protocolLevel;
	MQTTConnectFlagByte_t 	connectFlagByte;
	MQTTKeepAlive_t 		keepAlive;

	/* Fields for PUBLISH command type	 */
	MQTTLen_t 				topicNameLen;
	uint32_t				topicName;

	/* Fields for SUBSCRIBE command type */
	uint32_t				packetID;

} MQTTVariableHeader_t;

/**
  * @brief  MQTT PAYLOAD Structure definition
  */
typedef struct __MQTTPayload_t
{
	/* Fields for CONNECT command type 	 */
	MQTTLen_t	clientIDLen;
	uint8_t		*clientID;
	MQTTLen_t 	userNameLen;
	uint8_t		*userName;
	MQTTLen_t 	passwordLen;
	uint8_t		*password;

	/* Fields for PUBLISH command type	 */
	MQTTLen_t	messageLen;
	uint32_t	messageName;

	/* Fields for SUBSCRIBE command type */
	uint32_t	topicLen;
	uint8_t		topicName[2000];

} MQTTPayload_t;


/**
  * @brief  MQTT PACKET Structure definition
  */
typedef struct __MQTTPacket_t
{
	MQTTCommand_t 			commandType;
	MQTTCtrlFlag_t			controlFlag;
	int						remainingLength;
	MQTTVariableHeader_t 	variableHeader;
	MQTTPayload_t			payload;
} MQTTPacket_t;

/**
  * @brief  MQTT HANDLE Console Structure definition
  */
typedef struct __MQTTHandler_t
{
	DRIVERGsmHandler_t 		*gsmHandler;
	DRIVERConsoleHandler_t 	*consoleHandler;
	MQTTPacket_t 			mqttPacket;
	MQTTInit_t 				initState;

} MQTTHandler_t;

/**
  * @brief  MQTT CONFIGURATION Structure definition
  */
typedef struct __MQTTConfig_t
{
	DRIVERGsmHandler_t 		*gsmHandler;
	DRIVERConsoleHandler_t 	*consoleHandler;

} MQTTConfig_t;

/* Initialization operation functions ****************************************************************/
MQTTState_t MQTT_Init(MQTTHandler_t *handler, MQTTConfig_t *config);

MQTTState_t MQTT_Connect(MQTTHandler_t *handler);
MQTTState_t MQTT_Disconnect(MQTTHandler_t *handler);
MQTTState_t MQTT_Subscribe(MQTTHandler_t *handler, uint32_t timeout);
MQTTState_t MQTT_Publish(MQTTHandler_t *handler, uint32_t timeout);
MQTTState_t MQTT_Subscribe(MQTTHandler_t *handler, uint32_t timeout);
MQTTState_t MQTT_Unsubscribe(MQTTHandler_t *handler, uint32_t timeout);
MQTTState_t MQTT_PingReq(MQTTHandler_t *handler, uint32_t timeout);

#endif /* MIDDLEWARE_MQTT_H_ */
