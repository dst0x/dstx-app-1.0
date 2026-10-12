/**
 * @file    app.c
 * @brief   Main application state machine.
 */

#include "app.h"
#include "config.h"
#include "datalogger.h"
#include "esp32_util.h"
#include "safety.h"
#include "sensor_manager.h"
#include "bsp_gpio.h"
#include "bsp_iwdg.h"
#include "bsp_sdcard.h"
#include "bsp_uart.h"
#include "tx_api.h"
#include <stdio.h>
#include <string.h>

#define MS_TO_TICKS(ms)  ((((ULONG)(ms) * TX_TIMER_TICKS_PER_SECOND) + 999U) / 1000U)
#define SIGNAL_INTERVAL_S  (g_config.dev_inv)

static app_state_t s_state = APP_STATE_INIT;
static uint8_t     s_sd_ready   = 0U;
static uint8_t     s_wifi_ready = 0U;
static uint32_t    s_epoch_base = 0U;
static uint32_t    s_epoch_tick = 0U;

#define UNSENT_FILENAME  "unsent.csv"

static uint32_t datetime_to_epoch(uint16_t y, uint8_t mo, uint8_t d,
                                  uint8_t h, uint8_t mi, uint8_t s)
{
    uint32_t days = 0U;
    for (uint16_t yr = 1970U; yr < y; yr++) {
        days += ((yr % 4U == 0U) && ((yr % 100U != 0U) || (yr % 400U == 0U))) ? 366U : 365U;
    }
    static const uint16_t mdays[] = {0,31,59,90,120,151,181,212,243,273,304,334};
    days += mdays[mo - 1U] + d - 1U;
    if (mo > 2U && (y % 4U == 0U) && ((y % 100U != 0U) || (y % 400U == 0U))) {
        days++;
    }
    return days * 86400U + (uint32_t)h * 3600U + (uint32_t)mi * 60U + s;
}

static uint32_t get_timestamp(void)
{
    if (s_epoch_base == 0U) {
        return 0U;
    }
    return s_epoch_base + (HAL_GetTick() - s_epoch_tick) / 1000U;
}

static uint32_t seconds_to_next_slot(uint32_t now_epoch)
{
    uint32_t rem = now_epoch % SIGNAL_INTERVAL_S;
    return (rem == 0U) ? 0U : (SIGNAL_INTERVAL_S - rem);
}

static uint8_t unsent_save(const char *payload)
{
    if (s_sd_ready == 0U) {
        return 0U;
    }
    char line[ESP32_MQTT_PAYLOAD_MAX + 2U];
    int n = snprintf(line, sizeof(line), "%s\n", payload);
    if (n > 0) {
        bsp_iwdg_refresh();
        if (bsp_sd_append_file(UNSENT_FILENAME, line, (uint32_t)n) == HAL_OK) {
            return 1U;
        }
    }
    return 0U;
}

static void unsent_flush(void)
{
    if (s_sd_ready == 0U) {
        return;
    }
    bsp_iwdg_refresh();
    if (bsp_sd_open_read(UNSENT_FILENAME) != HAL_OK) {
        return;
    }

    bsp_uart_debug_print("[MQTT] flushing unsent data...\r\n");
    static char line[ESP32_MQTT_PAYLOAD_MAX];
    uint32_t sent = 0U;
    uint8_t  had_fail = 0U;

    while (bsp_sd_read_line(line, sizeof(line)) != 0U) {
        bsp_iwdg_refresh();
        if (esp32_send_signal(line) == ESP32_OK) {
            sent++;
        } else {
            had_fail = 1U;
            break;
        }
    }
    bsp_sd_close();

    bsp_uart_debug_printf("[MQTT] flush done: %lu sent\r\n", (unsigned long)sent);
    if (had_fail == 0U) {
        (void)bsp_sd_delete_file(UNSENT_FILENAME);
    }
}

