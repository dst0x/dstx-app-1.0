/**
 * @file    bsp_sdcard.h
 * @brief   micro SD card board support: SDIO (4-bit, PC8-PC12/PD2) + FatFs.
 *
 * Also provides the FatFs disk I/O layer (disk_read, disk_write, ...).
 * FatFs is built with FF_FS_REENTRANT = 0, so every function here must be
 * called from the same thread.
 */
#ifndef BSP_SDCARD_H
#define BSP_SDCARD_H

#include "main.h"
#include <stdint.h>

/**
 * @brief  Initialises the SDIO peripheral and the card (4-bit bus).
 *         Unlike MX_SDIO_SD_Init() it returns an error instead of halting.
 */
HAL_StatusTypeDef bsp_sd_init(void);

/** @brief Mounts the FAT volume. Requires bsp_sd_init() to have succeeded. */
HAL_StatusTypeDef bsp_sd_mount(void);

/**
 * @brief  Reads a whole file into buf and null-terminates it.
 * @param  path     file name, e.g. "config.json"
 * @param  buf      destination buffer
 * @param  buf_size buffer size (one byte is kept for the terminator)
 * @param  read_len number of bytes read (may be NULL)
 */
HAL_StatusTypeDef bsp_sd_read_file(const char *path, char *buf, uint32_t buf_size, uint32_t *read_len);

/** @brief Appends len bytes to a file, creating it if needed. */
HAL_StatusTypeDef bsp_sd_append_file(const char *path, const void *data, uint32_t len);

/** @brief Deletes a file. Returns HAL_OK even if the file did not exist. */
HAL_StatusTypeDef bsp_sd_delete_file(const char *path);

/** @brief Opens a file for sequential line-by-line reading. */
HAL_StatusTypeDef bsp_sd_open_read(const char *path);

/** @brief Reads one line (up to size-1 chars). Returns 1 if a line was read, 0 at EOF/error. */
uint8_t bsp_sd_read_line(char *buf, uint16_t size);

/** @brief Closes a file opened with bsp_sd_open_read. */
void bsp_sd_close(void);

#endif /* BSP_SDCARD_H */
