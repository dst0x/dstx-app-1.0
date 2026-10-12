/**
 * @file    bsp_at.c
 * @brief   Generic AT command engine — send command, wait for final result
 *          code (OK / ERROR / BUSY), capture intermediate response lines,
 *          and dispatch unsolicited result codes (URCs) via callback.
 */

#include "bsp_at.h"
#include "bsp_iwdg.h"
#include <string.h>
#include <stdio.h>


static uint16_t rx_available(at_dev_t *dev)
{
    uint16_t head = AT_RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(dev->huart->hdmarx);
    if (head >= dev->rx_tail) {
        return head - dev->rx_tail;
    }
    return AT_RX_BUF_SIZE - dev->rx_tail + head;
}

static uint8_t rx_read_byte(at_dev_t *dev)
{
    uint8_t b = dev->rx_buf[dev->rx_tail];
    dev->rx_tail = (dev->rx_tail + 1U) % AT_RX_BUF_SIZE;
    return b;
}

/* Scans for \n in the ring buffer, copies without CR/LF */
static uint8_t get_line(at_dev_t *dev, char *line, uint16_t max_len)
{
    uint16_t avail = rx_available(dev);
    uint16_t scan  = 0U;

    /* find \n in the ring buffer */
    while (scan < avail) {
        uint8_t b = dev->rx_buf[(dev->rx_tail + scan) % AT_RX_BUF_SIZE];
        if (b == '\n') {
            uint16_t out = 0U;
            for (uint16_t i = 0U; i <= scan; i++) {
                uint8_t c = rx_read_byte(dev);
                if (c != '\r' && c != '\n' && out < (max_len - 1U)) {
                    line[out++] = (char)c;
                }
            }
            line[out] = '\0';
            return (out > 0U) ? 1U : 0U;
        }
        scan++;
    }
    return 0U;
}

static at_result_t parse_final(const char *line)
{
    if (strcmp(line, "OK") == 0) {
        return AT_OK;
    }
    if (strcmp(line, "ERROR") == 0 ||
        strncmp(line, "+CME ERROR", 10) == 0 ||
        strncmp(line, "+CMS ERROR", 10) == 0) {
        return AT_ERROR;
    }
    if (strcmp(line, "BUSY p...") == 0 || strncmp(line, "busy", 4) == 0) {
        return AT_BUSY;
    }
    return (at_result_t)0xFFU; /* not a final code */
}

HAL_StatusTypeDef bsp_at_init(at_dev_t *dev, UART_HandleTypeDef *huart, at_urc_handler_t urc_cb)
{
    dev->huart       = huart;
    dev->rx_tail     = 0U;
    dev->rx_head     = 0U;
    dev->urc_handler = urc_cb;

    memset(dev->rx_buf, 0, sizeof(dev->rx_buf));

    /* start DMA circular receive */
    return HAL_UART_Receive_DMA(huart, dev->rx_buf, AT_RX_BUF_SIZE);
}

void bsp_at_deinit(at_dev_t *dev)
{
    HAL_UART_AbortReceive(dev->huart);
    dev->rx_tail = 0U;
}

at_result_t bsp_at_send_cmd(at_dev_t *dev, const char *cmd, uint32_t timeout_ms)
{
    return bsp_at_send_cmd_resp(dev, cmd, NULL, 0U, timeout_ms);
}

at_result_t bsp_at_send_cmd_resp(at_dev_t *dev, const char *cmd,
                                 char *resp, uint16_t resp_size,
                                 uint32_t timeout_ms)
{
    char     tx[AT_TX_BUF_SIZE];
    char     line[AT_LINE_MAX];
    uint32_t start = HAL_GetTick();
    int      n;

    n = snprintf(tx, sizeof(tx), "%s\r\n", cmd);
    if (n <= 0 || (uint16_t)n >= sizeof(tx)) {
        return AT_SEND_FAIL;
    }

    if (resp != NULL && resp_size > 0U) {
        resp[0] = '\0';
    }

    /* flush stale data */
    dev->rx_tail = AT_RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(dev->huart->hdmarx);

    bsp_iwdg_refresh();
    if (HAL_UART_Transmit(dev->huart, (const uint8_t *)tx, (uint16_t)n, timeout_ms) != HAL_OK) {
        return AT_SEND_FAIL;
    }
    /* wait for final result code */
    while ((HAL_GetTick() - start) < timeout_ms) {
        bsp_iwdg_refresh();
        if (get_line(dev, line, sizeof(line)) != 0U) {
            /* skip echo of the command */
            if (strncmp(line, cmd, strlen(cmd)) == 0) {
                continue;
            }

            at_result_t rc = parse_final(line);
            if (rc != (at_result_t)0xFFU) {
                return rc;
            }

            /* intermediate response line */
            if (resp != NULL && resp_size > 1U) {
                uint16_t cur = (uint16_t)strlen(resp);
                if (cur < (resp_size - 2U)) {
                    uint16_t rem = resp_size - cur - 1U;
                    if (cur > 0U) {
                        resp[cur++] = '\n';
                        resp[cur] = '\0';
                        rem--;
                    }
                    strncat(resp, line, rem);
                }
            }

            /* dispatch as URC if handler registered */
            if (dev->urc_handler != NULL) {
                dev->urc_handler(line);
            }
        }
    }
    return AT_TIMEOUT;
}

at_result_t bsp_at_send_raw(at_dev_t *dev, const uint8_t *data, uint16_t len, uint32_t timeout_ms)
{
    if (HAL_UART_Transmit(dev->huart, data, len, timeout_ms) != HAL_OK) {
        return AT_SEND_FAIL;
    }
    return AT_OK;
}

at_result_t bsp_at_wait_match(at_dev_t *dev, const char *match, uint32_t timeout_ms)
{
    char     line[AT_LINE_MAX];
    uint32_t start = HAL_GetTick();

    while ((HAL_GetTick() - start) < timeout_ms) {
        bsp_iwdg_refresh();
        if (get_line(dev, line, sizeof(line)) != 0U) {
            if (strstr(line, match) != NULL) {
                return AT_OK;
            }
            at_result_t rc = parse_final(line);
            if (rc == AT_ERROR || rc == AT_BUSY) {
                return rc;
            }
        }
    }
    return AT_TIMEOUT;
}

void bsp_at_process(at_dev_t *dev)
{
    char line[AT_LINE_MAX];

    while (get_line(dev, line, sizeof(line)) != 0U) {
        if (dev->urc_handler != NULL) {
            dev->urc_handler(line);
        }
    }
}
