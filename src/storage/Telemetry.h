// Telemetry.h — contadores agregados efêmeros (em RAM).
//
// Resumo barato para o dashboard e para o feed do Discord. Nada é persistido;
// `wipe()` zera tudo no STOP/desligamento. Não contém identificadores.
#pragma once

#include <cstdint>

namespace mak {

struct Telemetry {
  uint32_t bootCount = 0;          // reservado (poderia vir de NVS)
  uint32_t demoInteractions = 0;   // nº de "Simular envio"
  uint32_t scenariosStarted = 0;
  uint32_t reveals = 0;
  uint32_t wifiScans = 0;
  uint32_t bleScans = 0;
  uint32_t loginFailures = 0;

  void wipe() { *this = Telemetry{}; }
};

}  // namespace mak