static void app_init(void)
{
    bsp_gpio_init();
    safety_init();
    sensor_manager_init();

    bsp_uart_debug_printf("\r\n[STARTUP] DST0X application v%s\r\n", APP_VERSION);
    if ((safety_get_faults() & SAFETY_FAULT_IWDG_RESET) != 0U) {
        bsp_uart_debug_print("[WARN] Last reset was caused by the watchdog\r\n");
    }

    bsp_iwdg_refresh();
    if (bsp_sd_init() != HAL_OK) {
        bsp_uart_debug_print("[ERROR] SD card init failed\r\n");
    } else if (bsp_sd_mount() != HAL_OK) {
        bsp_uart_debug_print("[ERROR] SD card mount failed\r\n");
    } else {
        bsp_uart_debug_print("[INFO] SD card mounted\r\n");
        s_sd_ready = 1U;
    }

    bsp_iwdg_refresh();
    if ((s_sd_ready != 0U) && (config_load() != 0U)) {
        config_print();
    } else {
        bsp_uart_debug_print("[WARN] Using default configuration\r\n");
    }

    bsp_iwdg_refresh();
    if ((s_sd_ready != 0U) && (datalogger_init() == 0U)) {
        bsp_uart_debug_print("[ERROR] datalogger: cannot create " DATALOG_FILENAME "\r\n");
    }

    /* ESP32 — WiFi + NTP */
    bsp_iwdg_refresh();
    if (esp32_init() == ESP32_OK) {
        if (g_config.inet_mode == 1U) {
            if (esp32_wifi_connect(g_config.wifi_ssid, g_config.wifi_pass) == ESP32_OK) {
                s_wifi_ready = 1U;
                bsp_iwdg_refresh();
                if (esp32_ntp_config(g_config.ntp_timezone, NULL) != ESP32_OK) {
                    bsp_uart_debug_print("[ERROR] NTP config failed\r\n");
                } else {
                    uint16_t yr; uint8_t mo, dy, hh, mm, ss;
                    uint8_t ntp_ok = 0U;
                    for (uint8_t t = 0U; t < 3U; t++) {
                        bsp_iwdg_refresh();
                        HAL_Delay(2000U);
                        bsp_iwdg_refresh();
                        if (esp32_ntp_get_time(&yr, &mo, &dy, &hh, &mm, &ss) == ESP32_OK) {
                            s_epoch_base = datetime_to_epoch(yr, mo, dy, hh, mm, ss);
                            s_epoch_tick = HAL_GetTick();
                            bsp_uart_debug_printf("[NTP] %04u-%02u-%02u %02u:%02u:%02u (epoch %lu)\r\n",
                                                  yr, mo, dy, hh, mm, ss, (unsigned long)s_epoch_base);
                            ntp_ok = 1U;
                            break;
                        }
                        bsp_uart_debug_printf("[WARN] NTP sync retry %u/3\r\n", (unsigned)(t + 1U));
                    }
                    if (ntp_ok == 0U) {
                        bsp_uart_debug_print("[ERROR] NTP sync failed after 3 retries\r\n");
                    }
                }
            }
        }
    }
}

static void app_enter_fault(void)
{
    (void)bsp_relay_set(BSP_RELAY_1, 0U);
    (void)bsp_relay_set(BSP_RELAY_2, 0U);
    bsp_led_on();
    bsp_uart_debug_printf("[FAULT] Safety trip, faults = 0x%02X\r\n", (unsigned)safety_get_faults());
    s_state = APP_STATE_FAULT;
}

void app_pre_kernel_init(void)
{
    (void)bsp_uart_init();
}

static void build_signal_payload(char *buf, uint16_t size)
{
    int n;

    const esp32_status_t *st = esp32_get_status();
    int signal = (st->rssi < -100) ? 0 : (st->rssi > -50) ? 100 : (100 + (int)st->rssi + 50);
    uint32_t uptime_s = HAL_GetTick() / 1000U;
    n = snprintf(buf, size,
                 "{\"devid\":\"%s\",\"ver\":\"%s\",\"timestamp\":%lu,\"uptime\":%lu,\"signal\":%d",
                 g_config.devid, APP_VERSION,
                 (unsigned long)get_timestamp(), (unsigned long)uptime_s, signal);
    if (n < 0 || (uint16_t)n >= size) {
        buf[0] = '\0';
        return;
    }

    for (uint8_t i = 0U; i < g_sensor_count; i++) {
        const sensor_reading_t *rd = sensor_manager_get(i);
        if ((rd != NULL) && (rd->valid != 0U)) {
            int32_t milli = (int32_t)(rd->value * 1000.0f + ((rd->value < 0.0f) ? -0.5f : 0.5f));
            uint32_t abs_m = (milli < 0) ? (uint32_t)(-milli) : (uint32_t)milli;
            int w = snprintf(buf + n, (uint16_t)(size - (uint16_t)n),
                             ",\"%s\":%s%lu.%03lu",
                             g_sensor_defs[i].name,
                             (milli < 0) ? "-" : "",
                             (unsigned long)(abs_m / 1000U),
                             (unsigned long)(abs_m % 1000U));
            if (w > 0) {
                n += w;
            }
        }
    }

    if ((uint16_t)n < (size - 2U)) {
        buf[n++] = '}';
        buf[n]   = '\0';
    }
}

static void do_signal_cycle(void)
{
    static char sig_buf[ESP32_MQTT_PAYLOAD_MAX];

    sensor_manager_poll();
    if (s_sd_ready != 0U) {
        (void)datalogger_log_readings();
    }

    build_signal_payload(sig_buf, sizeof(sig_buf));
    esp32_result_t src = esp32_send_signal(sig_buf);
    bsp_uart_debug_printf("[MQTT] %s rc=%d\r\n", sig_buf, (int)src);

    if (src == ESP32_OK) {
        unsent_flush();
    } else {
        unsent_save(sig_buf);
    }
}

