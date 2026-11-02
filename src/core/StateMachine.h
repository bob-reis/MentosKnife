// StateMachine.h — máquina de estados do dispositivo.
//
// BOOT -> CONFIG -> AP_READY -> PORTAL_READY -> DEMO_RUNNING -> REVEAL -> STOPPED
// Falha: ERROR_SAFE (mantém log mínimo, desativa a simulação).
// Transições são validadas; transições inválidas são recusadas.
// Código puro (sem Arduino) — testável no host.
#pragma once

#include <cstdint>

namespace mak {

enum class State : uint8_t {
  kBoot = 0,
  kConfig,
  kApReady,
  kPortalReady,
  kDemoRunning,
  kReveal,
  kStopped,
  kErrorSafe,
  kCount
};

const char* toString(State s);

class StateMachine {
 public:
  StateMachine() = default;

  State current() const { return current_; }
  State previous() const { return previous_; }

  // Tenta transicionar. Retorna true se permitida (e aplicada).
  // ERROR_SAFE é alcançável a partir de qualquer estado.
  // STOPPED é alcançável a partir de qualquer estado operacional (STOP global).
  bool transitionTo(State next);

  // true se `next` é alcançável a partir do estado atual.
  bool canTransition(State next) const;

  // true se o dispositivo está em um estado em que a demo pode receber
  // interações (DEMO_RUNNING). Fora disso, /portal/submit é recusado.
  bool acceptsDemoInteraction() const { return current_ == State::kDemoRunning; }

  // true se scanners wireless podem rodar (estados exclusivos ou com portal
  // pausado). Aqui: permitido em AP_READY e STOPPED-para-admin; nunca durante
  // DEMO_RUNNING para não comprometer o captive portal.
  bool allowsWirelessScan() const {
    return current_ == State::kApReady || current_ == State::kPortalReady;
  }

 private:
  static bool isOperational(State s);

  State current_ = State::kBoot;
  State previous_ = State::kBoot;
};

}  // namespace mak
