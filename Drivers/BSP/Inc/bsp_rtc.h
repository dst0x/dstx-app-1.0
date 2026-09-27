/**
 * @file    bsp_rtc.h
 * @brief   Real-time clock board support (LSE 32.768 kHz on PC14/PC15).
 *
 * The RTC is not enabled in the .ioc yet. Enable it in STM32CubeMX
 * (Timers > RTC > Activate Clock Source, clock source LSE); until then every
 * function returns HAL_ERROR.
 */
#ifndef BSP_RTC_H
#define BSP_RTC_H

#include "main.h"
#include <stdint.h>

typedef struct {
  uint16_t year;    /**< 2000..2099 */
  uint8_t  month;   /**< 1..12 */
  uint8_t  day;     /**< 1..31 */
  uint8_t  hour;    /**< 0..23 */
  uint8_t  minute;  /**< 0..59 */
  uint8_t  second;  /**< 0..59 */
} bsp_datetime_t;

HAL_StatusTypeDef bsp_rtc_get_time(bsp_datetime_t *dt);
HAL_StatusTypeDef bsp_rtc_set_time(const bsp_datetime_t *dt);

#endif /* BSP_RTC_H */
