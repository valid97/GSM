/**
  *********************************************************************************************************
  * @file    gsm.h
  * @author  Valentina Denic
  * @brief   This file contains all the functions prototypes for the GSM
  *          module driver.
  *********************************************************************************************************
  */
#ifndef DRIVER_GSM_GSM_H_
#define DRIVER_GSM_GSM_H_

#include <driver_common.h>


/**
  * @brief  GSM INIT Status structures definition
  */
typedef enum
{
  GSM_INIT		= 0x00,					/*!< Time initialization status ok  					 */
  GSM_NOINIT	= 0x01					/*!< Time initialization status error  					 */
} DRIVERGsmInit;

/**
  * @brief  GSM STATE Status structures definition
  */
typedef enum
{
	GSM_STATE_IDLE      = 0x00,			/*!< Current process state idle		 					 */
	GSM_STATE_TRANSMIT  = 0x01,			/*!< Current process state transmit	 					 */
	GSM_STATE_RECEIVE   = 0x02			/*!< Current process state receive	 					 */
} DRIVERGsmState;

/**
  * @brief  DRIVER handle GSM Structure definition
  */
typedef struct __DRIVERGsmHandler_t
{
	volatile uint8_t* rxBuffer;					/*!< Receive registers base address    					 */

	uint16_t rxSize;							/*!< Receive registers size		    					 */

	DRIVERGsmInit InitState;					/*!< Inititial state parameter	     					 */

	DRIVERGsmState State;						/*!< Running communication parameters   			 	 */

	QueueHandle_t GsmQueueTransmit;				/*!< Queue for transmitting messages to UART  			 */

	QueueHandle_t GsmQueueReceive;				/*!< Queue for receiving messages from UART  			 */

	UART_HandleTypeDef* uartBase;				/*!< UART handle 						   			 	 */


}DRIVERGsmHandler_t;

/**
  * @brief  DRIVER GSM configuration Structure definition
  */
typedef struct __DRIVERGsmConfig_t
{
	uint8_t* rxBuffer;					/*!< Receive registers base address    					 */

	uint16_t rxSize;					/*!< Receive registers size		    					 */

	UART_HandleTypeDef* uartBase;		/*!< UART handle 						   			 	 */

	DRIVERState_t (*UartInit)(void);	/*!< Function pointer on UartInit to initialize UART     */

}DRIVERGsmConfig_t;

/**
 *  @brief  DRIVER Message passing structure definiton
 *  */
typedef struct
{
    uint8_t *startMsg;
    uint32_t sizeMsg;
}DRIVERGsmMsg_t;

/* Initialization operation functions ***********************************************************************/
DRIVERState_t DRIVER_GSM_Init(DRIVERGsmHandler_t *handler, DRIVERGsmConfig_t *config);

/* IO operation functions ***********************************************************************************/
DRIVERState_t DRIVER_GSM_Read(DRIVERGsmHandler_t *handler, uint8_t* userBuffer, uint32_t* size);
DRIVERState_t DRIVER_GSM_Write(DRIVERGsmHandler_t *handler, const uint8_t* msg, uint32_t msgSize);
DRIVERState_t DRIVER_GSM_Flush(DRIVERGsmHandler_t *handler);

#endif /* DRIVER_GSM_GSM_H_ */
