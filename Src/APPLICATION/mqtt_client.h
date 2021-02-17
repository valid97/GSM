/*
 * mqtt_client.h
 *
 *  Created on: Feb 8, 2021
 *      Author: valentinad
 */

#ifndef APPLICATION_MQTT_CLIENT_H_
#define APPLICATION_MQTT_CLIENT_H_

#define	MQTT_CLIENT_NO_BLOCK 		 (uint32_t) 0x00000000U
#define	MQTT_CLIENT_BLOCK_INFINITY   (uint32_t) 0xFFFFFFFFU

#include <driver_console.h>
#include <driver_common.h>
#include <driver_gsm.h>
#include <mqtt.h>

#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include "stm32h7xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"


/**
  * @brief  MQTT CLIENT State Status structures definition
  */
typedef enum
{
	MQTT_CLIENT_LISTEN       = 0x00,				/*!< Mqtt client listen state		 */
	MQTT_CLIENT_CLOSE	     = 0x01 				/*!< Mqtt client close  state		 */
} MQTTClientState_t;


/**
  * @brief  MQTT CLIENT Status structures definition
  */
typedef enum
{
	MQTT_CLIENT_OK     		= 0x00,				/*!< Mqtt client ok type		 */
	MQTT_CLIENT_ERROR	    = 0x01				/*!< Mqtt client error type		 */
} MQTTClientType_t;

/**
 *  @brief  MQTT CLIENT Message passing structure definiton
 *  */
typedef struct
{
	MQTTClientState_t state;
}MQTTClientMsg_t;

/**
  * @brief  MQTT CLIENT Handle Structure definition
  */
typedef struct __MQTTClientHandler_t
{
	DRIVERGsmHandler_t *gsm;					/*!< Gsm handler 														*/

	DRIVERConsoleHandler_t *console;			/*!< Console handler 													*/

	MQTTHandler_t *mqtt;						/*!< Mqtt handler 														*/

	MQTTClientState_t state;					/*!< Running communication parameters   			 	 				*/

	QueueHandle_t mqttClientQueue;				/*!< Queue for receiving command(listen or close) from responsibe task	*/
}MQTTClientHandler_t;

/**
  * @brief  MQTT CLIENT configuration Structure definition
  */
typedef struct __MQTTClientConfig_t
{
	DRIVERGsmHandler_t *gsm;					/*!< Gsm handler 									*/

	DRIVERConsoleHandler_t *console;			/*!< Console handler 								*/

	MQTTHandler_t *mqtt;						/*!< Mqtt handler 									*/
}MQTTClientConfig_t;

MQTTClientType_t MQTT_CLIENT_Init(MQTTClientHandler_t *handler, MQTTClientConfig_t *config);

MQTTClientType_t MQTT_CLIENT_SetState(MQTTClientHandler_t *handler, MQTTClientState_t state);

#endif /* APPLICATION_MQTT_CLIENT_H_ */
