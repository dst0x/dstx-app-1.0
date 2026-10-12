/**
 * @file    safety.c
 * @author  DST0x Developer
 * @date    2026-09-27
 */

#include "safety.h"
#include "config.h"
#include "main.h"
#include "bsp_gpio.h"
#include "bsp_iwdg.h"

static volatile uint8_t s_faults = SAFETY_FAULT_NONE;

void safety_init(void)
{
    PWR_PVDTypeDef pvd = {0};

    bsp_iwdg_init();
    if (bsp_iwdg_was_reset() != 0U) {
        s_faults |= SAFETY_FAULT_IWDG_RESET;
    }

    pvd.PVDLevel = SAFETY_PVD_LEVEL;
    pvd.Mode = PWR_PVD_MODE_IT_RISING_FALLING;
    (void)HAL_PWR_ConfigPVD(&pvd);

    /* clear any PVD flag left from the power-up ramp before enabling the interrupt */
    __HAL_PWR_PVD_EXTI_CLEAR_FLAG();
    HAL_PWR_EnablePVD();

    /* if the voltage is already stable, ignore a stale PVDO flag */
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_PVDO) == 0U) {
        s_faults &= (uint8_t)~SAFETY_FAULT_PVD;
    }
}

void safety_service(void)
{
    bsp_iwdg_refresh();

    if (bsp_estop_is_active() != 0U) {
        s_faults |= SAFETY_FAULT_ESTOP;
    }
}

uint8_t safety_get_faults(void)
{
    return s_faults;
}

uint8_t safety_is_tripped(void)
{
    return ((s_faults & (SAFETY_FAULT_CSS | SAFETY_FAULT_ESTOP)) != 0U) ? 1U : 0U;
}

void safety_clear_faults(void)
{
    uint8_t keep = (uint8_t)(s_faults & SAFETY_FAULT_CSS); /* CSS stays until reset */

    if (bsp_estop_is_active() != 0U) {
        keep |= SAFETY_FAULT_ESTOP;
    }
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_PVDO) != 0U) {
        keep |= SAFETY_FAULT_PVD;
    }
    s_faults = keep;
}

void HAL_RCC_CSSCallback(void)
{
    s_faults |= SAFETY_FAULT_CSS;
}

void HAL_PWR_PVDCallback(void)
{
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_PVDO) != 0U) {
        s_faults |= SAFETY_FAULT_PVD;
    }
}
