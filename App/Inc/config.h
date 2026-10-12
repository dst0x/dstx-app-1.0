/*
 * config.h
 *
 *  Created on: Sep 27, 2026
 *      Author: DSTX Labs
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

#define APP_VERSION                 "1.0.0"

#define APP_LOOP_PERIOD_MS          (100U)   
#define APP_HEARTBEAT_PERIOD_MS     (1000U)  

#define MODBUS_RESPONSE_TIMEOUT_MS  (200U)   
#define MODBUS_TX_TIMEOUT_MS        (100U)   
#define MODBUS_MAX_REGISTERS        (32U)    

#define DATALOG_FILENAME            "datalog.csv"
#define SAFETY_PVD_LEVEL            PWR_PVDLEVEL_2  

#define CONFIG_STR_MAX  (64U)
#define CONFIG_FILENAME "config.json"
#define CONFIG_FILE_BUF (1024U)

typedef struct {
    const char *name;     
    uint8_t     enabled;    
    uint8_t     slave_addr; 
    uint8_t     function;     
    uint16_t    reg_addr;    
    float       scale;      
    float       offset;
} sensor_def_t;

extern const sensor_def_t g_sensor_defs[];
extern const uint8_t     g_sensor_count;

typedef struct {
    char     devid[CONFIG_STR_MAX];          /**< Device ID, e.g. "DST0X-001" */
    uint32_t dev_inv;                        /**< Data send / log interval (seconds) */
    char     server[CONFIG_STR_MAX];         /**< Server address */
    uint16_t server_port;                    /**< Server port */
    char     bt_name[CONFIG_STR_MAX];        /**< Bluetooth name */
    char     bt_key[CONFIG_STR_MAX];         /**< Bluetooth key */
    char     protocol_mode[CONFIG_STR_MAX];  /**< Protocol: "mqtt", "http", "tcp" */
    uint8_t  inet_mode;                      /**< 0 = GSM, 1 = WiFi */
    char     gsm_apn[CONFIG_STR_MAX];        /**< GSM APN */
    char     wifi_ssid[CONFIG_STR_MAX];      /**< WiFi SSID */
    char     wifi_pass[CONFIG_STR_MAX];      /**< WiFi password */
    char     mqtt_topic[CONFIG_STR_MAX];     /**< MQTT publish topic */
    char     mqtt_user[CONFIG_STR_MAX];      /**< MQTT username */
    char     mqtt_pass[CONFIG_STR_MAX];      /**< MQTT password */
    int8_t   ntp_timezone;                   /**< NTP timezone offset (hours) */
} config_t;

extern config_t g_config;


uint8_t config_load(void);
void config_print(void);

#endif /* CONFIG_H */
