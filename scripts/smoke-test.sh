#!/usr/bin/env bash
# smoke-test.sh — compila o firmware e roda os testes de host (sem hardware).
set -euo pipefail

echo ">> Compilando firmware (mentos-c3)..."
pio run -e mentos-c3

echo ">> Rodando testes unitários no host (native)..."
pio test -e native

bash scripts/test-auth-host.sh

echo ">> Smoke test concluído."
