/**
 * @file    bsp_sdcard.c
 * @brief   SDIO + FatFs board support and FatFs disk I/O layer.
 *
 * Transfers use HAL polling mode. SDIO_CK = 48 MHz / (ClockDiv + 2).
 */

#include "bsp_sdcard.h"
#include "ff.h"
#include "diskio.h"

#define DEV_SD         (0U)
#define SD_TIMEOUT_MS  (1000U)

extern SD_HandleTypeDef hsd;

static FATFS   s_fs;
static FIL     s_file;
static uint8_t s_mounted = 0U;

/* Public API ---------------------------------------------------------------*/

HAL_StatusTypeDef bsp_sd_init(void)
{
  /* Same settings as MX_SDIO_SD_Init() in main.c */
  hsd.Instance = SDIO;
  hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
  hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
  hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
  hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
  hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd.Init.ClockDiv = 4;

  if (HAL_SD_Init(&hsd) != HAL_OK)
  {
    return HAL_ERROR;
  }
  return HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_4B);
}

HAL_StatusTypeDef bsp_sd_mount(void)
{
  if (f_mount(&s_fs, "", 1) != FR_OK)
  {
    s_mounted = 0U;
    return HAL_ERROR;
  }
  s_mounted = 1U;
  return HAL_OK;
}

HAL_StatusTypeDef bsp_sd_read_file(const char *path, char *buf, uint32_t buf_size, uint32_t *read_len)
{
  UINT    br = 0U;
  FRESULT fr;

  if ((s_mounted == 0U) || (buf_size == 0U))
  {
    return HAL_ERROR;
  }
  if (f_open(&s_file, path, FA_READ) != FR_OK)
  {
    return HAL_ERROR;
  }
  fr = f_read(&s_file, buf, (UINT)(buf_size - 1U), &br);
  (void)f_close(&s_file);

  buf[br] = '\0';
  if (read_len != NULL)
  {
    *read_len = (uint32_t)br;
  }
  return (fr == FR_OK) ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef bsp_sd_append_file(const char *path, const void *data, uint32_t len)
{
  UINT    bw = 0U;
  FRESULT fr;

  if (s_mounted == 0U)
  {
    return HAL_ERROR;
  }
  if (f_open(&s_file, path, FA_WRITE | FA_OPEN_APPEND) != FR_OK)
  {
    return HAL_ERROR;
  }
  fr = f_write(&s_file, data, (UINT)len, &bw);
  if (f_close(&s_file) != FR_OK)
  {
    fr = FR_DISK_ERR;
  }
  return ((fr == FR_OK) && (bw == (UINT)len)) ? HAL_OK : HAL_ERROR;
}

/* FatFs disk I/O layer -----------------------------------------------------*/

static DRESULT sd_wait_ready(void)
{
  uint32_t start = HAL_GetTick();

  while (HAL_SD_GetCardState(&hsd) != HAL_SD_CARD_TRANSFER)
  {
    if ((HAL_GetTick() - start) > SD_TIMEOUT_MS)
    {
      return RES_ERROR;
    }
  }
  return RES_OK;
}

DSTATUS disk_status(BYTE pdrv)
{
  if (pdrv != (BYTE)DEV_SD)
  {
    return STA_NOINIT;
  }
  return (hsd.State == HAL_SD_STATE_RESET) ? STA_NOINIT : (DSTATUS)0;
}

DSTATUS disk_initialize(BYTE pdrv)
{
  /* The card is initialised by bsp_sd_init() */
  return disk_status(pdrv);
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
  if ((pdrv != (BYTE)DEV_SD) || (buff == NULL))
  {
    return RES_PARERR;
  }
  if (HAL_SD_ReadBlocks(&hsd, buff, (uint32_t)sector, (uint32_t)count, SD_TIMEOUT_MS) != HAL_OK)
  {
    return RES_ERROR;
  }
  return sd_wait_ready();
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
  if ((pdrv != (BYTE)DEV_SD) || (buff == NULL))
  {
    return RES_PARERR;
  }
  if (HAL_SD_WriteBlocks(&hsd, (uint8_t *)buff, (uint32_t)sector, (uint32_t)count, SD_TIMEOUT_MS) != HAL_OK)
  {
    return RES_ERROR;
  }
  return sd_wait_ready();
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
  HAL_SD_CardInfoTypeDef info;

  if (pdrv != (BYTE)DEV_SD)
  {
    return RES_PARERR;
  }

  switch (cmd)
  {
    case CTRL_SYNC:
      return sd_wait_ready();

    case GET_SECTOR_COUNT:
      (void)HAL_SD_GetCardInfo(&hsd, &info);
      *(LBA_t *)buff = (LBA_t)info.LogBlockNbr;
      return RES_OK;

    case GET_SECTOR_SIZE:
      (void)HAL_SD_GetCardInfo(&hsd, &info);
      *(WORD *)buff = (WORD)info.LogBlockSize;
      return RES_OK;

    case GET_BLOCK_SIZE:
      *(DWORD *)buff = 1U; /* Erase block size unknown */
      return RES_OK;

    default:
      return RES_PARERR;
  }
}

DWORD get_fattime(void)
{
  /* TODO: use bsp_rtc_get_time() once the RTC is enabled in CubeMX.
     Until then: fixed timestamp 2026-01-01 00:00:00 */
  return ((DWORD)(2026U - 1980U) << 25) | ((DWORD)1U << 21) | ((DWORD)1U << 16);
}
