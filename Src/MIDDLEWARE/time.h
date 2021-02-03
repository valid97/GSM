/**
  ***************************************************************************************************
  * @file    time.h
  * @author  Valentina Denic
  * @brief   This file contains all the functions prototypes for the TIME
  *          operations.
  ***************************************************************************************************
  */

#ifndef MIDDLEWARE_TIME_H_
#define MIDDLEWARE_TIME_H_

#include <driver_common.h>


/**
  * @brief  TIME INIT Status structures definition
  */
typedef enum
{
  TIME_INIT		= 0x00,					/*!< Time initialization status initialization ok		 */
  TIME_NO_INIT	= 0x01					/*!< Time initialization status no initialization		 */
} TIMEInit_t;

/**
  * @brief  TIME Status structures definition
  */
typedef enum
{
	TIME_OK     	= 0x00,				/*!< Time status ok			 */
	TIME_TIMEOUT    = 0x01,				/*!< Time status timeout	 */
	TIME_ERROR	    = 0x02				/*!< Time status error		 */
} TIMEState_t;

/**
  * @brief  TIME handle Structure definition
  */
typedef struct __TIMEHandler_t
{
	TIMEInit_t initState;					/*!< Initial state parameter	 */

	TIM_HandleTypeDef* timerBase;			/*!< TIMER handle 				 */

	volatile uint32_t timeTick;		    	/*!< Current time				 */

}TIMEHandler_t;

/**
  * @brief  TIME configuration Structure definition
  */
typedef struct __TIMEConfig_t
{
	TIM_HandleTypeDef* timerBase;		/*!< TIMER handle 						   			 	  */

	void (*TimerInit)(void);			/*!< Function pointer on TimerInit to initialize TIMER    */

}TIMEConfig_t;


/* Initialization operation functions ****************************************************************/
TIMEState_t TIME_Init(TIMEHandler_t *handler, TIMEConfig_t *config);

/* IO operation functions ****************************************************************************/
uint32_t TIME_GetTick(void);
TIMEState_t TIME_Delay(uint32_t timeout);


#endif /* MIDDLEWARE_TIME_H_ */
