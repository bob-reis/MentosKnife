# MentosArmyKnife-C3 — Plano de execução

**Versão:** 0.1  
**Data do plano:** 2026-10-21  
**Demonstração-alvo:** 2026-11-03  
**Hardware:** ESP32-C3 SuperMini, gabinete Mentos Kiss  
**Princípio:** demonstrações autorizadas, ambiente isolado, dados sintéticos, nenhuma coleta de credenciais reais.

## 1. Visão, objetivos e não objetivos

Construir um dispositivo portátil com SoftAP Wi-Fi, captive portal educativo, dashboard administrativo, observação Wi-Fi/BLE e execução de cenários determinísticos de conscientização.

**Objetivos MVP:** ligar e obter AP confiável; autenticar operador; acessar painel pelo celular; conectar telefone de teste ao portal; registrar evento sem conteúdo sensível; apresentar revelação; realizar observações wireless passivas quando solicitadas; funcionar totalmente sem Internet; poder desligar imediatamente.

**Não objetivos MVP:** BadUSB, HID/MSC/NCM USB, deauth, rogue AP imitando a rede do cliente, MITM, exfiltração, execução arbitrária de comandos, agentes em computadores, coleta de senhas/cookies, persistência em máquinas de terceiros, limpeza de rastros.

**Restrição de hardware:** USB do C3 é Serial/JTAG, não dispositivo USB HID genérico; usar S3 ou RP2040 em versão futura. SoftAP e BLE compartilham rádio de 2,4 GHz, exigir testes de coexistência.

## 2. Referências upstream e política de reutilização

- `https://github.com/i-am-shodan/USBArmyKnife` — estrutura modular, roteador de comandos, tarefas, interface.
- `https://github.com/ravssh/WifiExe` — ações controladas por web, máquina de estados.
- `https://github.com/dfyR433/USB_Ware` — UI minimalista para acionar operações.

Os códigos são **referências, não dependências diretas**. Antes de portar trechos, verificar licenças de cada projeto, atribuições, licenças transitivas e riscos de componentes. Adotar implementação original para reduzir acoplamento a USB OTG, bibliotecas e SDK específicos. Registrar em `docs/upstream-review.md` o commit SHA auditado de cada fonte; nunca puxar `main/master` dinamicamente para builds de release.

## 3. Repositórios e bootstrap da estação

```bash
mkdir -p ~/dev/mentos && cd ~/dev/mentos
git clone https://github.com/i-am-shodan/USBArmyKnife.git upstream-USBArmyKnife
git clone https://github.com/ravssh/WifiExe.git upstream-WifiExe
git clone https://github.com/dfyR433/USB_Ware.git upstream-USB_Ware
mkdir MentosArmyKnife-C3 && cd MentosArmyKnife-C3
git init -b main
python3 -m venv .venv
source .venv/bin/activate
python -m pip install --upgrade pip platformio
pio --version
```

Verifique `lsusb`, `dmesg -w`, `pio device list` e confirme porta serial; Linux pode exigir permissões de grupo. Não conecte alimentação externa e USB simultaneamente sem conferir circuitos da placa e esquema de carga. Confirme chip e capacidade real de flash via `esptool.py flash_id` após identificar a porta.

## 4. Estrutura do novo projeto

```text
MentosArmyKnife-C3/
├── platformio.ini
├── README.md
├── LICENSE
├── .gitignore
├── .github/workflows/ci.yml
├── include/
│   ├── AppConfig.h
│   ├── IModule.h
│   └── Event.h
├── src/
│   ├── main.cpp
│   ├── core/{App,StateMachine,EventBus,TaskScheduler}.*
│   ├── security/{Auth,RateLimiter,ConfigValidation}.*
│   ├── network/{AccessPoint,DnsPortal,WifiScanner}.*
│   ├── ble/BleScanner.*
│   ├── portal/{PortalController,PortalEvents}.*
│   ├── web/{WebServer,ApiRoutes}.*
│   └── storage/{Settings,Telemetry}.*
├── data/{index.html,portal.html,style.css,app.js}
├── test/{test_events,test_state_machine,test_configuration}/
├── docs/{architecture,hardware,security,upstream-review,runbook,training-script}.md
└── scripts/{flash,smoke-test,package-release}.sh
```

Organização orientada a interfaces; regra simples: UI chama API; API valida autorização, modo e parâmetros; dispatcher publica evento; módulo executa; evento retorna status. Sem acesso direto de UI a periféricos. Sem execução arbitrária de código ou scripts enviados pelo visitante.

## 5. Stack e configuração

