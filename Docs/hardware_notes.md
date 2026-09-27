# Hardware Notes

## Board
- MCU: STM32F412RETx (LQFP64)
- Crystal: 8 MHz HSE
- Clock: SYSCLK 100 MHz (PLLM=4, PLLN=100, PLLP=2), APB1=50 MHz, APB2=100 MHz

## HAL Timebase
SysTick is used by ThreadX. HAL timebase runs on TIM6 (1 kHz).

## Watchdog
IWDG: LSI 32 kHz, prescaler /4, reload 4095 => ~512 ms timeout.
The application must call `BSP_IWDG_Refresh()` at least every 400 ms.

## Serial Monitor
- Port: `/dev/ttyACM0` (or `ttyACM1` after ST-Link re-enumeration)
- Baud: 115200
- Handshake: **None** (RTS/CTS must be disabled; moserial default blocks output)

## SD Card
- SDIO 4-bit mode, ClockDiv=4
- Pull-ups required on CMD, D0-D3 (enabled in CubeMX GPIO config)
- FatFs ff15, code page 437
