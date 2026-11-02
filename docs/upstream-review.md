# Revisão de upstreams

USBArmyKnife possui uma **importação seletiva** documentada em
[`../third_party/USBArmyKnife/UPSTREAM.md`](../third_party/USBArmyKnife/UPSTREAM.md).
Os demais projetos abaixo continuam como referências de arquitetura/UX. Nenhum trecho é puxado dinamicamente em build de release. Qualquer
port de trecho deve registrar aqui o arquivo, o commit e a licença.

| Projeto | Commit auditado | Data | Licença | Uso como referência |
|---|---|---|---|---|
| [USBArmyKnife](https://github.com/i-am-shodan/USBArmyKnife) | `fcea8c5d3f0ce48652215183272b19f3e1cd8473` | 2026-10-15 | MIT | Estrutura modular, roteador de comandos, tarefas, organização de `src/`. |
| [WifiExe](https://github.com/ravssh/WifiExe) | `3a4814074be9ac6a03d916d5c90223f698f63d1a` | 2024-09-10 | MIT | Ações controladas por web, máquina de estados, conteúdo web embutido. |
| [USB_Ware](https://github.com/dfyR433/USB_Ware) | `363420e2f574e46d31a48393c118ac547a782e36` | 2026-08-16 | MIT | UI minimalista para acionar operações. |
| [GoFlockYourself](https://github.com/OffGridPete/GoFlockYourself) | `02df05477b15b2dd27dac0127295292f71494ac2` | 2026-10-05 | MIT | Organização por interfaces (`include/`+`src/`), NimBLE, dual-board, PT-BR. |

## Política

- Preferir módulos compatíveis com o C3; ports devem preservar licença,
  commit, originais e mudanças locais. O plano original de implementação
  exclusivamente original foi substituído por importação seletiva.
- Todas as fontes são MIT; atribuição mantida neste arquivo e no `LICENSE`.
- Antes de portar qualquer trecho: conferir licença transitiva da dependência
  que ele arrasta e avaliar risco de componente.
- Builds de release fixam versões (`lib_deps` com `@`), nunca `main`/`master`.

## Decisões derivadas da revisão

- Servidor HTTP **síncrono** (`WebServer` do core) em vez de assíncrono para
  previsibilidade de RAM no C3; reavaliar se E2E exigir 2+ clientes de fato.
- BLE via **NimBLE-Arduino** (como GoFlockYourself), desligado após cada
  varredura por coexistência com o SoftAP (rádio 2,4 GHz compartilhado).
