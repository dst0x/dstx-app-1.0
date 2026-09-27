/**
 * @file    bsp_iwdg.h
 * @brief   Independent watchdog board support.
 *
 * The IWDG is started by MX_IWDG_Init() (LSI 32 kHz, prescaler 4,
 * reload 4095, so the timeout is about 512 ms). Once started it cannot be
 * stopped, so bsp_iwdg_refresh() must run at least every ~500 ms.
 */
#ifndef BSP_IWDG_H
#define BSP_IWDG_H

#include "main.h"
#include <stdint.h>

/** @brief Captures and clears the reset cause flags. Call once at start-up. */
void bsp_iwdg_init(void);

/** @brief Reloads the watchdog counter. */
void bsp_iwdg_refresh(void);

/** @retval 1 if the last reset was caused by the IWDG */
uint8_t bsp_iwdg_was_reset(void);

#endif /* BSP_IWDG_H */
