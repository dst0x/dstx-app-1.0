/**
 * @file    bsp_gpio.c
 * @brief   Discrete I/O board support.
 */

#include "bsp_gpio.h"

#define LED_PORT  GPIOB
#define LED_PIN   GPIO_PIN_2

void bsp_gpio_init(void)
{
  GPIO_InitTypeDef gpio = {0};

  /* PB2 is not configured in the .ioc, so the LED is set up here */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
  gpio.Pin = LED_PIN;
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_PORT, &gpio);

  /* Relay, input and ESTOP pins are configured by MX_GPIO_Init() once they
     are labelled in CubeMX. Make sure relays start switched off. */
  (void)bsp_relay_set(BSP_RELAY_1, 0U);
  (void)bsp_relay_set(BSP_RELAY_2, 0U);
}

void bsp_led_on(void)
{
  HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

void bsp_led_off(void)
{
  HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
}

void bsp_led_toggle(void)
{
  HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
}

HAL_StatusTypeDef bsp_relay_set(BSP_Relay_t relay, uint8_t on)
{
  GPIO_PinState state = (on != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET;

  switch (relay)
  {
#ifdef RELAY1_Pin
    case BSP_RELAY_1:
      HAL_GPIO_WritePin(RELAY1_GPIO_Port, RELAY1_Pin, state);
      return HAL_OK;
#endif
#ifdef RELAY2_Pin
    case BSP_RELAY_2:
      HAL_GPIO_WritePin(RELAY2_GPIO_Port, RELAY2_Pin, state);
      return HAL_OK;
#endif
    default:
      (void)state;
      return HAL_ERROR;
  }
}

uint8_t bsp_input_read(BSP_Input_t input)
{
  switch (input)
  {
#ifdef INPUT1_Pin
    case BSP_INPUT_1:
      return (HAL_GPIO_ReadPin(INPUT1_GPIO_Port, INPUT1_Pin) == GPIO_PIN_SET) ? 1U : 0U;
#endif
#ifdef INPUT2_Pin
    case BSP_INPUT_2:
      return (HAL_GPIO_ReadPin(INPUT2_GPIO_Port, INPUT2_Pin) == GPIO_PIN_SET) ? 1U : 0U;
#endif
    default:
      return 0U;
  }
}

uint8_t bsp_estop_is_active(void)
{
#ifdef ESTOP_Pin
  return (HAL_GPIO_ReadPin(ESTOP_GPIO_Port, ESTOP_Pin) == GPIO_PIN_RESET) ? 1U : 0U;
#else
  return 0U;
#endif
}
