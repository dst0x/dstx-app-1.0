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

/** @brief Clears all readings. */
void sensor_manager_init(void);

/** @brief Reads every enabled sensor once. Blocks for up to ~300 ms per sensor. */
void sensor_manager_poll(void);

/** @brief Latest reading of sensor index i (same order as g_sensor_defs), or NULL. */
const sensor_reading_t *sensor_manager_get(uint8_t index);

#endif /* SENSOR_MANAGER_H */
