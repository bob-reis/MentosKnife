#!/usr/bin/env python3
"""provision-serial.py — provisiona o dispositivo via console serial.

Lê secrets.local.env (ignorado pelo git) e envia os comandos de
provisionamento ao MentosArmyKnife-C3 pela USB Serial/JTAG. Nenhum segredo é
gravado no binário nem no repositório.

Uso:
    python3 scripts/provision-serial.py [--port /dev/ttyACM0] [--no-start]

Requer: pyserial  (pip install pyserial)
"""
import argparse
import os
import sys
import time

try:
    import serial  # type: ignore
    from serial.tools import list_ports  # type: ignore
except ImportError:
    sys.exit("pyserial não encontrado. Rode: pip install pyserial")

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ENV_FILE = os.path.join(HERE, "secrets.local.env")


def load_env(path):
    env = {}
    if not os.path.exists(path):
        sys.exit(f"Arquivo de segredos não encontrado: {path}")
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            k, v = line.split("=", 1)
            env[k.strip()] = v.strip()
    return env


def autodetect_port():
    for p in list_ports.comports():
        if any(t in (p.description or "").lower() for t in ("jtag", "usb", "acm", "cp210", "ch340")):
            return p.device
    ports = list(list_ports.comports())
    return ports[0].device if ports else None


def send(ser, line, delay=0.4):
    print(f">> {line.split()[0]} ...")
    ser.write((line + "\n").encode())
    ser.flush()
    time.sleep(delay)
    out = ser.read(ser.in_waiting or 1).decode(errors="replace")
    for ln in out.splitlines():
        if ln.strip() and not ln.startswith("mak>"):
            print("   " + ln.strip())


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default=None)
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--no-start", action="store_true", help="não sobe a rede ao final")
    args = ap.parse_args()

    env = load_env(ENV_FILE)
    port = args.port or autodetect_port()
    if not port:
        sys.exit("Nenhuma porta serial encontrada. Use --port.")

    op = env.get("MAK_OPERATOR_PASSWORD", "")
    hook = env.get("MAK_DISCORD_WEBHOOK", "")
    up_ssid = env.get("MAK_UPLINK_SSID", "")
    up_pw = env.get("MAK_UPLINK_PASSWORD", "")

    # O AP de visitantes é aberto (sem senha de rádio); só a senha do operador
    # é provisionada.
    if not op:
        sys.exit("Defina MAK_OPERATOR_PASSWORD em secrets.local.env")

    print(f"Conectando em {port} @ {args.baud}...")
    with serial.Serial(port, args.baud, timeout=1) as ser:
        time.sleep(0.5)
        send(ser, f"set-op-pass {op}")
        if hook:
            send(ser, f"set-discord-url {hook}")
            if up_ssid:
                send(ser, f"set-uplink {up_ssid} {up_pw}")
            send(ser, "discord on")
        if not args.no_start:
            send(ser, "start", delay=1.0)
        send(ser, "status", delay=0.6)
    print("Provisionamento concluído.")


if __name__ == "__main__":
    main()
