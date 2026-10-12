/**
 * @file    modbus_master.h
 * @brief   Modbus RTU master over RS-485 (BSP_RS485_*).
 *
 * Supports function 03 (read holding registers) and 04 (read input
 * registers). Call from a single thread; requests are not queued.
 */
#ifndef MODBUS_MASTER_H
#define MODBUS_MASTER_H

#include <stdint.h>

#define MODBUS_FC_READ_HOLDING  (0x03U)
#define MODBUS_FC_READ_INPUT    (0x04U)

typedef enum {
    MODBUS_OK = 0,
    MODBUS_ERR_PARAM,      /**< Invalid argument */
    MODBUS_ERR_TX,         /**< Transmission failed */
    MODBUS_ERR_TIMEOUT,    /**< No reply from the slave */
    MODBUS_ERR_CRC,        /**< Reply with a bad CRC */
    MODBUS_ERR_FRAME,      /**< Reply from another slave / function or wrong length */
    MODBUS_ERR_EXCEPTION   /**< Slave replied with an exception code */
} modbus_status_t;

/**
 * @brief  Reads consecutive 16-bit registers from a slave.
 * @param  slave      slave address (1..247)
 * @param  function   MODBUS_FC_READ_HOLDING or MODBUS_FC_READ_INPUT
 * @param  start_addr first register (0-based protocol address)
 * @param  quantity   number of registers (1..MODBUS_MAX_REGISTERS)
 * @param  dest       destination, quantity elements
 */
modbus_status_t modbus_read_registers(uint8_t slave, uint8_t function, uint16_t start_addr,
                                     uint16_t quantity, uint16_t *dest);

uint8_t modbus_get_last_exception(void);
const char *modbus_status_string(modbus_status_t status);
uint16_t modbus_crc16(const uint8_t *data, uint16_t len);

#endif /* MODBUS_MASTER_H */
