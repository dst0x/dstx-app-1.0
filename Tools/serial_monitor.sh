#!/bin/bash
# Open a serial monitor on the ST-Link VCP
set -e

PORT="${1:-/dev/ttyACM0}"
BAUD="${2:-115200}"

if ! command -v picocom &>/dev/null; then
  echo "picocom not found. Install: sudo apt install picocom"
  exit 1
fi

echo "Connecting to $PORT at $BAUD baud (Ctrl-A Ctrl-X to exit)"
picocom -b "$BAUD" --flow none "$PORT"
