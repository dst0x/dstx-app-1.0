/**
 * @file    bsp_gpio.h
 * @brief   Discrete I/O board support: status LED, relays, digital inputs,
 *          emergency stop.
 *
 * The LED is on PB2 (same as the DST0x_DApps reference firmware) and is set
 * up here. Relays, inputs and the emergency stop are not assigned yet: give
 * the pins these user labels in STM32CubeMX and they are picked up
 * automatically from main.h:
 *   RELAY1, RELAY2        GPIO output, active high
 *   INPUT1, INPUT2        GPIO input
 *   ESTOP                 GPIO input, active low (NC contact to GND)
 */
#ifndef BSP_GPIO_H
#define BSP_GPIO_H

#include "main.h"
#include <stdint.h>

typedef enum {
  BSP_RELAY_1 = 0,
  BSP_RELAY_2,
  BSP_RELAY_COUNT
} BSP_Relay_t;

typedef enum {
  BSP_INPUT_1 = 0,
  BSP_INPUT_2,
  BSP_INPUT_COUNT
} BSP_Input_t;

void bsp_gpio_init(void);

void bsp_led_on(void);
void bsp_led_off(void);
void bsp_led_toggle(void);

/** @retval HAL_ERROR if the relay pin is not assigned in CubeMX */
HAL_StatusTypeDef bsp_relay_set(BSP_Relay_t relay, uint8_t on);

/** @retval 1 if the input is high, 0 if low or not assigned */
uint8_t bsp_input_read(BSP_Input_t input);

/** @retval 1 if the emergency stop is pressed, 0 otherwise or if not assigned */
uint8_t bsp_estop_is_active(void);

#endif /* BSP_GPIO_H */
