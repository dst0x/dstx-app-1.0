/**
 * @file    bsp_uart.h
 * @brief   UART board support:
 *          - USART2 (PA2/PA3, DEBUG_TX/RX): debug log, interrupt mode
 *          - USART1 (PA9/PA10, RS485_TX/RX): RS-485 bus, DMA mode, DE/RE control
 *
 * All functions except bsp_uart_init() block on ThreadX semaphores, so they
 * must be called from a thread, never from an ISR or before the kernel starts.
 *
 * RS-485 direction pin: give the DE/RE pin the user label "RS485_DE" in
 * STM32CubeMX (GPIO output). CubeMX then defines RS485_DE_Pin and
 * RS485_DE_GPIO_Port in main.h and this driver drives it automatically.
 */
#ifndef BSP_UART_H
#define BSP_UART_H

#include "main.h"
#include <stdint.h>

/** @brief Creates the ThreadX semaphores. Call once from App_ThreadX_Init(). */
HAL_StatusTypeDef bsp_uart_init(void);

/** @brief Sends a null-terminated string on the debug UART. */
void bsp_uart_debug_print(const char *str);

/** @brief printf-style debug output (max 255 characters per call). */
void bsp_uart_debug_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/**
 * @brief  Transmits a frame on the RS-485 bus (DE high while sending).
 * @param  data       frame to send
 * @param  len        frame length in bytes
 * @param  timeout_ms maximum time to wait for the end of transmission
 */
HAL_StatusTypeDef bsp_rs485_transmit(const uint8_t *data, uint16_t len, uint32_t timeout_ms);

/**
 * @brief  Receives one frame from the RS-485 bus. A frame ends when the line
 *         goes idle or when max_len bytes have been received.
 * @param  buf        destination buffer
 * @param  max_len    buffer size
 * @param  rx_len     number of bytes received
 * @param  timeout_ms maximum time to wait for a frame
 */
HAL_StatusTypeDef bsp_rs485_receive(uint8_t *buf, uint16_t max_len, uint16_t *rx_len, uint32_t timeout_ms);

#endif /* BSP_UART_H */
