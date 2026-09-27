/**
 * @file    datalogger.h
 * @brief   Appends sensor readings as CSV lines to DATALOG_FILENAME on the
 *          SD card. The SD card must already be mounted.
 */
#ifndef DATALOGGER_H
#define DATALOGGER_H

#include <stdint.h>

/** @brief Writes the CSV header if the log file does not exist yet. */
uint8_t datalogger_init(void);

/**
 * @brief  Appends one line: timestamp,sensor1,sensor2,...
 *         The timestamp comes from the RTC, or uptime in seconds without RTC.
 *         Invalid readings are written as empty fields.
 * @retval 1 on success, 0 on failure
 */
uint8_t datalogger_log_readings(void);

#endif /* DATALOGGER_H */
