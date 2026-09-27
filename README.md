# DST0X App 1.0

STM32F412RETx application with ThreadX RTOS for industrial sensor data logging.

## Features
- Modbus RTU master (RS-485) sensor polling
- CSV data logging to micro SD card (FatFs)
- Runtime configuration via `config.json` on the SD card
- Safety supervision: CSS, PVD, IWDG, emergency stop
- Debug output via USART2 (ST-Link VCP, 115200 baud)

## Project Structure
```
App/            Application logic (state machine, config, Modbus, datalogger, safety)
Core/           CubeMX-generated HAL init and ThreadX entry point
Drivers/BSP/    Board support (UART, SD card, GPIO, IWDG, RTC, ADC)
Drivers/        STM32 HAL and CMSIS (CubeMX-managed)
Middlewares/    ThreadX (ST), FatFs (ff15), cJSON
FATFS/          FatFs configuration (ffconf.h)
Docs/           Pinout, register map, hardware notes
Tools/          Flash and serial monitor scripts
```

## Build
Open in STM32CubeIDE and build the Debug or Release configuration.

## Flash
```bash
Tools/flash.sh            # defaults to Debug/dstx-app_1.0.elf
Tools/flash.sh Release/dstx-app_1.0.elf
```

## Serial Monitor
```bash
Tools/serial_monitor.sh            # defaults to /dev/ttyACM0
Tools/serial_monitor.sh /dev/ttyACM1
```
