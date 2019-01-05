#include "stm32f1xx_hal.h"

RTC_HandleTypeDef hrtc;
void RTC_Error_Handler(void);

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
void RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */
  __HAL_RCC_RTC_ENABLE();
  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef DateToUpdate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */
  /**Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
  hrtc.Init.OutPut = RTC_OUTPUTSOURCE_ALARM;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    RTC_Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /**Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;

  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    RTC_Error_Handler();
  }
  DateToUpdate.WeekDay = RTC_WEEKDAY_MONDAY;
  DateToUpdate.Month = RTC_MONTH_JANUARY;
  DateToUpdate.Date = 0x1;
  DateToUpdate.Year = 0x0;

  if (HAL_RTC_SetDate(&hrtc, &DateToUpdate, RTC_FORMAT_BCD) != HAL_OK)
  {
    RTC_Error_Handler();
  }

  /* USER CODE BEGIN RTC_Init 2 */
  HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 0xf, 0U);
  HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
  /* USER CODE END RTC_Init 2 */
}

void RTCAlarm_IRQHandler(void){
    HAL_RTC_AlarmIRQHandler(&hrtc);
}


/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void RTC_Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}