- C++ / PlatformIO; inicializar com Arduino core ESP32 compatível com a placa e dependências *pinned* após smoke test; ESP-IDF puro é opção posterior.
- AP e HTTP: WiFi, WebServer ou ESPAsyncWebServer conforme medição de consumo; DNSServer apenas para encaminhamento local de resolução do portal; LittleFS para conteúdo estático.
- BLE: NimBLE-Arduino (validar compatibilidade com o core escolhido).
- JSON: ArduinoJson somente se necessário; preferir respostas compactas.
- Logs: ring buffer em RAM com descarte automático; saída serial apenas em modo de desenvolvimento; não salvar entradas sensíveis.
- OTA: fora do MVP; realizar atualização presencial via USB.

`platformio.ini` inicial, a validar na bancada:

```ini
[platformio]
default_envs = mentos-c3

[env:mentos-c3]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
monitor_speed = 115200
board_build.filesystem = littlefs
build_flags =
    -D MENTOS_C3=1
    -D TRAINING_MODE=1
lib_deps =
    h2zero/NimBLE-Arduino
```

**Atenção:** board ID genérico pode não coincidir com o mapa de pinos/capacidade de flash da SuperMini; verificar antes de gravar. Fixar as versões apenas depois da primeira compilação estável.

## 6. Arquitetura e estados

Estados principais: `BOOT -> CONFIG -> AP_READY -> PORTAL_READY -> DEMO_RUNNING -> REVEAL -> STOPPED`. Falha: `ERROR_SAFE`, que mantém log local mínimo e desativa a simulação. Observação Wi-Fi/BLE funciona em estados exclusivos ou com pausa do portal, para não comprometer o captive portal.

Fluxo de dados:

```text
Celular operador -> Wi-Fi AP isolado -> Dashboard autenticado
                                         |
                                         v
                                     API Router
                                         |
                                         v
                                  Command Dispatcher
                                   /      |        \
                               Portal   Wi-Fi Scan  BLE Scan
                                   \      |       /
                                     Event Bus
                                         |
                                 Telemetria efêmera

Celular de demonstração -> AP -> DNS local -> Portal educativo
                                                 |
                                            evento booleano
                                              (sem senha)
```

### Interface dos módulos (conceito)

```cpp
class IModule {
public:
    virtual ~IModule() = default;
    virtual bool begin() = 0;
    virtual void loop() = 0;
    virtual void stop() = 0;
    virtual const char* name() const = 0;
};
```

Evitar `delay()` bloqueante nos handlers. Usar estado + `millis()` ou tarefas FreeRTOS pequenas, conforme necessidade validada em testes. Limitar heap, conexões simultâneas e tamanho de requisições.

## 7. Endpoints planejados

| Método | Endpoint | Autorização | Função |
|---|---|---|---|
| GET | `/api/v1/health` | operador | status, uptime e heap livre |
| GET | `/api/v1/modules` | operador | disponibilidade de módulos |
| POST | `/api/v1/scenarios/start` | operador | inicia um cenário por ID allowlisted |
| POST | `/api/v1/scenarios/stop` | operador | parada imediata |
| GET | `/api/v1/events` | operador | eventos sintéticos recentes |
| POST | `/api/v1/wifi/scan` | operador | varredura de redes próximas |
| POST | `/api/v1/ble/scan` | operador | captura limitada de anúncios BLE |
| GET | `/portal` | participante | página educativa |
| POST | `/portal/submit` | participante | registra interação booleana, sem receber senhas reais |

A UI do participante **não deve** chamar endpoints administrativos. Implementar senha forte exclusiva de operador, cookies de sessão com `HttpOnly`/`SameSite` quando aplicável, proteção CSRF para mutações, rate limiting, AP de visitantes aberto porém isolado (sem bridge/NAT) e desabilitar debug em release. Como HTTP local não protege do observador conectado, separar administração da experiência do participante; idealmente controlar o dashboard por USB serial ou em fase administrativa exclusiva, sem participantes conectados.

## 8. Cenário padrão do treinamento

1. Operador inicia o dispositivo na bancada em rede de laboratório; valida health check.
2. Exibe QR de demonstração/SSID `MENTOS-TREINAMENTO` e informa que é uma simulação voluntária.
3. Participante usa aparelho dedicado de teste e abre portal; formulário usa apenas identidade fictícia predefinida, ou botão `Simular envio` sem campos de senha.
4. Firmware registra `demo_interaction=true`, timestamp relativo e `scenario_id` sem conteúdo digitado, MAC, IP individual ou outros identificadores persistentes.
5. Dashboard mostra `Interação de demonstração detectada`.
6. Ativa revelação no portal e abre fisicamente a caixinha.
7. Discute controles: validação de SSID/certificado, evitar credenciais em páginas não verificadas, phishing-resistant MFA, gerenciamento de dispositivos e sinalização de incidentes.
8. Apaga a telemetria efêmera, desliga AP e remove energia.

