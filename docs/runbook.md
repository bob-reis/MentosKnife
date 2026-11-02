# Runbook — gravação, provisionamento e apresentação

> O roteiro abaixo é histórico. Para as ferramentas de rede e exportação,
> siga `network-tools.md`. STOP preserva os dados; use Limpar explicitamente.

## 1. Build e gravação

```bash
# testes de host + build
scripts/smoke-test.sh

# confirmar porta e chip
pio device list
esptool.py --port /dev/ttyACM0 flash_id

# gravar firmware + LittleFS
scripts/flash.sh /dev/ttyACM0
```

## 2. Provisionamento (uma vez, por serial)

Edite `secrets.local.env` (ignorado pelo git) e rode:

```bash
pip install pyserial
python3 scripts/provision-serial.py --port /dev/ttyACM0
```

Ou manualmente no monitor (`pio device monitor -b 115200`):

```
set-op-pass SuaSenhaForte2026!
# O AP de visitantes é aberto (sem senha de rádio) — nada a provisionar aqui.
set-discord-url https://discord.com/api/webhooks/...   # opcional
set-uplink HOTSPOT_DEMO senha-hotspot                  # opcional (não use a rede do cliente)
discord on                                             # opcional
start
status
```

## 3. Apresentação (10 min)

| Tempo | Ação |
|---|---|
| T-20 | Ligar; `status`; checar AP, URL alternativa, bateria e telefone de teste. Se usar Discord: `discord test` e confirmar o feed projetado. |
| 00:00 | Mostrar a Mentos sem revelar o circuito; contextualizar. |
| 01:30 | Mostrar SSID `MENTOS-TREINAMENTO`; conectar o telefone de teste. |
| 03:00 | Abrir o portal (pop-up **ou** URL manual `http://192.168.4.1/portal`); tocar **Simular envio**. |
| 04:30 | Painel/Discord exibem **Interação de demonstração detectada** (sem dado sensível). |
| 05:30 | Ativar **revelação** no painel; o portal revela e a caixinha é aberta. |
| 07:00 | Discutir controles defensivos (validar SSID/cert, MFA resistente a phishing, reporte). |
| 09:30 | **STOP** (painel ou `stop`); exportar resultados, usar Limpar e desligar. |

**Plano B:** se o captive portal não abrir, use a URL/IP local; se o Wi-Fi
falhar, mostre a gravação do ensaio e o hardware físico. Versão estável guardada
offline no notebook.

## 4. Recuperação / rollback

- Regravar a release congelada: `release/<versao>/firmware.bin` +
  `littlefs.bin` (ver `SHA256SUMS`).
- `wipe-secrets` no console apaga credenciais (factory reset parcial).
- Em `ERROR_SAFE`: a simulação fica desativada; reinicie por USB.
