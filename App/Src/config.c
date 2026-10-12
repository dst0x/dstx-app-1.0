/**
 * @file    config.c
 * @brief   Sensor table and config.json loader. cJSON uses a static bump
 *          allocator instead of the heap.
 */

#include "config.h"
#include "bsp_sdcard.h"
#include "bsp_uart.h"
#include "cJSON.h"
#include <string.h>

const sensor_def_t g_sensor_defs[] = {
    { "example_temp", 0U, 1U,   3U, 0x0000U, 0.1f,  0.0f },
};
const uint8_t g_sensor_count = (uint8_t)(sizeof(g_sensor_defs) / sizeof(g_sensor_defs[0]));

config_t g_config = {
    .devid          = "UNKNOWN",
    .dev_inv        = 300U,
    .server         = "localhost",
    .server_port    = 1883U,
    .bt_name        = "DEVICE",
    .bt_key         = "000000",
    .protocol_mode  = "mqtt",
    .inet_mode      = 0U,
    .gsm_apn        = "",
    .wifi_ssid      = "",
    .wifi_pass      = "",
    .mqtt_topic     = "dstxlabs/signal",
    .mqtt_user      = "",
    .mqtt_pass      = "",
    .ntp_timezone   = 7
};

#define CJSON_HEAP_SIZE (4096U)
static uint8_t  s_cjson_heap[CJSON_HEAP_SIZE] __attribute__((aligned(8)));
static uint32_t s_cjson_heap_idx = 0U;

static void *cjson_malloc(size_t sz)
{
    size_t aligned_sz = (sz + 7U) & ~(size_t)7U;
    void  *ptr;

    if ((s_cjson_heap_idx + aligned_sz) > CJSON_HEAP_SIZE) {
        return NULL;
    }
    ptr = &s_cjson_heap[s_cjson_heap_idx];
    s_cjson_heap_idx += aligned_sz;
    return ptr;
}

static void cjson_free(void *ptr)
{
    (void)ptr;
}

static uint8_t get_string(const cJSON *root, const char *key, char *dest)
{
    const cJSON *item = cJSON_GetObjectItem(root, key);

    if (cJSON_IsString(item) && (item->valuestring != NULL)) {
        strncpy(dest, item->valuestring, CONFIG_STR_MAX - 1U);
        dest[CONFIG_STR_MAX - 1U] = '\0';
        return 1U;
    }
    return 0U;
}

static uint8_t get_number(const cJSON *root, const char *key, double *value)
{
    const cJSON *item = cJSON_GetObjectItem(root, key);

    if (cJSON_IsNumber(item)) {
        *value = item->valuedouble;
        return 1U;
    }
    return 0U;
}

uint8_t config_load(void)
{
    static char buf[CONFIG_FILE_BUF];
    uint32_t    len = 0U;
    cJSON      *root;
    cJSON_Hooks hooks;
    double      num;

    if (bsp_sd_read_file(CONFIG_FILENAME, buf, sizeof(buf), &len) != HAL_OK) {
        bsp_uart_debug_printf("[ERROR] Cannot read %s\r\n", CONFIG_FILENAME);
        return 0U;
    }

    bsp_uart_debug_printf("[INFO] %s (%lu bytes):\r\n", CONFIG_FILENAME, (unsigned long)len);
    bsp_uart_debug_print(buf);
    bsp_uart_debug_print("\r\n");

    s_cjson_heap_idx = 0U;
    hooks.malloc_fn = cjson_malloc;
    hooks.free_fn   = cjson_free;
    cJSON_InitHooks(&hooks);

    root = cJSON_Parse(buf);
    if (root == NULL) {
        bsp_uart_debug_print("[ERROR] cJSON_Parse failed, invalid JSON\r\n");
        return 0U;
    }

    /* required fields — warn if missing */
    if (get_string(root, "devid", g_config.devid) == 0U) {
        bsp_uart_debug_print("[WARN] config: devid missing\r\n");
    }
    (void)get_number(root, "dev_inv", &num);
    if (num > 0.0) { g_config.dev_inv = (uint32_t)num; }
    if (get_string(root, "server", g_config.server) == 0U) {
        bsp_uart_debug_print("[WARN] config: server missing\r\n");
    }
    if (get_number(root, "server_port", &num) != 0U) {
        g_config.server_port = (uint16_t)num;
    }
    if (get_number(root, "inet_mode", &num) != 0U) {
        g_config.inet_mode = (uint8_t)num;
    }
    if (get_string(root, "wifi_ssid", g_config.wifi_ssid) == 0U) {
        bsp_uart_debug_print("[WARN] config: wifi_ssid missing\r\n");
    }
    if (get_string(root, "wifi_pass", g_config.wifi_pass) == 0U) {
        bsp_uart_debug_print("[WARN] config: wifi_pass missing\r\n");
    }
    if (get_string(root, "mqtt_topic", g_config.mqtt_topic) == 0U) {
        bsp_uart_debug_print("[WARN] config: mqtt_topic missing\r\n");
    }
    if (get_string(root, "mqtt_user", g_config.mqtt_user) == 0U) {
        bsp_uart_debug_print("[WARN] config: mqtt_user missing\r\n");
    }
    if (get_string(root, "mqtt_pass", g_config.mqtt_pass) == 0U) {
        bsp_uart_debug_print("[WARN] config: mqtt_pass missing\r\n");
    }
    if (get_number(root, "ntp_timezone", &num) != 0U) {
        g_config.ntp_timezone = (int8_t)num;
    }

    /* optional fields — silent if missing */
    (void)get_string(root, "bt_name", g_config.bt_name);
    (void)get_string(root, "bt_key", g_config.bt_key);
    (void)get_string(root, "protocol_mode", g_config.protocol_mode);
    (void)get_string(root, "gsm_apn", g_config.gsm_apn);

    /* Release everything held by the bump allocator */
    s_cjson_heap_idx = 0U;
    return 1U;
}

void config_print(void)
{
    bsp_uart_debug_printf("[INFO] Device ID      : %s\r\n", g_config.devid);
    bsp_uart_debug_printf("[INFO] Interval       : %lu s\r\n", (unsigned long)g_config.dev_inv);
    bsp_uart_debug_printf("[INFO] Server         : %s:%u\r\n", g_config.server, (unsigned)g_config.server_port);
    bsp_uart_debug_printf("[INFO] Protocol       : %s\r\n", g_config.protocol_mode);
    bsp_uart_debug_printf("[INFO] Bluetooth      : %s\r\n", g_config.bt_name);
    bsp_uart_debug_printf("[INFO] Internet mode  : %s\r\n", (g_config.inet_mode == 0U) ? "GSM" : "WiFi");
    if (g_config.inet_mode == 0U) {
        bsp_uart_debug_printf("[INFO] APN            : %s\r\n", g_config.gsm_apn);
    } else {
        bsp_uart_debug_printf("[INFO] WiFi SSID      : %s\r\n", g_config.wifi_ssid);
    }
    bsp_uart_debug_printf("[INFO] MQTT topic     : %s\r\n", g_config.mqtt_topic);
    bsp_uart_debug_printf("[INFO] MQTT user      : %s\r\n", g_config.mqtt_user);
    bsp_uart_debug_printf("[INFO] NTP timezone   : %+d\r\n", (int)g_config.ntp_timezone);
}
