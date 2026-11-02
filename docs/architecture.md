# Arquitetura

## Princípio

`UI → API (valida autorização/modo/parâmetros) → App (dispatcher) → módulo →
evento retorna status.` A UI nunca toca periféricos; nada de execução de código
enviado pelo visitante; cenários são **allowlisted**.

## Camadas

```
src/
├── main.cpp                 boot serial + heartbeat + laço cooperativo
├── core/
│   ├── App                  orquestrador/facade: lógica de aplicação
│   ├── StateMachine         BOOT→CONFIG→AP_READY→PORTAL_READY→DEMO_RUNNING→REVEAL→STOPPED (+ERROR_SAFE)
│   ├── EventBus             ring buffer de eventos efêmeros (RAM)
│   ├── Event                struct de evento (sem dados sensíveis)
│   └── TaskScheduler        tarefas periódicas não bloqueantes (millis)
├── security/
│   ├── Auth                 sessões de operador (cookie + CSRF, TTL)
│   ├── RateLimiter          janela deslizante + lockout de login
│   ├── Crypto               SHA-256, aleatoriedade, comparação constante
│   └── ConfigValidation     allowlist de cenários, força de senha, webhook
├── network/
│   ├── AccessPoint          SoftAP isolado (+STA opcional p/ uplink)
│   ├── DnsPortal            DNS cativo local
│   └── WifiScanner          varredura passiva agregada (P1)
├── ble/BleScanner           discovery BLE passivo agregado (P1)
├── portal/
│   ├── PortalController     /portal, /portal/submit (booleano), /portal/state
│   └── PortalEvents         serialização do estado do portal
├── web/
│   ├── WebServer            HTTP: estáticos, captive portal, rotas
│   ├── ApiRoutes            /api/v1/* (operador + CSRF)
│   └── HttpUtil             cookie/guard/CSRF/limite/JSON
├── integrations/
│   └── DiscordNotifier      feed ao vivo opcional (ver security.md)
├── storage/
│   ├── Settings             NVS (hash de senha, config de uplink/discord)
│   └── Telemetry            contadores agregados efêmeros
└── console/SerialConsole    provisionamento/controle por USB
```

## Estados

`BOOT → CONFIG → AP_READY → PORTAL_READY → DEMO_RUNNING → REVEAL → STOPPED`.
`ERROR_SAFE` é alcançável de qualquer estado e mantém log mínimo, desativando a
simulação. Scanners wireless só rodam em `AP_READY`/`PORTAL_READY` (nunca
durante a demo, para não comprometer o captive portal). STOP global vai a
`STOPPED` e limpa a telemetria.

## Fluxo de evento

Toda ação relevante passa por `App::publish()`, que: (1) grava no `EventBus`,
(2) atualiza `Telemetry`, (3) espelha no `DiscordNotifier`. Assim o painel, a
telemetria e o feed do Discord ficam sempre consistentes.

## Testabilidade

`StateMachine`, `EventBus`, `Event`, `TaskScheduler` e `ConfigValidation` são
**puros** (sem Arduino) e cobertos por testes no host (`pio test -e native`).
