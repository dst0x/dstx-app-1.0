/**
 * @file    datalogger.c
 * @brief   CSV data logger on the SD card.
 */

#include "datalogger.h"
#include "config.h"
#include "sensor_manager.h"
#include "bsp_sdcard.h"
#include "bsp_rtc.h"
#include "bsp_uart.h"
#include <stdio.h>

#define DATALOG_LINE_MAX  (256U)

static char s_line[DATALOG_LINE_MAX];

/* Appends text to s_line at *pos; returns 0 when the line is full */
static uint8_t line_append(size_t *pos, const char *prefix, const char *text)
{
    int n = snprintf(&s_line[*pos], DATALOG_LINE_MAX - *pos, "%s%s", prefix, text);

    if ((n < 0) || ((size_t)n >= (DATALOG_LINE_MAX - *pos))) {
        return 0U;
    }
    *pos += (size_t)n;
    return 1U;
}

static uint8_t line_append_value(size_t *pos, float value)
{
    char     num[24];
    int32_t  milli = (int32_t)((value * 1000.0f) + ((value < 0.0f) ? -0.5f : 0.5f));
    uint32_t abs_milli = (milli < 0) ? (uint32_t)(-milli) : (uint32_t)milli;

    (void)snprintf(num, sizeof(num), "%s%lu.%03lu", (milli < 0) ? "-" : "",
                   (unsigned long)(abs_milli / 1000U), (unsigned long)(abs_milli % 1000U));
    return line_append(pos, ",", num);
}

uint8_t datalogger_init(void)
{
    char   probe[2];
    size_t pos = 0U;

    /* File already there: keep appending to it */
    if (bsp_sd_read_file(DATALOG_FILENAME, probe, sizeof(probe), NULL) == HAL_OK) {
        return 1U;
    }

    (void)line_append(&pos, "", "timestamp");
    for (uint8_t i = 0U; i < g_sensor_count; i++) {
        if (line_append(&pos, ",", g_sensor_defs[i].name) == 0U) {
            return 0U;
        }
    }
    if (line_append(&pos, "", "\r\n") == 0U) {
        return 0U;
    }
    return (bsp_sd_append_file(DATALOG_FILENAME, s_line, (uint32_t)pos) == HAL_OK) ? 1U : 0U;
}

uint8_t datalogger_log_readings(void)
{
    bsp_datetime_t dt;
    size_t         pos = 0U;
    int            n;

    if (bsp_rtc_get_time(&dt) == HAL_OK) {
        n = snprintf(s_line, DATALOG_LINE_MAX, "%04u-%02u-%02u %02u:%02u:%02u",
                     (unsigned)dt.year, (unsigned)dt.month, (unsigned)dt.day,
                     (unsigned)dt.hour, (unsigned)dt.minute, (unsigned)dt.second);
    } else {
        n = snprintf(s_line, DATALOG_LINE_MAX, "%lu", (unsigned long)(HAL_GetTick() / 1000U));
    }
    if (n < 0) {
        return 0U;
    }
    pos = (size_t)n;

    for (uint8_t i = 0U; i < g_sensor_count; i++) {
        const sensor_reading_t *rd = sensor_manager_get(i);
        uint8_t ok;

        if ((rd != NULL) && (rd->valid != 0U)) {
            ok = line_append_value(&pos, rd->value);
        } else {
            ok = line_append(&pos, ",", "");
        }
        if (ok == 0U) {
            return 0U;
        }
    }
    if (line_append(&pos, "", "\r\n") == 0U) {
        return 0U;
    }

    if (bsp_sd_append_file(DATALOG_FILENAME, s_line, (uint32_t)pos) != HAL_OK) {
        bsp_uart_debug_print("[ERROR] datalogger: write failed\r\n");
        return 0U;
    }
    return 1U;
}
