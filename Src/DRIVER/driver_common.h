/**
  ******************************************************************************
  * @file    driver_common.h
  * @author  Valentina Denic
  * @brief   This file contains all the functions prototypes for the CONSOLE
  *          module driver in a global usage.
  ******************************************************************************
  */

#ifndef DRIVER_DRIVER_COMMON_H_
#define DRIVER_DRIVER_COMMON_H_

#define BACKSLASH '\r'
#define NEWLINE '\n'
#define QUEUELENGTH 10
#define BACKSPACE 8
#define ESCAPE 27
#define ULONG_MAX 0xFFFFFFFFUL

#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include "stm32h7xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/**
  * @brief  DRIVER Status structures definition
  */
typedef enum
{
  DRIVER_OK       = 0x00,				/*!< Driver return status ok		 */
  DRIVER_ERROR    = 0x01,				/*!< Driver return status error		 */
  DRIVER_TIMEOUT  = 0x02,				/*!< Driver return status timeout	 */
} DRIVERState_t;

/**
  * @brief  UART INIT Status structures definition
  */
typedef enum
{
  UART_INIT		= 0x00,					/*!< Uart initialization status ok		 */
  UART_NOINIT	= 0x01					/*!< Uart initialization status error	 */
} DRIVERUartInit;

/**
  * @brief  DRIVER circular buffer Structure definition
  */
typedef struct __DRIVERCircularBuffer_t
{
	 uint8_t* pointerWrite;				/*!< Pointer for writing new characters to buffer		 */

	 uint8_t* pointerRead;				/*!< Pointer for reading new characters to buffer		 */

	 uint8_t* pointerStart;				/*!< Pointer where buffer starts						 */

	 uint8_t* pointerEnd;				/*!< Pointer where buffer ends							 */

}DRIVERCircularBuffer_t;

#endif /* DRIVER_DRIVER_COMMON_H_ */