Não prometer pop-up automático em todos os aparelhos: captive portal e mecanismos de detecção variam entre Android/iOS. Manter URL/QR local de contingência.

## 9. Marcos e backlog

### P0 — necessário para o evento

- [ ] `P0-01` Identificar chip, flash, pinout, regulador e porta USB.
- [ ] `P0-02` Hello World / Serial / LED e reinicialização repetível.
- [ ] `P0-03` Estrutura PlatformIO, lint/format e CI build.
- [ ] `P0-04` SoftAP isolado com senha e endereço fixo.
- [ ] `P0-05` Servidor HTTP + arquivos LittleFS.
- [ ] `P0-06` DNS cativo e fallback de URL manual.
- [ ] `P0-07` Portal educativo sem armazenamento de entrada.
- [ ] `P0-08` Dashboard com autenticação do operador.
- [ ] `P0-09` Estado, eventos e comando STOP.
- [ ] `P0-10` Telemetria efêmera, endpoint de health.
- [ ] `P0-11` Teste integrado com Android/iOS de laboratório.
- [ ] `P0-12` Ensaio na Mentos e release congelada.

### P1 — somente se P0 estável até 28/10

- [ ] Wi-Fi scanner passivo por janelas curtas, com retorno agregado.
- [ ] BLE discovery passivo, com endereços omitidos do painel.
- [ ] Runner de cenários allowlisted e timer.
- [ ] Botão físico STOP ou modo seguro no boot.

### P2 — depois do evento

- [ ] OTA com assinatura e rollback.
- [ ] Interface USB HID/MSC para target S3/RP2040.
- [ ] Exportação de eventos anonimizados para dashboard externo/SOC.
- [ ] Suite de testes de carga e perfis de memória.

## 10. Calendário de execução

| Datas | Marco | Saída verificável |
|---|---|---|
| 21–22/10 | Base | Builds, flash e serial funcionando |
| 23–25/10 | Rede | AP, HTTP, DNS e dashboard mínimo |
| 26–28/10 | Demonstração | Portal, evento, revelação end-to-end |
| 29–30/10 | Segurança/extra | Autenticação, testes; scanners apenas se estáveis |
| 31/10–01/11 | Integração física | bateria, temperatura, USB, ensaio 30 min |
| 02/11 | Release freeze | binário, checksum, imagens de configuração, rollback manual |
| 03/11 | Treinamento | execução seguindo runbook |

**Kill-switch de escopo:** se portal + revelação não estiverem estáveis em 28/10, remover BLE, scanner Wi-Fi e scripting da v0.1.

## 11. Critérios de aceitação

- [ ] Liga e serve o portal em até 20 s em 9/10 tentativas na bancada (meta, ajustar após medição).
- [ ] Acesso manual ao portal em Android e iOS via URL local, mesmo sem pop-up automático.
- [ ] Reinício controlado não corrompe arquivos nem configuração.
- [ ] 30 minutos de demonstrações repetidas sem travar/reboot.
- [ ] STOP encerra cenário e desabilita novos envios/eventos.
- [ ] Nenhum dado de credencial armazenado; varredura de logs confirma.
- [ ] UI participante não alcança API administrativa.
- [ ] Sem gateway para rede corporativa; acesso isolado e sem conexão à Internet.
- [ ] Teste de bateria e temperatura na caixinha fechada, sem aquecimento perigoso.
- [ ] Binário release, config e roteiro existem offline no notebook.

## 12. Plano de testes

**Unitários:** máquina de estados, validação de configuração, expiração de sessão, descarte do buffer de eventos, recusa de comandos não allowlisted.

**Integração:** AP + DNS + HTTP, LittleFS após reboot, autenticação de endpoints, falha de autenticação, limite de payload HTTP, perda/reconexão de Wi-Fi, long-running uptime.

**E2E:** Android/iPhone, sem internet, 2 clientes simultâneos, portal na URL manual, revelação após toque, STOP físico/logical, perda de alimentação e boot seguro.

**Segurança:** revisão de senhas default, logs, endpoints, capacidade de desligar, ausência de ponte para a rede real, permissões de CI, SBOM/dependências, sem credenciais em git.

