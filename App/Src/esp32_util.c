/**
 * @file    esp32_util.c
 * @brief   ESP32 AT command utility — WiFi, BLE, NTP, TCP.
 *          Espressif AT instruction set v3.x.
 */

#include "esp32_util.h"
#include "config.h"
#include "bsp_at.h"
#include "bsp_iwdg.h"
#include "bsp_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* UART handle — change this if ESP32 is on a different USART */
extern UART_HandleTypeDef huart6;
#define ESP32_HUART  (&huart6)

static at_dev_t       s_at;
static esp32_status_t s_status;

/* --- URC handler -------------------------------------------------------- */

static void urc_handler(const char *line)
{
    if (strcmp(line, "WIFI CONNECTED") == 0) {
        s_status.wifi_state = ESP32_WIFI_CONNECTED;
    } else if (strcmp(line, "WIFI GOT IP") == 0) {
        s_status.wifi_state = ESP32_WIFI_GOT_IP;
    } else if (strcmp(line, "WIFI DISCONNECT") == 0) {
        s_status.wifi_state = ESP32_WIFI_DISCONNECTED;
        s_status.ip_addr[0] = '\0';
        s_status.rssi = 0;
    }
}

/* --- Helpers ------------------------------------------------------------ */

static esp32_result_t at_to_esp(at_result_t rc)
{
    switch (rc) {
        case AT_OK:        return ESP32_OK;
        case AT_TIMEOUT:   return ESP32_TIMEOUT;
        default:           return ESP32_ERROR;
    }
}

static esp32_result_t send_check(const char *cmd, uint32_t timeout_ms)
{
    return at_to_esp(bsp_at_send_cmd(&s_at, cmd, timeout_ms));
}

static esp32_result_t send_resp(const char *cmd, char *resp, uint16_t sz, uint32_t timeout_ms)
{
    return at_to_esp(bsp_at_send_cmd_resp(&s_at, cmd, resp, sz, timeout_ms));
}

/* --- Init / Reset ------------------------------------------------------- */

esp32_result_t esp32_init(void)
{
    memset(&s_status, 0, sizeof(s_status));

    if (bsp_at_init(&s_at, ESP32_HUART, urc_handler) != HAL_OK) {
        return ESP32_ERROR;
    }

    /* wait for ESP32 to finish booting */
    for (uint8_t d = 0U; d < 4U; d++) {
        bsp_iwdg_refresh();
        HAL_Delay(500U);
    }

    /* sync: send AT a few times */
    for (uint8_t i = 0U; i < 5U; i++) {
        if (bsp_at_send_cmd(&s_at, "AT", 1000U) == AT_OK) {
            s_status.init_ok = 1U;

            /* disable echo */
            (void)bsp_at_send_cmd(&s_at, "ATE0", 1000U);

            bsp_uart_debug_print("[ESP32] init OK\r\n");
            return ESP32_OK;
        }
        bsp_iwdg_refresh();
        HAL_Delay(200U);
        bsp_iwdg_refresh();
    }

    bsp_uart_debug_print("[ESP32] init failed — no response\r\n");
    return ESP32_TIMEOUT;
}

esp32_result_t esp32_reset(void)
{
    esp32_result_t rc = send_check("AT+RST", ESP32_UART_TIMEOUT);
    if (rc == ESP32_OK) {
        for (uint8_t i = 0U; i < 8U; i++) {
            bsp_iwdg_refresh();
            HAL_Delay(250U);
        }
        /* re-sync after reset */
        (void)bsp_at_send_cmd(&s_at, "ATE0", 1000U);
    }
    return rc;
}

/* --- WiFi --------------------------------------------------------------- */

esp32_result_t esp32_wifi_set_mode(uint8_t mode)
{
    char cmd[24];
    (void)snprintf(cmd, sizeof(cmd), "AT+CWMODE=%u", (unsigned)mode);
    return send_check(cmd, ESP32_UART_TIMEOUT);
}