static void enter_low_power(uint32_t sleep_sec)
{
    bsp_uart_debug_printf("[SLEEP] %lu s until next slot\r\n", (unsigned long)sleep_sec);

    /* put ESP32 in light sleep — wakes on UART activity */
    (void)esp32_mqtt_disconnect();
    (void)esp32_wifi_disconnect();

    /* sleep in chunks to keep IWDG alive */
    uint32_t remaining_ms = sleep_sec * 1000U;
    while (remaining_ms > 0U) {
        bsp_iwdg_refresh();
        uint32_t chunk = (remaining_ms > 5000U) ? 5000U : remaining_ms;
        tx_thread_sleep(MS_TO_TICKS(chunk));
        remaining_ms -= chunk;
    }

    bsp_uart_debug_print("[WAKE] reconnecting...\r\n");

    /* re-init SD card after sleep */
    if (s_sd_ready != 0U) {
        bsp_iwdg_refresh();
        if (bsp_sd_init() != HAL_OK || bsp_sd_mount() != HAL_OK) {
            bsp_uart_debug_print("[WARN] SD card re-mount failed\r\n");
            s_sd_ready = 0U;
        }
    }

    /* reconnect WiFi with 3 retries */
    bsp_iwdg_refresh();
    uint8_t wifi_ok = 0U;
    for (uint8_t r = 0U; r < 3U; r++) {
        if (esp32_wifi_connect(g_config.wifi_ssid, g_config.wifi_pass) == ESP32_OK) {
            wifi_ok = 1U;
            break;
        }
        bsp_uart_debug_printf("[WARN] WiFi retry %u/3 failed\r\n", (unsigned)(r + 1U));
        bsp_iwdg_refresh();
        HAL_Delay(2000U);
        bsp_iwdg_refresh();
    }
    if (wifi_ok == 0U) {
        bsp_uart_debug_print("[ERROR] WiFi reconnect failed, skipping this slot\r\n");
        s_wifi_ready = 0U;
        return;
    }
    s_wifi_ready = 1U;

    /* resync NTP time to correct drift */
    bsp_iwdg_refresh();
    if (esp32_ntp_config(g_config.ntp_timezone, NULL) == ESP32_OK) {
        uint16_t yr; uint8_t mo, dy, hh, mm, ss;
        for (uint8_t t = 0U; t < 3U; t++) {
            bsp_iwdg_refresh();
            HAL_Delay(2000U);
            bsp_iwdg_refresh();
            if (esp32_ntp_get_time(&yr, &mo, &dy, &hh, &mm, &ss) == ESP32_OK) {
                s_epoch_base = datetime_to_epoch(yr, mo, dy, hh, mm, ss);
                s_epoch_tick = HAL_GetTick();
                break;
            }
        }
    }
}

void app_run(void)
{
    for (;;) {
        safety_service();
        esp32_process();

        switch (s_state) {
            case APP_STATE_INIT:
                app_init();
                s_state = APP_STATE_RUN;
                /* send immediately on boot */
                if (s_wifi_ready != 0U) {
                    do_signal_cycle();
                }
                break;

            case APP_STATE_RUN:
                if (safety_is_tripped() != 0U) {
                    app_enter_fault();
                    break;
                }

                if (s_epoch_base != 0U) {
                    uint32_t now = get_timestamp();
                    uint32_t wait = seconds_to_next_slot(now);

                    if (wait == 0U) {
                        wait = SIGNAL_INTERVAL_S;
                    }

                    enter_low_power(wait);

                    if (s_wifi_ready != 0U) {
                        bsp_iwdg_refresh();
                        do_signal_cycle();
                    } else {
                        /* WiFi failed — save data to SD, try next slot */
                        static char sig_buf[ESP32_MQTT_PAYLOAD_MAX];
                        sensor_manager_poll();
                        build_signal_payload(sig_buf, sizeof(sig_buf));
                        if (unsent_save(sig_buf) != 0U) {
                            bsp_uart_debug_printf("[SD] saved unsent: %s\r\n", sig_buf);
                        } else {
                            bsp_uart_debug_print("[SD] unsent save failed\r\n");
                        }
                    }
                } else {
                    bsp_led_toggle();
                    tx_thread_sleep(MS_TO_TICKS(1000U));
                }
                break;

            case APP_STATE_FAULT:
            default:
                safety_clear_faults();
                if (safety_is_tripped() == 0U) {
                    bsp_uart_debug_print("[INFO] Faults cleared, resuming\r\n");
                    bsp_led_off();
                    s_state = APP_STATE_RUN;
                }
                tx_thread_sleep(MS_TO_TICKS(1000U));
                break;
        }
    }
}

app_state_t app_get_state(void)
{
    return s_state;
}
