# Validação da versão 0.2.1-dev

## Atualização do dashboard em 2026-10-22

- Dashboard reorganizado por ferramentas, sem bloco de contexto, seleção de
  cenários, QR ou revelação.
- Testes de host do Auth passaram: restauração, CSRF, token inválido, expiração,
  logout e rollover do relógio.
- JavaScript com DOM/API simulados: restauração após recarga, aba persistida,
  CSRF em operações, sessão expirada e repetição após erro de rede passaram.
  Sintaxe, IDs e estrutura HTML conferidos; sem validação visual em navegador.
- Firmware e LittleFS instalados com verificação de hash; NVS preservada.
- Serial confirmou `fw=0.2.1-dev state=PORTAL_READY`, heap livre de 138120 bytes.
  Uplink desconectado; testes HTTP e entrega real ao Discord continuam pendentes.

## Verificado anteriormente

- 18 testes de host passaram, incluindo formato, truncamento, capacidade e
  timestamp de 64 bits do buffer PCAP.
- JavaScript: sintaxe, IDs do HTML e fluxos de login/CSRF, envio de SSID/senha,
  limpeza do campo de senha, exibição de erros e início TCP passaram com DOM/API
  simulados. Não é validação visual em navegador.
- Build do firmware e imagem LittleFS concluídos.
- Originais importados do USBArmyKnife e licença conferidos por SHA-256.
- Chip consultado no deploy: ESP32-C3 rev. 0.4, flash de 4 MB.
- Tabela de partições da placa conferida; NVS em 0x9000, tamanho 0x5000,
  coincide com a configuração local.
- Firmware gravado em 0x10000 e LittleFS em 0x390000, com verificação de hash
  pelo esptool. Configuração NVS preservada pela gravação.

## Ainda pendente

Em 2026-10-22, após reconexão e atualização do SSID para `Wifi-Café`,
o firmware e o LittleFS foram novamente gravados e verificados. O serial
confirmou `fw=0.2.0-dev state=PORTAL_READY`, com 138300 bytes de heap livre.
O uplink estava desconectado nessa checagem; acesso HTTP e funcionamento
integrado das ferramentas continuam pendentes de teste.

A entrega real do relatório ao Discord, o PCAP capturado por rádio e os
resultados TCP/DNS/mDNS precisam do teste de bancada descrito em network-tools.md.

## Artefatos gravados

| Arquivo | Bytes | SHA-256 |
|---|---:|---|
| `firmware.bin` | 1607584 | `bd87f52baae6962eb402caf0cab50511d96c1cba85ecb9ad857fe81be6c370c0` |
| `littlefs.bin` | 393216 | `b867e3a7d2f3d1d378ad753be977dea621b9c85abd5563829ab2932e44372111` |

## Portal de simulação — 0.2.2-dev

- Portal com alerta vermelho permanente, escolhas Google/Apple explicitamente
  simuladas e contas fictícias predefinidas em `example.invalid`.
- `/portal/submit` aceita somente `provider=google` ou `provider=apple`; campos
  adicionais e credenciais são rejeitados. Não adiciona capturas ao relatório.
- Evento aceito em PORTAL_READY ou DEMO_RUNNING; STOP continua bloqueando envios.
- Discord recebe apenas evento educativo e provedor fictício. O portal confirma
  registro local, sem afirmar entrega remota.
- Sintaxe JavaScript e os dois fluxos com DOM/fetch simulados conferidos: somente
  o provedor é enviado, sem campos de credenciais.
- Dispositivo USB ausente nesta sessão; instalação e entrega real pendentes.
- Build do firmware 0.2.2-dev concluído com sucesso (flash 84,9%).

### Instalação da 0.2.2-dev para teste

- ESP32-C3 identificado fora do isolamento em `/dev/ttyACM0`, MAC
  `7c:4f:ad:d1:5e:ec`. Firmware e LittleFS gravados com hash verificado;
  configurações NVS preservadas.
- Serial confirmou `fw=0.2.2-dev state=PORTAL_READY`, heap 129396 bytes.
- Uplink conectado, IP `192.168.15.110`; Discord habilitado, um item pendente e
  último resultado HTTP `-1`. A entrega ao Discord não foi confirmada.