esp32_result_t esp32_wifi_connect(const char *ssid, const char *pass)
{
    char cmd[AT_TX_BUF_SIZE];
    char resp[AT_LINE_MAX];
    esp32_result_t rc;

    if (s_status.init_ok == 0U) {
        return ESP32_NOT_INIT;
    }

    /* station mode */
    rc = esp32_wifi_set_mode(1U);
    if (rc != ESP32_OK) {
        return rc;
    }

    (void)snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", ssid, pass);
    bsp_uart_debug_printf("[ESP32] connecting to %s...\r\n", ssid);

    at_result_t ar = bsp_at_send_cmd_resp(&s_at, cmd, resp, sizeof(resp), ESP32_WIFI_TIMEOUT);

    if (ar == AT_OK) {
        s_status.wifi_state = ESP32_WIFI_GOT_IP;
        for (uint8_t retry = 0U; retry < 5U; retry++) {
            bsp_iwdg_refresh();
            HAL_Delay(500U);
            if (esp32_wifi_get_ip(s_status.ip_addr, sizeof(s_status.ip_addr)) == ESP32_OK) {
                break;
            }
        }
        (void)esp32_wifi_get_rssi(&s_status.rssi);
        bsp_uart_debug_printf("[ESP32] WiFi connected, IP: %s, RSSI: %d\r\n",
                              s_status.ip_addr, (int)s_status.rssi);
        return ESP32_OK;
    }

    /* parse error reason */
    if (strstr(resp, "+CWJAP:3") != NULL) {
        bsp_uart_debug_print("[ESP32] WiFi AP not found\r\n");
        return ESP32_WIFI_NO_AP;
    }
    if (strstr(resp, "+CWJAP:4") != NULL) {
        bsp_uart_debug_print("[ESP32] WiFi wrong password\r\n");
        return ESP32_WIFI_WRONG_PASS;
    }

    bsp_uart_debug_print("[ESP32] WiFi connect failed\r\n");
    return ESP32_WIFI_FAIL;
}

esp32_result_t esp32_wifi_disconnect(void)
{
    esp32_result_t rc = send_check("AT+CWQAP", ESP32_UART_TIMEOUT);
    if (rc == ESP32_OK) {
        s_status.wifi_state = ESP32_WIFI_DISCONNECTED;
        s_status.ip_addr[0] = '\0';
    }
    return rc;
}

esp32_result_t esp32_wifi_get_ip(char *ip_buf, uint16_t buf_size)
{
    char resp[256];

    if (send_resp("AT+CIPSTA?", resp, sizeof(resp), ESP32_UART_TIMEOUT) != ESP32_OK) {
        return ESP32_ERROR;
    }

    /* try +CIPSTA:ip:"x.x.x.x" (AT v3.x) */
    char *p = strstr(resp, "+CIPSTA:ip:\"");
    if (p != NULL) {
        p += 12;
    } else {
        /* try +CIPSTA:"x.x.x.x" (older firmware) */
        p = strstr(resp, "+CIPSTA:\"");
        if (p != NULL) {
            p += 9;
        }
    }

    if (p == NULL) {
        return ESP32_ERROR;
    }

    char *end = strchr(p, '"');
    if (end == NULL || end == p) {
        return ESP32_ERROR;
    }

    uint16_t len = (uint16_t)(end - p);
    if (len >= buf_size) {
        len = buf_size - 1U;
    }
    memcpy(ip_buf, p, len);
    ip_buf[len] = '\0';

    if (strcmp(ip_buf, "0.0.0.0") == 0) {
        ip_buf[0] = '\0';
        return ESP32_ERROR;
    }

    return ESP32_OK;
}

esp32_result_t esp32_wifi_get_rssi(int8_t *rssi)
{
    char resp[128];

    if (send_resp("AT+CWJAP?", resp, sizeof(resp), ESP32_UART_TIMEOUT) != ESP32_OK) {
        return ESP32_ERROR;
    }

    /* response: +CWJAP:"ssid","bssid",ch,rssi,... */
    char *p = resp;
    uint8_t comma_count = 0U;
    while (*p != '\0' && comma_count < 3U) {
        if (*p == ',') {
            comma_count++;
        }
        p++;
    }

    if (comma_count == 3U) {
        *rssi = (int8_t)atoi(p);
        s_status.rssi = *rssi;
        return ESP32_OK;
    }
    return ESP32_ERROR;
}

