/**
 * @file    bsp_at.h
 * @brief   Generic AT command engine over UART with DMA receive.
 *          Handles send, receive, response parsing, URC dispatching
 *          and timeout management for industrial modem modules.
 */
#ifndef BSP_AT_H
#define BSP_AT_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define AT_RX_BUF_SIZE      (512U)
#define AT_TX_BUF_SIZE      (256U)
#define AT_LINE_MAX         (256U)
#define AT_DEFAULT_TIMEOUT  (5000U)

typedef enum {
    AT_OK = 0,
    AT_ERROR,
    AT_TIMEOUT,
    AT_BUSY,
    AT_SEND_FAIL
} at_result_t;

typedef void (*at_urc_handler_t)(const char *line);

typedef struct {
    UART_HandleTypeDef *huart;
    uint8_t             rx_buf[AT_RX_BUF_SIZE];
    volatile uint16_t   rx_head;
    uint16_t            rx_tail;
    at_urc_handler_t    urc_handler;
} at_dev_t;

HAL_StatusTypeDef bsp_at_init(at_dev_t *dev, UART_HandleTypeDef *huart, at_urc_handler_t urc_cb);
void              bsp_at_deinit(at_dev_t *dev);

at_result_t bsp_at_send_cmd(at_dev_t *dev, const char *cmd, uint32_t timeout_ms);
at_result_t bsp_at_send_cmd_resp(at_dev_t *dev, const char *cmd,
                                 char *resp, uint16_t resp_size,
                                 uint32_t timeout_ms);
at_result_t bsp_at_send_raw(at_dev_t *dev, const uint8_t *data, uint16_t len, uint32_t timeout_ms);
at_result_t bsp_at_wait_match(at_dev_t *dev, const char *match, uint32_t timeout_ms);
void        bsp_at_process(at_dev_t *dev);

#endif /* BSP_AT_H */
