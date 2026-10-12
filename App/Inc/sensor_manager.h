/**
 * @file    sensor_manager.h
 * @brief   Polls the sensors listed in g_sensor_defs (config.c) over Modbus
 *          and keeps the latest scaled value of each one.
 */
#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <stdint.h>

typedef struct {
    float    value;        /**< Latest scaled value */
    uint8_t  valid;        /**< 1 if the last read succeeded */
    uint32_t error_count;  /**< Consecutive failed reads */
} sensor_reading_t;

void sensor_manager_init(void);
void sensor_manager_poll(void);

const sensor_reading_t *sensor_manager_get(uint8_t index);

#endif /* SENSOR_MANAGER_H */