esp32_wifi_state_t esp32_wifi_get_state(void)
{
    return s_status.wifi_state;
}

/* --- NTP ---------------------------------------------------------------- */

esp32_result_t esp32_ntp_config(int8_t timezone, const char *server)
{
    char cmd[128];

    if (server == NULL) {
        server = ESP32_NTP_SERVER;
    }

    (void)snprintf(cmd, sizeof(cmd), "AT+CIPSNTPCFG=1,%d,\"%s\"",
                   (int)timezone, server);
    return send_check(cmd, ESP32_NTP_TIMEOUT);
}

esp32_result_t esp32_ntp_get_time(uint16_t *year, uint8_t *month, uint8_t *day,
                                  uint8_t *hour, uint8_t *minute, uint8_t *second)
{
    char resp[128];

    if (send_resp("AT+CIPSNTPTIME?", resp, sizeof(resp), ESP32_NTP_TIMEOUT) != ESP32_OK) {
        return ESP32_NTP_FAIL;
    }

    /* response: +CIPSNTPTIME:Mon Dec 12 02:33:32 2016
     * or newer: +CIPSNTPTIME:2024-01-15 10:30:45 */

    /* try ISO format first */
    char *p = strstr(resp, "+CIPSNTPTIME:");
    if (p == NULL) {
        return ESP32_NTP_FAIL;
    }
    p += 13;

    /* try YYYY-MM-DD HH:MM:SS */
    if (p[4] == '-' && p[7] == '-') {
        *year   = (uint16_t)atoi(p);
        *month  = (uint8_t)atoi(p + 5);
        *day    = (uint8_t)atoi(p + 8);
        *hour   = (uint8_t)atoi(p + 11);
        *minute = (uint8_t)atoi(p + 14);
        *second = (uint8_t)atoi(p + 17);

        if (*year >= 2020U && *year <= 2099U) {
            return ESP32_OK;
        }
    }

    /* try legacy ctime format: "Mon Dec 12 02:33:32 2016" */
    static const char * const months[] = {
        "Jan","Feb","Mar","Apr","May","Jun",
        "Jul","Aug","Sep","Oct","Nov","Dec"
    };

    /* skip day name (3 chars + space) */
    if (strlen(p) < 20U) {
        return ESP32_NTP_FAIL;
    }
    p += 4;

    *month = 0U;
    for (uint8_t i = 0U; i < 12U; i++) {
        if (strncmp(p, months[i], 3) == 0) {
            *month = i + 1U;
            break;
        }
    }
    if (*month == 0U) {
        return ESP32_NTP_FAIL;
    }

    *day    = (uint8_t)atoi(p + 4);
    *hour   = (uint8_t)atoi(p + 7);
    *minute = (uint8_t)atoi(p + 10);
    *second = (uint8_t)atoi(p + 13);
    *year   = (uint16_t)atoi(p + 16);

    if (*year < 2020U || *year > 2099U) {
        return ESP32_NTP_FAIL;
    }

    return ESP32_OK;
}

/* --- Bluetooth LE ------------------------------------------------------- */

esp32_result_t esp32_bt_init(const char *name)
{
    char cmd[96];
    esp32_result_t rc;

    /* init BLE as server role */
    rc = send_check("AT+BLEINIT=2", ESP32_BT_TIMEOUT);
    if (rc != ESP32_OK) {
        bsp_uart_debug_print("[ESP32] BLE init failed\r\n");
        return ESP32_BT_FAIL;
    }

    /* set device name */
    (void)snprintf(cmd, sizeof(cmd), "AT+BLENAME=\"%s\"", name);
    rc = send_check(cmd, ESP32_BT_TIMEOUT);
    if (rc != ESP32_OK) {
        return ESP32_BT_FAIL;
    }

    /* create GATT service table */
    rc = send_check("AT+BLEGATTSSRVCRE", ESP32_BT_TIMEOUT);
    if (rc != ESP32_OK) {
        return ESP32_BT_FAIL;
    }

    /* start GATT service */
    rc = send_check("AT+BLEGATTSSRVSTART", ESP32_BT_TIMEOUT);
    if (rc != ESP32_OK) {
        return ESP32_BT_FAIL;
    }

    s_status.bt_active = 1U;
    bsp_uart_debug_printf("[ESP32] BLE ready: %s\r\n", name);
    return ESP32_OK;
}

