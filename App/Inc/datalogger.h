/*
 * datalogger.h
 *
 *  Created on: Sep 27, 2026
 *      Author: DSTX Labs
 */

#ifndef DATALOGGER_H
#define DATALOGGER_H

#include <stdint.h>

uint8_t datalogger_init(void);
uint8_t datalogger_log_readings(void);

#endif /* DATALOGGER_H */
