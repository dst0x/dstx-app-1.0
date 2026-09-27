/**
 * @file    sensor_manager.c
 * @brief   Modbus sensor polling.
 */

#include "sensor_manager.h"
#include "config.h"
#include "modbus_master.h"
#include "bsp_iwdg.h"
#include "bsp_uart.h"
#include <string.h>

#define SENSOR_MAX  (16U)

static sensor_reading_t s_readings[SENSOR_MAX];

void sensor_manager_init(void)
{
    (void)memset(s_readings, 0, sizeof(s_readings));
}

void sensor_manager_poll(void)
{
    uint8_t count = (g_sensor_count < SENSOR_MAX) ? g_sensor_count : (uint8_t)SENSOR_MAX;

    for (uint8_t i = 0U; i < count; i++) {
        const sensor_def_t *def = &g_sensor_defs[i];
        sensor_reading_t   *rd  = &s_readings[i];
        uint16_t           raw = 0U;
        modbus_status_t    st;

        if (def->enabled == 0U) {
            continue;
        }

        bsp_iwdg_refresh();
        st = modbus_read_registers(def->slave_addr, def->function, def->reg_addr, 1U, &raw);
        if (st == MODBUS_OK) {
            rd->value = ((float)raw * def->scale) + def->offset;
            rd->valid = 1U;
            rd->error_count = 0U;
        } else {
            rd->valid = 0U;
            rd->error_count++;
            bsp_uart_debug_printf("[WARN] sensor %s (slave %u): %s\r\n",
                                 def->name, (unsigned)def->slave_addr, modbus_status_string(st));
        }
    }
}

const sensor_reading_t *sensor_manager_get(uint8_t index)
{
    if ((index >= g_sensor_count) || (index >= SENSOR_MAX)) {
        return NULL;
    }
    return &s_readings[index];
}
