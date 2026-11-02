// PortalEvents.h — serialização mínima do estado do portal p/ o participante.
//
// Mantém o contrato do que o portal pode saber: se está aceitando interação e
// se a revelação está ativa. Nunca expõe estado administrativo nem eventos.
#pragma once

#include <Arduino.h>

namespace mak {
namespace portal {

struct PortalEvents {
  // `scenario` é o ID público do cenário ativo (allowlisted). O portal usa esse
  // ID para escolher a moldura educativa e as dicas de revelação (texto que já
  // é público). Nada administrativo é exposto.
  static String stateJson(bool accepting, bool reveal, const char* scenario) {
    return String("{\"ok\":") + (accepting ? "true" : "false") +
           ",\"accepting\":" + (accepting ? "true" : "false") +
           ",\"reveal\":" + (reveal ? "true" : "false") +
           ",\"scenario\":\"" + (scenario ? scenario : "") + "\"}";
  }
};

}  // namespace portal
}  // namespace mak
