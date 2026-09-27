/**
 * @file    app.c
 * @brief   Main application state machine.
 */

#include "app.h"
#include "config.h"
#include "datalogger.h"
#include "safety.h"
#include "sensor_manager.h"
#include "bsp_gpio.h"
#include "bsp_iwdg.h"
#include "bsp_sdcard.h"
#include "bsp_uart.h"
#include "tx_api.h"

#define MS_TO_TICKS(ms)  ((((ULONG)(ms) * TX_TIMER_TICKS_PER_SECOND) + 999U) / 1000U)

static app_state_t s_state = APP_STATE_INIT;
static uint8_t     s_sd_ready = 0U;

static void app_init(void)
{
    bsp_gpio_init();
    safety_init();
    sensor_manager_init();

    bsp_uart_debug_print("\r\n[STARTUP] DST0X application\r\n");
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

void app_run(void)
{
    uint32_t last_heartbeat = HAL_GetTick();
    uint32_t last_log = HAL_GetTick();

    for (;;) {
        safety_service();

        switch (s_state) {
            case APP_STATE_INIT:
                app_init();
                s_state = APP_STATE_RUN;
                break;

            case APP_STATE_RUN:
                if (safety_is_tripped() != 0U) {
                    app_enter_fault();
                    break;
                }
                if ((HAL_GetTick() - last_heartbeat) >= APP_HEARTBEAT_PERIOD_MS) {
                    last_heartbeat += APP_HEARTBEAT_PERIOD_MS;
                    bsp_led_toggle();
                    bsp_uart_debug_print("Hello World\r\n");
                }
                if ((HAL_GetTick() - last_log) >= (g_config.dev_interval * 1000U)) {
                    last_log = HAL_GetTick();
                    sensor_manager_poll();
                    if (s_sd_ready != 0U) {
                        (void)datalogger_log_readings();
                    }
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
                break;
        }

        tx_thread_sleep(MS_TO_TICKS(APP_LOOP_PERIOD_MS));
    }
}

app_state_t app_get_state(void)
{
    return s_state;
}
