// Event.h — evento sintético e efêmero do barramento.
//
// IMPORTANTE (privacidade): um Event NUNCA carrega conteúdo digitado, senha,
// MAC, IP individual, SSID corporativo nem qualquer identificador persistente.
// Apenas sinais booleanos/contadores e rótulos de cenário allowlisted.
#pragma once

#include <cstdint>
#include <cstring>

#include "AppConfig.h"

namespace mak {

enum class EventType : uint8_t {
  kNone = 0,
  kSystem,            // boot, mudança de estado, erro seguro
  kOperatorAuth,      // login/logout/falha de login (sem credenciais)
  kScenarioStart,     // cenário allowlisted iniciado
  kScenarioStop,      // cenário encerrado (inclui STOP)
  kDemoInteraction,   // participante enviou o formulário do portal
  kReveal,            // revelação educativa ativada
  kWifiScan,          // varredura passiva concluída (agregado)
  kBleScan,           // discovery passivo concluído (agregado)
  kCredentialCapture, // portal capturou credenciais (payload NÃO vai no evento)
  kDeviceCapture,     // portal coletou fingerprint do aparelho (efêmero em RAM)
  kCount
};

const char* toString(EventType type);

// Evento compacto de tamanho fixo (cabe no ring buffer em RAM).
struct Event {
  EventType type = EventType::kNone;
  bool flag = false;           // sinal booleano (ex.: demo_interaction=true)
  uint16_t count = 0;          // contador agregado (ex.: redes vistas)
  uint32_t uptimeMs = 0;       // timestamp RELATIVO ao boot (não é relógio)
  char scenarioId[cfg::kScenarioIdMaxLen] = {0};  // rótulo allowlisted

  Event() = default;
  Event(EventType t, bool f, uint16_t c, uint32_t up) : type(t), flag(f), count(c), uptimeMs(up) {}

  void setScenario(const char* id) {
    scenarioId[0] = '\0';
    if (id) {
      std::strncpy(scenarioId, id, sizeof(scenarioId) - 1);
      scenarioId[sizeof(scenarioId) - 1] = '\0';
    }
  }
};

}  // namespace mak
