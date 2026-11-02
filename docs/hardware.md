# Hardware / gabinete Mentos

## Alvo

- **ESP32-C3 SuperMini** (RISC-V, Wi-Fi + BLE, USB Serial/JTAG nativo).
- Gabinete: caixinha **Mentos Kiss**, interruptor, espaçadores isolantes,
  cabo USB acessível, (fase 2) LiPo com proteção + módulo de recarga + regulador.

> **USB do C3 é Serial/JTAG**, não HID genérico. BadUSB/HID/MSC estão fora do
> MVP; alvo futuro: S3 ou RP2040 (ver backlog P2).

## Antes de gravar (P0-01)

```bash
pio device list
esptool.py --port /dev/ttyACM0 flash_id   # confirmar chip e capacidade de flash
```

- Confirmar que o `board` do `platformio.ini` casa com o mapa de pinos/flash da
  SuperMini. O board genérico `esp32-c3-devkitm-1` é ponto de partida; ajustar
  `partitions.csv` se a flash não for 4 MB.
- LED on-board: o `main.cpp` assume GPIO8 (ativo-baixo). Ajustar `MAK_LED_PIN`
  se a placa divergir.
- **Não** conectar alimentação externa e USB ao mesmo tempo sem conferir o
  circuito de carga.

## Coexistência de rádio

SoftAP e BLE dividem o rádio de 2,4 GHz. O firmware só roda scanners fora da
demo e desliga o BLE após cada varredura. Validar coexistência na bancada
(plano §1). Referência: docs da Espressif sobre coexistência.

## Checklist mecânico (gabinete fechado)

- [ ] Dimensões internas comportam placa + fios sem pressão.
- [ ] Porta USB acessível para flash/monitor/alimentação de bancada.
- [ ] Antena com caminho RF **não obstruído**; afastada de metal e da bateria.
- [ ] Isolação de metal; alívio de tensão nos fios.
- [ ] Botão/interruptor STOP acessível; desligamento imediato possível.
- [ ] Inspeção de aquecimento com a caixa fechada por 30 min.
- [ ] LiPo: **nunca** soldar direto na célula; usar módulo de proteção/carga.

## Energia

Validar **primeiro via USB de bancada**. Bateria é etapa separada (P1/integração
física). Medir corrente de pico, tensão em carga, brownout e superaquecimento.
