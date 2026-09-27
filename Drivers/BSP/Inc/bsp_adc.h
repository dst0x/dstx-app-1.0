/**
 * @file    bsp_adc.h
 * @brief   ADC board support: continuous scan of BSP_ADC_CHANNELS channels
 *          into a DMA circular buffer.
 *
 * The ADC is not enabled in the .ioc yet. Enable ADC1 in STM32CubeMX with
 * scan + continuous conversion, DMA circular mode (half word), and set
 * BSP_ADC_CHANNELS to the number of ranks. Until then every function
 * returns HAL_ERROR.
 */
#ifndef BSP_ADC_H
#define BSP_ADC_H

#include "main.h"
#include <stdint.h>

#ifndef BSP_ADC_CHANNELS
#define BSP_ADC_CHANNELS  (1U)
#endif

/** @brief Starts continuous DMA sampling. */
HAL_StatusTypeDef BSP_ADC_Start(void);

/** @brief Stops sampling. */
HAL_StatusTypeDef BSP_ADC_Stop(void);

/**
 * @brief  Latest raw 12-bit sample of a channel (rank order in CubeMX).
 * @param  channel  0 .. BSP_ADC_CHANNELS - 1
 */
HAL_StatusTypeDef bsp_adc_read(uint8_t channel, uint16_t *raw);

/** @brief Converts a raw 12-bit sample to millivolts (VREF = 3300 mV). */
uint32_t BSP_ADC_ToMillivolts(uint16_t raw);

#endif /* BSP_ADC_H */