**Hardware:** medir corrente de pico e consumo, tensão em carga, superaquecimento, brownout, integridade mecânica, antena afastada de metal e bateria, desconexão rápida; jamais usar LiPo sem proteção/carga apropriada.

## 13. Riscos e mitigação

| Risco | Impacto | Mitigação |
|---|---|---|
| USB HID do C3 não existe | Alto | remover USB do MVP; interface de HAL para S3 futuro |
| Flash/RAM insuficientes | Alto | medir chip, reduzir UI e usar LittleFS estático |
| AP+BLE instáveis | Médio | execução por modos exclusivos, BLE P1 |
| Pop-up captive portal não aparece | Médio | URL/QR como fallback |
| Wi-Fi na Mentos perde alcance | Médio | testar orientação/posição e gabinete |
| LiPo/brownout/sobreaquecimento | Alto | regulador correto, proteção, medição e USB de bancada no ensaio |
| UI admin exposta ao convidado | Alto | separar sessões/fases, autenticação, controle por serial como fallback |
| Pouco tempo até 03/11 | Alto | congelar P0 em 28/10 e P1 opcional |

## 14. Segurança, privacidade e autorização

Escopo aprovado por escrito pelo cliente: local, janela, participantes voluntários, equipamentos de teste e contatos de emergência. SSID nunca se faz passar pela WLAN real. Não solicitar senhas de produção. Demonstrar somente em aparelho de teste; apresentar a revelação educacional imediatamente. Não fazer deauth, interferência, extração de cookies, credenciais ou comandos remotos nos dispositivos dos participantes. Não persistir IP/MAC, SSID corporativo, identificadores BLE ou payload de formulário; logs sintéticos são apagados no STOP ou desligamento. Tratar o dispositivo como equipamento de laboratório.

## 15. Hardware / gabinete Mentos

Itens: ESP32-C3 SuperMini, caixinha Mentos Kiss, interruptor, espaçadores isolantes, cabo USB acessível, bateria LiPo com proteção e módulo de recarga + regulador adequado **somente após conferir a placa real**. Ideal: primeiro validar tudo via alimentação USB externa de bancada; bateria é uma etapa separada.

Checklist mecânico: dimensões internas, porta USB acessível, antena com caminho RF não obstruído, isolação de metal, nenhuma pressão na célula LiPo, alívio de tensão dos fios, botão STOP acessível, inspeção de aquecimento fechado por 30 min, capacidade de desligamento imediato. Não soldar diretamente em bateria; usar módulo apropriado e proteger contra curto.

## 16. Runbook de apresentação (10 min)

- **T-20 min**: ligar, teste health, AP, URL alternativa, bateria e smartphone de teste.
- **00:00**: mostrar Mentos sem revelar o circuito; contextualizar o experimento.
- **01:30**: mostrar SSID de treinamento e conectar telefone de teste.
- **03:00**: visitar portal e acionar botão `Simular envio`.
- **04:30**: dashboard exibe evento, sem exibir dado sensível.
- **05:30**: ativar revelação e abrir Mentos.
- **07:00**: explicar controles defensivos e limites do experimento.
- **09:30**: STOP, limpar eventos, desligar.

Plano B: caso o captive portal não abra automaticamente, usar endereço/IP local; se Wi-Fi falhar, mostrar gravação do ensaio e demonstrar hardware físico. Versão estável guardada offline.

## 17. Definition of Done da v0.1

Tag `v0.1.0`, build reproduzível, SHA256 do firmware, CHANGELOG, documentação de flash e recuperação, evidência de smoke test, revisão de segurança, hardware validado e roteiro ensaiado. Nunca liberar versão baseada somente em compilação sem teste no C3 real.

## 18. Primeira sessão de desenvolvimento

1. Clonar os três upstreams, anotar commits e licenças.
2. Rodar `pio device list` e identificar flash e interface USB.
3. Criar repo MentosArmyKnife-C3 e boot serial mínimo.
4. Implementar `AccessPoint` + `/health` antes do portal.
5. Testar celular no AP isolado e medir heap.
6. Registrar resultados e decidir framework/libraries a fixar.

## Fontes

- USBArmyKnife: https://github.com/i-am-shodan/USBArmyKnife
- WifiExe: https://github.com/ravssh/WifiExe
- USB_Ware: https://github.com/dfyR433/USB_Ware
- ESP32-C3 RF coexistence: https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32c3/api-guides/coexist.html
- Espressif USB Serial/JTAG: https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/api-guides/usb-serial-jtag-console.html
