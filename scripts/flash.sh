#!/usr/bin/env bash
# flash.sh — compila, grava o firmware e envia o LittleFS (data/) para o C3.
# Uso: scripts/flash.sh [porta]   (ex.: scripts/flash.sh /dev/ttyACM0)
set -euo pipefail

ENV="mentos-c3"
PORT="${1:-}"
PORT_ARG=()
[ -n "$PORT" ] && PORT_ARG=(--upload-port "$PORT")

echo ">> Confirme o chip/flash antes de gravar (P0-01):"
echo "   esptool.py flash_id"
echo

echo ">> Build do firmware..."
pio run -e "$ENV"

echo ">> Gravando firmware..."
pio run -e "$ENV" -t upload "${PORT_ARG[@]}"

echo ">> Enviando LittleFS (data/)..."
pio run -e "$ENV" -t uploadfs "${PORT_ARG[@]}"

echo ">> OK. Abra o monitor: pio device monitor -b 115200"
