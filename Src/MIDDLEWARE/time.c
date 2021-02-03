/**
//  ********************************************************************************************
  * @file    time.c
  * @author  Valentina Denic
  * @brief   MIDDLEWARE for waiting response from gsm modul.
  *          This file provides firmware functions to manage the following
  *          functionalities of the gsm modul.
  *
  *
  @verbatim
 ==============================================================================================
                        ##### How to use this driver #####
 ==============================================================================================
  [..]
    The MIDDLEWARE driver can be used as follows:

    (#) Initialize timer for counting up to number 1 with frequency of 32kHz, set time handler
    (#) Get current time
    (#) Delay fuction for waiting in while loop to pass timeout
    (#) Timer callback function for incrementing time variable
  @endverbatim
  *
  *********************************************************************************************
  */

/* Includes ----------------------------------------------------------------------------------*/
#include <time.h>

/* Current global TIME handle */
static volatile TIMEHandler_t *timeHandle;

/* Variable for time that timer use */
volatile uint32_t timeTick;

/**
  * @brief Timer callback function for incrementing variable for time.
  * @param htim      TIMER handle.
  * @retval void
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
	timeHandle->timeTick += 1;
}

/**
  * @brief Initialize timer for time counting.
  * @param handler      TIME handle.
  * @param config       Configuration handle.
  * @retval TIMEState_t status
  */
TIMEState_t TIME_Init(TIMEHandler_t *handler, TIMEConfig_t *config)
{
	/* Check the configuration handle allocation */
	if (config == NULL)
	{
		handler->initState 	= TIME_NO_INIT;
		return TIME_ERROR;
	}

	/* Check the configuration UART initialization */
	(*(config->TimerInit))();

	/* Set handler fields */
	handler->timerBase 	= config->timerBase;

	handler->initState 	= TIME_INIT;

	timeTick 			= 0;

	handler->timeTick	= timeTick;

	timeHandle 			= handler;

	return TIME_OK;

}

/**
  * @brief Get current time.
  * @param void
  * @retval uint32_t time variable
  */
uint32_t TIME_GetTick(void)
{
	return timeHandle->timeTick;
}

/**
  * @brief Delay function.
  * @param timeout		Variable for delaying.
  * @retval TIMEState_t status
  */
TIMEState_t TIME_Delay(uint32_t timeout)
{
	uint32_t tickstart = TIME_GetTick();

	while((TIME_GetTick() - tickstart) < timeout)
	{
		/* Wait to pass timeout */
	}
	return TIME_OK;
}
