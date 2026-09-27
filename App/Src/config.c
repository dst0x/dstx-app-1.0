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

/* Sensor table -------------------------------------------------------------*/
/* TODO: fill in from Docs/modbus_register_map.md and set enabled = 1U. */
const sensor_def_t g_sensor_defs[] = {
    /* name          en   slave fc  reg      scale  offset */
    { "example_temp", 0U, 1U,   3U, 0x0000U, 0.1f,  0.0f },
};
const uint8_t g_sensor_count = (uint8_t)(sizeof(g_sensor_defs) / sizeof(g_sensor_defs[0]));

/* Runtime configuration ----------------------------------------------------*/
config_t g_config = {
    .device_id      = "UNKNOWN",
    .dev_interval   = 3600U,
    .server_address = "localhost",
    .server_port    = 80U,
    .bt_name        = "DEVICE",
    .bt_key         = "000000",
    .internet_mode  = 0U,
    .gsm_apn        = "",
    .wifi_ssid      = "",
    .wifi_pass      = ""
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

static void get_string(const cJSON *root, const char *key, char *dest)
{
    const cJSON *item = cJSON_GetObjectItem(root, key);

    if (cJSON_IsString(item) && (item->valuestring != NULL)) {
        strncpy(dest, item->valuestring, CONFIG_STR_MAX - 1U);
        dest[CONFIG_STR_MAX - 1U] = '\0';
    } else {
        bsp_uart_debug_printf("[WARN] config: %s invalid\r\n", key);
    }
}

static uint8_t get_number(const cJSON *root, const char *key, double *value)
{
    const cJSON *item = cJSON_GetObjectItem(root, key);

    if (cJSON_IsNumber(item)) {
        *value = item->valuedouble;
        return 1U;
    }
    bsp_uart_debug_printf("[WARN] config: %s invalid\r\n", key);
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

    get_string(root, "device_id", g_config.device_id);
    if (get_number(root, "dev_interval", &num) != 0U) {
        g_config.dev_interval = (uint32_t)num;
    }
    get_string(root, "server_address", g_config.server_address);
    if (get_number(root, "server_port", &num) != 0U) {
        g_config.server_port = (uint16_t)num;
    }
    get_string(root, "bt_name", g_config.bt_name);
    get_string(root, "bt_key", g_config.bt_key);
    if (get_number(root, "internet_mode", &num) != 0U) {
        g_config.internet_mode = (uint8_t)num;
    }
    get_string(root, "gsm_apn", g_config.gsm_apn);
    get_string(root, "wifi_ssid", g_config.wifi_ssid);
    get_string(root, "wifi_pass", g_config.wifi_pass);

    /* Release everything held by the bump allocator */
    s_cjson_heap_idx = 0U;
    return 1U;
}

void config_print(void)
{
    bsp_uart_debug_printf("[INFO] Device ID      : %s\r\n", g_config.device_id);
    bsp_uart_debug_printf("[INFO] Interval       : %lu s\r\n", (unsigned long)g_config.dev_interval);
    bsp_uart_debug_printf("[INFO] Server         : %s:%u\r\n", g_config.server_address, (unsigned)g_config.server_port);
    bsp_uart_debug_printf("[INFO] Bluetooth name : %s\r\n", g_config.bt_name);
    bsp_uart_debug_printf("[INFO] Internet mode  : %s\r\n", (g_config.internet_mode == 0U) ? "GSM" : "WiFi");
    if (g_config.internet_mode == 0U) {
        bsp_uart_debug_printf("[INFO] APN            : %s\r\n", g_config.gsm_apn);
    } else {
        bsp_uart_debug_printf("[INFO] WiFi SSID      : %s\r\n", g_config.wifi_ssid);
    }
}
