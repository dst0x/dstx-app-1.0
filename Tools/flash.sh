#!/bin/bash
# Flash the firmware via OpenOCD + ST-Link V2-1
set -e

ELF="${1:-Debug/dstx-app_1.0.elf}"

if [ ! -f "$ELF" ]; then
  echo "Usage: $0 [path/to/firmware.elf]"
  exit 1
fi

openocd \
  -f interface/stlink-v2-1.cfg \
  -f target/stm32f4x.cfg \
  -c "reset_config connect_assert_srst" \
  -c "program $ELF verify reset exit"
