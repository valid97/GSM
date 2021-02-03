/**
  ******************************************************************************************************************************
  * @file    driver_console.h
  * @author  Valentina Denic
  * @brief   This file contains all the functions prototypes for the CONSOLE
  *          module driver.
  ******************************************************************************************************************************
  */

#ifndef DRIVER_CONSOLE_CONSOLE_H_
#define DRIVER_CONSOLE_CONSOLE_H_

#include <driver_common.h>
#include <time.h>

/**
  * @brief  CONSOLE INIT Status structures definition
  */
typedef enum
{
  CONSOLE_INIT		= 0x00,						/*!< Console initialization status ok  					 */
  CONSOLE_NOINIT	= 0x01						/*!< Console initialization status error				 */
} DRIVERConsoleInit;

/**
  * @brief  CONSOLE STATE Status structures definition
  */
typedef enum
{
  COSNOLE_STATE_IDLE      = 0x00,				/*!< Current process state idle		 					 */
  COSNOLE_STATE_TRANSMIT  = 0x01,				/*!< Current process state transmit	 					 */
  COSNOLE_STATE_RECEIVE   = 0x02				/*!< Current process state receive	 					 */
} DRIVERConsoleState;

/**
  * @brief  DRIVER handle Console Structure definition
  */
typedef struct __DRIVERConsoleHandler_t
{
	uint8_t* txBuffer;							/*!< Transmit registers base address    				 */

	uint32_t txSize;							/*!< Transmit registers size        					 */

	uint8_t* rxBuffer;							/*!< Receive registers base address    					 */

	uint32_t rxSize;							/*!< Receive registers size		       					 */

	QueueHandle_t ConsoleQueueTransmit;			/*!< Queue for transmitting messages to UART  			 */

	QueueHandle_t ConsoleQueueReceive;			/*!< Queue for receiving messages from UART  			 */

	DRIVERConsoleInit InitState;				/*!< Init communication parameters     					 */

	DRIVERConsoleState State;					/*!< Running communication parameters   			 	 */

	UART_HandleTypeDef* uartBase;				/*!< UART handle 						   			 	 */

}DRIVERConsoleHandler_t;

/**
  * @brief  DRIVER configuration Structure definition
  */
typedef struct __DRIVERConsoleConfig_t
{
	uint8_t* txBuffer;							/*!< Transmit registers base address    			 	 */

	uint8_t* rxBuffer;							/*!< Receive registers base address    					 */

	uint32_t txSize;							/*!< Configuration transmit registers size				 */

	uint32_t rxSize;							/*!< Configuration receive registers size				 */

	void (*WriteChar)(uint8_t Char);			/*!< Function pointer on WriteChar handler      		 */

	uint8_t (*ReadChar)(uint32_t timeout);		/*!< Function pointer on ReadChar handler       	 	 */

	UART_HandleTypeDef* uartBase;				/*!< UART handle 						   			 	 */

	uint8_t (*UartInit)(void);					/*!< Function pointer on UartInit to initialize UART     */

}DRIVERConsoleConfig_t;

/**
 *  @brief  DRIVER Message passing structure definiton
 *  */
typedef struct
{
    uint8_t *startMsg;
    uint32_t sizeMsg;
}DRIVERConsoleMsg_t;

/* Initialization operation functions ******************************************************************************************/
DRIVERState_t DRIVER_CONSOLE_Init(DRIVERConsoleHandler_t *handler, DRIVERConsoleConfig_t *config);

/* IO operation functions ******************************************************************************************************/
DRIVERState_t DRIVER_CONSOLE_Get(DRIVERConsoleHandler_t *handler, uint8_t *userBuffer, uint32_t* dataSize, uint32_t timeout);
DRIVERState_t DRIVER_CONSOLE_Put(DRIVERConsoleHandler_t *handler, const uint8_t *string);

#endif /* DRIVER_CONSOLE_CONSOLE_H_ */
