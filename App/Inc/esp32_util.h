/*
 * esp32_util.h
 *
 *  Created on: Sep 27, 2026
 *      Author: DSTX Labs
 */

#ifndef ESP32_UTIL_H
#define ESP32_UTIL_H

#include <stdint.h>

#define ESP32_UART_TIMEOUT      (5000U)
#define ESP32_WIFI_TIMEOUT      (15000U)
#define ESP32_NTP_TIMEOUT       (10000U)
#define ESP32_BT_TIMEOUT        (5000U)
#define ESP32_MQTT_TIMEOUT      (10000U)
#define ESP32_HTTP_TIMEOUT      (15000U)
#define ESP32_RETRY_COUNT       (3U)
#define ESP32_NTP_SERVER        "pool.ntp.org"
#define ESP32_MQTT_PAYLOAD_MAX  (512U)

typedef enum {
    ESP32_OK = 0,
    ESP32_ERROR,
    ESP32_TIMEOUT,
    ESP32_NOT_INIT,
    ESP32_WIFI_FAIL,
    ESP32_WIFI_NO_AP,
    ESP32_WIFI_WRONG_PASS,
    ESP32_NTP_FAIL,
    ESP32_BT_FAIL,
    ESP32_MQTT_FAIL,
    ESP32_HTTP_FAIL
} esp32_result_t;

typedef enum {
    ESP32_WIFI_DISCONNECTED = 0,
    ESP32_WIFI_CONNECTED,
    ESP32_WIFI_GOT_IP
} esp32_wifi_state_t;

typedef struct {
    uint8_t  init_ok;
    esp32_wifi_state_t wifi_state;
    char     ip_addr[16];
    int8_t   rssi;
    uint8_t  bt_active;
    uint8_t  mqtt_connected;
} esp32_status_t;

esp32_result_t esp32_init(void);
esp32_result_t esp32_reset(void);

/* WiFi */
esp32_result_t esp32_wifi_set_mode(uint8_t mode);
esp32_result_t esp32_wifi_connect(const char *ssid, const char *pass);
esp32_result_t esp32_wifi_disconnect(void);
esp32_result_t esp32_wifi_get_ip(char *ip_buf, uint16_t buf_size);
esp32_result_t esp32_wifi_get_rssi(int8_t *rssi);
esp32_wifi_state_t esp32_wifi_get_state(void);

/* NTP */
esp32_result_t esp32_ntp_config(int8_t timezone, const char *server);
esp32_result_t esp32_ntp_get_time(uint16_t *year, uint8_t *month, uint8_t *day,
                                  uint8_t *hour, uint8_t *minute, uint8_t *second);

/* Bluetooth LE */
esp32_result_t esp32_bt_init(const char *name);
esp32_result_t esp32_bt_deinit(void);
esp32_result_t esp32_bt_advertise_start(void);
esp32_result_t esp32_bt_advertise_stop(void);
esp32_result_t esp32_bt_notify(uint16_t conn_id, const uint8_t *data, uint16_t len);

/* TCP client */
esp32_result_t esp32_tcp_connect(const char *host, uint16_t port);
esp32_result_t esp32_tcp_send(const uint8_t *data, uint16_t len);
esp32_result_t esp32_tcp_disconnect(void);

/* MQTT */
esp32_result_t esp32_mqtt_config(const char *client_id, const char *username, const char *password);
esp32_result_t esp32_mqtt_connect(const char *host, uint16_t port, uint8_t reconnect);
esp32_result_t esp32_mqtt_publish(const char *topic, const char *data, uint8_t qos, uint8_t retain);
esp32_result_t esp32_mqtt_subscribe(const char *topic, uint8_t qos);
esp32_result_t esp32_mqtt_unsubscribe(const char *topic);
esp32_result_t esp32_mqtt_disconnect(void);

/* HTTP */
esp32_result_t esp32_http_get(const char *url, char *resp, uint16_t resp_size);
esp32_result_t esp32_http_post(const char *url, const char *content_type,
                               const char *body, char *resp, uint16_t resp_size);

/* Signal — kirim data ke dstxlabs/signal */
esp32_result_t esp32_send_signal(const char *payload);

/* Status & diagnostics */
const esp32_status_t *esp32_get_status(void);
const char           *esp32_result_str(esp32_result_t result);
void                  esp32_process(void);

#endif /* ESP32_UTIL_H */
