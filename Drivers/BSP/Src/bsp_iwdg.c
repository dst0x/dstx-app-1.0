/**
 * @file    bsp_iwdg.c
 * @brief   Independent watchdog board support.
 */

#include "bsp_iwdg.h"

extern IWDG_HandleTypeDef hiwdg;

static uint8_t s_reset_by_iwdg = 0U;

void bsp_iwdg_init(void)
{
  s_reset_by_iwdg = (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != 0U) ? 1U : 0U;
  __HAL_RCC_CLEAR_RESET_FLAGS();
}

void bsp_iwdg_refresh(void)
{
  (void)HAL_IWDG_Refresh(&hiwdg);
}

uint8_t bsp_iwdg_was_reset(void)
{
  return s_reset_by_iwdg;
}
