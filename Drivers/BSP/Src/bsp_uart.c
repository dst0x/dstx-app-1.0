/**
 * @file    bsp_uart.c
 * @brief   Debug UART (USART2) and RS-485 (USART1 + DMA) board support.
 */

#include "bsp_uart.h"
#include "tx_api.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define DEBUG_PRINTF_BUF_SIZE  (256U)
#define DEBUG_TX_TIMEOUT_MS    (1000U)
#define MS_TO_TICKS(ms)        ((((ULONG)(ms) * TX_TIMER_TICKS_PER_SECOND) + 999U) / 1000U)

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

static TX_SEMAPHORE s_debug_tx_done;
static TX_SEMAPHORE s_rs485_tx_done;
static TX_SEMAPHORE s_rs485_rx_done;
static TX_MUTEX     s_debug_lock;
static volatile uint16_t s_rs485_rx_len;

static void rs485_set_transmit(uint8_t enable)
{
#ifdef RS485_DE_Pin
  HAL_GPIO_WritePin(RS485_DE_GPIO_Port, RS485_DE_Pin, (enable != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
#else
  (void)enable; /* No DE/RE pin labelled in CubeMX: transceiver with auto direction */
#endif
}

HAL_StatusTypeDef bsp_uart_init(void)
{
  if ((tx_semaphore_create(&s_debug_tx_done, "dbg tx", 0) != TX_SUCCESS) ||
      (tx_semaphore_create(&s_rs485_tx_done, "485 tx", 0) != TX_SUCCESS) ||
      (tx_semaphore_create(&s_rs485_rx_done, "485 rx", 0) != TX_SUCCESS) ||
      (tx_mutex_create(&s_debug_lock, "dbg lock", TX_INHERIT) != TX_SUCCESS))
  {
    return HAL_ERROR;
  }
  rs485_set_transmit(0U);
  return HAL_OK;
}

void bsp_uart_debug_print(const char *str)
{
  size_t len = strlen(str);

  tx_mutex_get(&s_debug_lock, TX_WAIT_FOREVER);
  while (len > 0U)
  {
    uint16_t chunk = (len > 0xFFFFU) ? 0xFFFFU : (uint16_t)len;

    if (HAL_UART_Transmit_IT(&huart2, (const uint8_t *)str, chunk) != HAL_OK)
    {
      break;
    }
    /* Released by HAL_UART_TxCpltCallback */
    tx_semaphore_get(&s_debug_tx_done, MS_TO_TICKS(DEBUG_TX_TIMEOUT_MS));
    str += chunk;
    len -= chunk;
  }
  tx_mutex_put(&s_debug_lock);
}

void bsp_uart_debug_printf(const char *fmt, ...)
{
  static char buf[DEBUG_PRINTF_BUF_SIZE];
  va_list args;
  size_t len;

  tx_mutex_get(&s_debug_lock, TX_WAIT_FOREVER);
  va_start(args, fmt);
  (void)vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  len = strlen(buf);
  while (len > 0U)
  {
    uint16_t chunk = (len > 0xFFFFU) ? 0xFFFFU : (uint16_t)len;

    if (HAL_UART_Transmit_IT(&huart2, (const uint8_t *)buf, chunk) != HAL_OK)
    {
      break;
    }
    tx_semaphore_get(&s_debug_tx_done, MS_TO_TICKS(DEBUG_TX_TIMEOUT_MS));
    len -= chunk;
  }
  tx_mutex_put(&s_debug_lock);
}

HAL_StatusTypeDef bsp_rs485_transmit(const uint8_t *data, uint16_t len, uint32_t timeout_ms)
{
  HAL_StatusTypeDef status;

  rs485_set_transmit(1U);
  status = HAL_UART_Transmit_DMA(&huart1, data, len);
  if (status == HAL_OK)
  {
    /* TxCplt fires on the TC flag, i.e. after the last stop bit left the pin */
    if (tx_semaphore_get(&s_rs485_tx_done, MS_TO_TICKS(timeout_ms)) != TX_SUCCESS)
    {
      (void)HAL_UART_AbortTransmit(&huart1);
      status = HAL_TIMEOUT;
    }
  }
  rs485_set_transmit(0U);
  return status;
}

HAL_StatusTypeDef bsp_rs485_receive(uint8_t *buf, uint16_t max_len, uint16_t *rx_len, uint32_t timeout_ms)
{
  *rx_len = 0U;
  s_rs485_rx_len = 0U;

  if (HAL_UARTEx_ReceiveToIdle_DMA(&huart1, buf, max_len) != HAL_OK)
  {
    return HAL_ERROR;
  }
  /* The half-transfer interrupt would end the frame too early */
  __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);

  if (tx_semaphore_get(&s_rs485_rx_done, MS_TO_TICKS(timeout_ms)) != TX_SUCCESS)
  {
    (void)HAL_UART_AbortReceive(&huart1);
    return HAL_TIMEOUT;
  }
  *rx_len = s_rs485_rx_len;
  return (*rx_len > 0U) ? HAL_OK : HAL_ERROR;
}

/* HAL callbacks ------------------------------------------------------------*/

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    tx_semaphore_put(&s_debug_tx_done);
  }
  else if (huart->Instance == USART1)
  {
    tx_semaphore_put(&s_rs485_tx_done);
  }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart->Instance == USART1)
  {
    s_rs485_rx_len = Size;
    tx_semaphore_put(&s_rs485_rx_done);
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    /* Wake the waiting receiver; s_rs485_rx_len stays 0, so it reports an error */
    tx_semaphore_put(&s_rs485_rx_done);
  }
}
