/**
 * @file    modbus_master.c
 * @brief   Modbus RTU master (functions 03 and 04).
 */

#include "modbus_master.h"
#include "config.h"
#include "bsp_uart.h"
#include <stddef.h>

#define MODBUS_REQUEST_LEN   (8U)
#define MODBUS_EXCEPTION_LEN (5U)
#define MODBUS_RX_BUF_LEN    (5U + (2U * MODBUS_MAX_REGISTERS))

static uint8_t s_last_exception = 0U;

uint16_t modbus_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFFU;

    for (uint16_t i = 0U; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0U; bit < 8U; bit++) {
            crc = ((crc & 1U) != 0U) ? (uint16_t)((crc >> 1) ^ 0xA001U) : (uint16_t)(crc >> 1);
        }
    }
    return crc;
}

modbus_status_t modbus_read_registers(uint8_t slave, uint8_t function, uint16_t start_addr,
                                     uint16_t quantity, uint16_t *dest)
{
    uint8_t  req[MODBUS_REQUEST_LEN];
    uint8_t  rsp[MODBUS_RX_BUF_LEN];
    uint16_t rx_len = 0U;
    uint16_t crc;
    uint16_t expected_len;

    if ((slave == 0U) || (slave > 247U) || (dest == NULL) ||
        ((function != MODBUS_FC_READ_HOLDING) && (function != MODBUS_FC_READ_INPUT)) ||
        (quantity == 0U) || (quantity > MODBUS_MAX_REGISTERS)) {
        return MODBUS_ERR_PARAM;
    }

    req[0] = slave;
    req[1] = function;
    req[2] = (uint8_t)(start_addr >> 8);
    req[3] = (uint8_t)(start_addr & 0xFFU);
    req[4] = (uint8_t)(quantity >> 8);
    req[5] = (uint8_t)(quantity & 0xFFU);
    crc = modbus_crc16(req, 6U);
    req[6] = (uint8_t)(crc & 0xFFU); /* CRC is sent low byte first */
    req[7] = (uint8_t)(crc >> 8);

    if (bsp_rs485_transmit(req, MODBUS_REQUEST_LEN, MODBUS_TX_TIMEOUT_MS) != HAL_OK) {
        return MODBUS_ERR_TX;
    }
    if (bsp_rs485_receive(rsp, sizeof(rsp), &rx_len, MODBUS_RESPONSE_TIMEOUT_MS) != HAL_OK) {
        return MODBUS_ERR_TIMEOUT;
    }

    if (rx_len < MODBUS_EXCEPTION_LEN) {
        return MODBUS_ERR_FRAME;
    }
    crc = modbus_crc16(rsp, (uint16_t)(rx_len - 2U));
    if ((rsp[rx_len - 2U] != (uint8_t)(crc & 0xFFU)) || (rsp[rx_len - 1U] != (uint8_t)(crc >> 8))) {
        return MODBUS_ERR_CRC;
    }
    if (rsp[0] != slave) {
        return MODBUS_ERR_FRAME;
    }
    if (rsp[1] == (uint8_t)(function | 0x80U)) {
        s_last_exception = rsp[2];
        return MODBUS_ERR_EXCEPTION;
    }

    expected_len = (uint16_t)(5U + (2U * quantity));
    if ((rsp[1] != function) || (rsp[2] != (uint8_t)(2U * quantity)) || (rx_len != expected_len)) {
        return MODBUS_ERR_FRAME;
    }

    for (uint16_t i = 0U; i < quantity; i++) {
        dest[i] = (uint16_t)(((uint16_t)rsp[3U + (2U * i)] << 8) | rsp[4U + (2U * i)]);
    }
    return MODBUS_OK;
}

uint8_t modbus_get_last_exception(void)
{
    return s_last_exception;
}

const char *modbus_status_string(modbus_status_t status)
{
    switch (status) {
        case MODBUS_OK:            return "OK";
        case MODBUS_ERR_PARAM:     return "bad parameter";
        case MODBUS_ERR_TX:        return "transmit error";
        case MODBUS_ERR_TIMEOUT:   return "timeout";
        case MODBUS_ERR_CRC:       return "CRC error";
        case MODBUS_ERR_FRAME:     return "bad frame";
        case MODBUS_ERR_EXCEPTION: return "slave exception";
        default:                   return "unknown";
    }
}
