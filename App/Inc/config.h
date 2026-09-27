/**
 * @file    config.h
 * @brief   Application constants, Modbus addresses, intervals and the
 *          runtime configuration loaded from config.json on the SD card.
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* Application timing -------------------------------------------------------*/
#define APP_LOOP_PERIOD_MS          (100U)   /**< Main loop period (< IWDG timeout ~512 ms) */
#define APP_HEARTBEAT_PERIOD_MS     (1000U)  /**< LED toggle + "Hello World" period */

/* Modbus RTU master (USART1 / RS-485) --------------------------------------*/
#define MODBUS_RESPONSE_TIMEOUT_MS  (200U)   /**< Wait for a slave reply */
#define MODBUS_TX_TIMEOUT_MS        (100U)   /**< Wait for the request to leave the bus */
#define MODBUS_MAX_REGISTERS        (32U)    /**< Max registers per read request */

/* Sensors ------------------------------------------------------------------*/
typedef struct {
    const char *name;         /**< Name used in logs and the CSV header */
    uint8_t     enabled;      /**< 0 = skipped by the sensor manager */
    uint8_t     slave_addr;   /**< Modbus slave address (1..247) */
    uint8_t     function;     /**< 3 = holding registers, 4 = input registers */
    uint16_t    reg_addr;     /**< First register (0-based protocol address) */
    float       scale;        /**< value = raw * scale + offset */
    float       offset;
} sensor_def_t;

/** Sensor table, defined in config.c. See Docs/modbus_register_map.md. */
extern const sensor_def_t g_sensor_defs[];
extern const uint8_t     g_sensor_count;

/* Data logger --------------------------------------------------------------*/
#define DATALOG_FILENAME            "datalog.csv"

/* Safety -------------------------------------------------------------------*/
#define SAFETY_PVD_LEVEL            PWR_PVDLEVEL_6   /**< Brown-out warning at ~2.9 V */

/* Runtime configuration (config.json) --------------------------------------*/
#define CONFIG_STR_MAX  (64U)
#define CONFIG_FILENAME "config.json"
#define CONFIG_FILE_BUF (1024U)

typedef struct {
    char     device_id[CONFIG_STR_MAX];      /**< Device ID, e.g. "DST0X-001" */
    uint32_t dev_interval;                   /**< Data send / log interval (seconds) */
    char     server_address[CONFIG_STR_MAX]; /**< Server address */
    uint16_t server_port;                    /**< Server port */
    char     bt_name[CONFIG_STR_MAX];        /**< Bluetooth name */
    char     bt_key[CONFIG_STR_MAX];         /**< Bluetooth key */
    uint8_t  internet_mode;                  /**< 0 = GSM, 1 = WiFi */
    char     gsm_apn[CONFIG_STR_MAX];        /**< GSM APN */
    char     wifi_ssid[CONFIG_STR_MAX];      /**< WiFi SSID */
    char     wifi_pass[CONFIG_STR_MAX];      /**< WiFi password */
} config_t;

extern config_t g_config;

/**
 * @brief  Reads config.json from the (mounted) SD card, prints its raw
 *         content and parses it into g_config. Missing keys keep defaults.
 * @retval 1 on success, 0 on failure
 */
uint8_t config_load(void);

/** @brief Prints the parsed configuration on the debug UART. */
void config_print(void);

#endif /* CONFIG_H */