esp32_result_t esp32_bt_deinit(void)
{
    esp32_result_t rc = send_check("AT+BLEINIT=0", ESP32_BT_TIMEOUT);
    s_status.bt_active = 0U;
    return rc;
}

esp32_result_t esp32_bt_advertise_start(void)
{
    return send_check("AT+BLEADVSTART", ESP32_BT_TIMEOUT);
}

esp32_result_t esp32_bt_advertise_stop(void)
{
    return send_check("AT+BLEADVSTOP", ESP32_BT_TIMEOUT);
}

esp32_result_t esp32_bt_notify(uint16_t conn_id, const uint8_t *data, uint16_t len)
{
    char cmd[64];

    /* srv_index=1, char_index=1 — first characteristic of first service */
    (void)snprintf(cmd, sizeof(cmd), "AT+BLEGATTSNTFY=%u,1,1,%u",
                   (unsigned)conn_id, (unsigned)len);

    at_result_t ar = bsp_at_send_cmd(&s_at, cmd, ESP32_BT_TIMEOUT);
    if (ar != AT_OK) {
        return ESP32_BT_FAIL;
    }

    /* wait for '>' prompt then send data */
    HAL_Delay(50U);
    ar = bsp_at_send_raw(&s_at, data, len, ESP32_BT_TIMEOUT);
    return (ar == AT_OK) ? ESP32_OK : ESP32_BT_FAIL;
}

/* --- TCP ---------------------------------------------------------------- */

esp32_result_t esp32_tcp_connect(const char *host, uint16_t port)
{
    char cmd[AT_TX_BUF_SIZE];
    (void)snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%u", host, (unsigned)port);
    return send_check(cmd, ESP32_WIFI_TIMEOUT);
}

esp32_result_t esp32_tcp_send(const uint8_t *data, uint16_t len)
{
    char cmd[32];
    (void)snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%u", (unsigned)len);

    at_result_t ar = bsp_at_send_cmd(&s_at, cmd, ESP32_UART_TIMEOUT);
    if (ar != AT_OK) {
        return ESP32_ERROR;
    }

    /* wait for '>' prompt */
    HAL_Delay(50U);
    ar = bsp_at_send_raw(&s_at, data, len, ESP32_UART_TIMEOUT);

    /* wait for SEND OK */
    char resp[64];
    at_result_t final = bsp_at_send_cmd_resp(&s_at, "", resp, sizeof(resp), ESP32_UART_TIMEOUT);
    (void)final;

    return (ar == AT_OK) ? ESP32_OK : ESP32_ERROR;
}

esp32_result_t esp32_tcp_disconnect(void)
{
    return send_check("AT+CIPCLOSE", ESP32_UART_TIMEOUT);
}

/* --- MQTT --------------------------------------------------------------- */

esp32_result_t esp32_mqtt_config(const char *client_id, const char *username, const char *password)
{
    char cmd[AT_TX_BUF_SIZE];

    /* AT+MQTTUSERCFG=<LinkID>,<scheme>,<"client_id">,<"username">,<"password">,<cert_key_ID>,<CA_ID>,<"path">
       LinkID=0, scheme=1 (MQTT over TCP) */
    (void)snprintf(cmd, sizeof(cmd),
                   "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"",
                   client_id, username, password);
    return send_check(cmd, ESP32_MQTT_TIMEOUT);
}

