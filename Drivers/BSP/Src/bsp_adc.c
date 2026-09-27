/**
 * @file    bsp_adc.c
 * @brief   ADC + DMA board support.
 */

#include "bsp_adc.h"

#define ADC_VREF_MV   (3300U)
#define ADC_FULLSCALE (4095U)

uint32_t BSP_ADC_ToMillivolts(uint16_t raw)
{
  return ((uint32_t)raw * ADC_VREF_MV) / ADC_FULLSCALE;
}

#ifdef HAL_ADC_MODULE_ENABLED

extern ADC_HandleTypeDef hadc1;

static volatile uint16_t s_samples[BSP_ADC_CHANNELS];

HAL_StatusTypeDef BSP_ADC_Start(void)
{
  return HAL_ADC_Start_DMA(&hadc1, (uint32_t *)s_samples, BSP_ADC_CHANNELS);
}

HAL_StatusTypeDef BSP_ADC_Stop(void)
{
  return HAL_ADC_Stop_DMA(&hadc1);
}

HAL_StatusTypeDef bsp_adc_read(uint8_t channel, uint16_t *raw)
{
  if (channel >= BSP_ADC_CHANNELS)
  {
    return HAL_ERROR;
  }
  *raw = s_samples[channel];
  return HAL_OK;
}

#else /* ADC not enabled in CubeMX */

HAL_StatusTypeDef BSP_ADC_Start(void)
{
  return HAL_ERROR;
}

HAL_StatusTypeDef BSP_ADC_Stop(void)
{
  return HAL_ERROR;
}

HAL_StatusTypeDef bsp_adc_read(uint8_t channel, uint16_t *raw)
{
  (void)channel;
  (void)raw;
  return HAL_ERROR;
}

#endif /* HAL_ADC_MODULE_ENABLED */
