/**
 * @file    bsp_rtc.c
 * @brief   Real-time clock board support.
 */

#include "bsp_rtc.h"

#ifdef HAL_RTC_MODULE_ENABLED

extern RTC_HandleTypeDef hrtc;

HAL_StatusTypeDef bsp_rtc_get_time(bsp_datetime_t *dt)
{
  RTC_TimeTypeDef time;
  RTC_DateTypeDef date;

  /* GetTime must be called before GetDate to unlock the shadow registers */
  if ((HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN) != HAL_OK) ||
      (HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN) != HAL_OK))
  {
    return HAL_ERROR;
  }
  dt->year = (uint16_t)(2000U + date.Year);
  dt->month = date.Month;
  dt->day = date.Date;
  dt->hour = time.Hours;
  dt->minute = time.Minutes;
  dt->second = time.Seconds;
  return HAL_OK;
}

HAL_StatusTypeDef bsp_rtc_set_time(const bsp_datetime_t *dt)
{
  RTC_TimeTypeDef time = {0};
  RTC_DateTypeDef date = {0};

  if ((dt->year < 2000U) || (dt->year > 2099U))
  {
    return HAL_ERROR;
  }
  time.Hours = dt->hour;
  time.Minutes = dt->minute;
  time.Seconds = dt->second;
  date.Year = (uint8_t)(dt->year - 2000U);
  date.Month = dt->month;
  date.Date = dt->day;
  date.WeekDay = RTC_WEEKDAY_MONDAY; /* Not used by the application */

  if ((HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN) != HAL_OK) ||
      (HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN) != HAL_OK))
  {
    return HAL_ERROR;
  }
  return HAL_OK;
}

#else /* RTC not enabled in CubeMX */

HAL_StatusTypeDef bsp_rtc_get_time(bsp_datetime_t *dt)
{
  (void)dt;
  return HAL_ERROR;
}

HAL_StatusTypeDef bsp_rtc_set_time(const bsp_datetime_t *dt)
{
  (void)dt;
  return HAL_ERROR;
}

#endif /* HAL_RTC_MODULE_ENABLED */