esp32_result_t esp32_mqtt_connect(const char *host, uint16_t port, uint8_t reconnect)
{
    char cmd[AT_TX_BUF_SIZE];
    esp32_result_t rc;

    (void)snprintf(cmd, sizeof(cmd), "AT+MQTTCONN=0,\"%s\",%u,%u",
                   host, (unsigned)port, (unsigned)reconnect);

    bsp_uart_debug_printf("[ESP32] MQTT connecting to %s:%u...\r\n", host, (unsigned)port);
    rc = send_check(cmd, ESP32_MQTT_TIMEOUT);

    if (rc == ESP32_OK) {
        s_status.mqtt_connected = 1U;
        bsp_uart_debug_print("[ESP32] MQTT connected\r\n");
    } else {
        bsp_uart_debug_print("[ESP32] MQTT connect failed\r\n");
        rc = ESP32_MQTT_FAIL;
    }
    return rc;
}

esp32_result_t esp32_mqtt_publish(const char *topic, const char *data, uint8_t qos, uint8_t retain)
{
    char cmd[AT_TX_BUF_SIZE];
    uint16_t data_len = (uint16_t)strlen(data);

    (void)snprintf(cmd, sizeof(cmd), "AT+MQTTPUBRAW=0,\"%s\",%u,%u,%u",
                   topic, (unsigned)data_len, (unsigned)qos, (unsigned)retain);

    at_result_t ar = bsp_at_send_cmd(&s_at, cmd, ESP32_MQTT_TIMEOUT);
    if (ar != AT_OK) {
        return ESP32_MQTT_FAIL;
    }

    HAL_Delay(100U);
    bsp_iwdg_refresh();

    ar = bsp_at_send_raw(&s_at, (const uint8_t *)data, data_len, ESP32_MQTT_TIMEOUT);
    if (ar != AT_OK) {
        return ESP32_MQTT_FAIL;
    }

    ar = bsp_at_wait_match(&s_at, "+MQTTPUB:OK", ESP32_MQTT_TIMEOUT);
    return (ar == AT_OK) ? ESP32_OK : ESP32_MQTT_FAIL;
}

esp32_result_t esp32_mqtt_subscribe(const char *topic, uint8_t qos)
{
    char cmd[AT_TX_BUF_SIZE];

    (void)snprintf(cmd, sizeof(cmd), "AT+MQTTSUB=0,\"%s\",%u", topic, (unsigned)qos);
    return send_check(cmd, ESP32_MQTT_TIMEOUT);
}

esp32_result_t esp32_mqtt_unsubscribe(const char *topic)
{
    char cmd[AT_TX_BUF_SIZE];

    (void)snprintf(cmd, sizeof(cmd), "AT+MQTTUNSUB=0,\"%s\"", topic);
    return send_check(cmd, ESP32_MQTT_TIMEOUT);
}

esp32_result_t esp32_mqtt_disconnect(void)
{
    esp32_result_t rc = send_check("AT+MQTTCLEAN=0", ESP32_MQTT_TIMEOUT);
    s_status.mqtt_connected = 0U;
    return rc;
}

/* --- HTTP --------------------------------------------------------------- */

esp32_result_t esp32_http_get(const char *url, char *resp, uint16_t resp_size)
{
    char cmd[AT_TX_BUF_SIZE];

    /* AT+HTTPCLIENT=<opt>,<content-type>,<"url">,[<"host">],[<"path">],<transport_type>
       opt=1 (GET), content-type=0, transport=1 (HTTP) */
    (void)snprintf(cmd, sizeof(cmd), "AT+HTTPCLIENT=1,0,\"%s\",,,1", url);

    esp32_result_t rc = send_resp(cmd, resp, resp_size, ESP32_HTTP_TIMEOUT);
    if (rc != ESP32_OK) {
        rc = ESP32_HTTP_FAIL;
    }
    return rc;
}

esp32_result_t esp32_http_post(const char *url, const char *content_type,
                               const char *body, char *resp, uint16_t resp_size)
{
    char cmd[AT_TX_BUF_SIZE];
    uint16_t body_len = (uint16_t)strlen(body);

    /* AT+HTTPCPOST=<"url">,<length>[,<header_count>,<"header">]
       For simple POST with content-type */
    (void)snprintf(cmd, sizeof(cmd), "AT+HTTPCPOST=\"%s\",%u,1,\"Content-Type: %s\"",
                   url, (unsigned)body_len, content_type);

    at_result_t ar = bsp_at_send_cmd(&s_at, cmd, ESP32_HTTP_TIMEOUT);
    if (ar != AT_OK) {
        return ESP32_HTTP_FAIL;
    }

    /* wait for '>' prompt then send body */
    HAL_Delay(100U);
    ar = bsp_at_send_raw(&s_at, (const uint8_t *)body, body_len, ESP32_HTTP_TIMEOUT);
    if (ar != AT_OK) {
        return ESP32_HTTP_FAIL;
    }

    /* wait for response */
    at_result_t final = bsp_at_send_cmd_resp(&s_at, "", resp, resp_size, ESP32_HTTP_TIMEOUT);
    return (final == AT_OK) ? ESP32_OK : ESP32_HTTP_FAIL;
}

