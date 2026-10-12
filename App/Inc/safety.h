/**
 * @file    safety.h
 * @brief   Safety supervision: clock security system (CSS), power voltage
 *          detector (PVD), watchdog and emergency stop.
 *
 * CSS: if the HSE crystal fails, the hardware switches SYSCLK to the 16 MHz
 * HSI and raises an NMI; HAL_RCC_CSSCallback() records the fault. The clock
 * is then 6x slower, so UART baud rates are wrong until the next reset.
 */
#ifndef SAFETY_H
#define SAFETY_H

#include <stdint.h>

#define SAFETY_FAULT_NONE        (0x00U)
#define SAFETY_FAULT_CSS         (0x01U)  /**< HSE failure, running on HSI */
#define SAFETY_FAULT_PVD         (0x02U)  /**< Supply below SAFETY_PVD_LEVEL */
#define SAFETY_FAULT_ESTOP       (0x04U)  /**< Emergency stop pressed */
#define SAFETY_FAULT_IWDG_RESET  (0x08U)  /**< Last reset was caused by the watchdog */

void safety_init(void);
void safety_service(void);
void safety_clear_faults(void);

uint8_t safety_get_faults(void);
uint8_t safety_is_tripped(void);

#endif /* SAFETY_H */