/* --- Signal ------------------------------------------------------------- */

esp32_result_t esp32_send_signal(const char *payload)
{
    esp32_result_t rc;

    /* process pending URCs so wifi_state is up to date */
    esp32_process();

    /* reconnect WiFi if needed */
    if (s_status.wifi_state != ESP32_WIFI_GOT_IP) {
        bsp_uart_debug_print("[ESP32] signal: WiFi lost, reconnecting...\r\n");
        rc = esp32_wifi_connect(g_config.wifi_ssid, g_config.wifi_pass);
        if (rc != ESP32_OK) {
            return ESP32_WIFI_FAIL;
        }
    }

    /* configure MQTT credentials if not connected */
    if (s_status.mqtt_connected == 0U) {
        (void)send_check("AT+MQTTCLEAN=0", 1000U);
        bsp_iwdg_refresh();
        HAL_Delay(500U);

        rc = esp32_mqtt_config(g_config.devid, g_config.mqtt_user, g_config.mqtt_pass);
        if (rc != ESP32_OK) {
            bsp_uart_debug_print("[ESP32] signal: MQTT config failed, resetting...\r\n");
            (void)esp32_reset();
            bsp_iwdg_refresh();
            rc = esp32_wifi_connect(g_config.wifi_ssid, g_config.wifi_pass);
            if (rc != ESP32_OK) {
                return ESP32_WIFI_FAIL;
            }
            rc = esp32_mqtt_config(g_config.devid, g_config.mqtt_user, g_config.mqtt_pass);
            if (rc != ESP32_OK) {
                return ESP32_MQTT_FAIL;
            }
        }

        rc = esp32_mqtt_connect(g_config.server, g_config.server_port, 1U);
        if (rc != ESP32_OK) {
            return ESP32_MQTT_FAIL;
        }
    }

    static uint8_t rssi_cnt = 0U;
    if (++rssi_cnt >= 10U) {
        rssi_cnt = 0U;
        (void)esp32_wifi_get_rssi(&s_status.rssi);
    }

    rc = esp32_mqtt_publish(g_config.mqtt_topic, payload, 1U, 0U);
    if (rc != ESP32_OK) {
        bsp_uart_debug_print("[ESP32] signal: publish failed\r\n");
        s_status.mqtt_connected = 0U;
        return ESP32_MQTT_FAIL;
    }

    return ESP32_OK;
}

/* --- Status ------------------------------------------------------------- */

const esp32_status_t *esp32_get_status(void)
{
    return &s_status;
}

const char *esp32_result_str(esp32_result_t result)
{
    switch (result) {
        case ESP32_OK:              return "OK";
        case ESP32_ERROR:           return "ERROR";
        case ESP32_TIMEOUT:         return "TIMEOUT";
        case ESP32_NOT_INIT:        return "NOT_INIT";
        case ESP32_WIFI_FAIL:       return "WIFI_FAIL";
        case ESP32_WIFI_NO_AP:      return "WIFI_NO_AP";
        case ESP32_WIFI_WRONG_PASS: return "WIFI_WRONG_PASS";
        case ESP32_NTP_FAIL:        return "NTP_FAIL";
        case ESP32_BT_FAIL:        return "BT_FAIL";
        case ESP32_MQTT_FAIL:      return "MQTT_FAIL";
        case ESP32_HTTP_FAIL:      return "HTTP_FAIL";
        default:                    return "UNKNOWN";
    }
}

void esp32_process(void)
{
    bsp_at_process(&s_at);
}
