#include "core/StateMachine.h"

namespace mak {

const char* toString(State s) {
  switch (s) {
    case State::kBoot: return "BOOT";
    case State::kConfig: return "CONFIG";
    case State::kApReady: return "AP_READY";
    case State::kPortalReady: return "PORTAL_READY";
    case State::kDemoRunning: return "DEMO_RUNNING";
    case State::kReveal: return "REVEAL";
    case State::kStopped: return "STOPPED";
    case State::kErrorSafe: return "ERROR_SAFE";
    default: return "UNKNOWN";
  }
}

bool StateMachine::isOperational(State s) {
  switch (s) {
    case State::kApReady:
    case State::kPortalReady:
    case State::kDemoRunning:
    case State::kReveal:
      return true;
    default:
      return false;
  }
}

bool StateMachine::canTransition(State next) const {
  if (next == current_) {
    return true;  // no-op permitido
  }
  // ERROR_SAFE sempre alcançável.
  if (next == State::kErrorSafe) {
    return true;
  }
  // STOP global: qualquer estado operacional (ou CONFIG) pode ir a STOPPED.
  if (next == State::kStopped) {
    return isOperational(current_) || current_ == State::kConfig ||
           current_ == State::kErrorSafe;
  }

  switch (current_) {
    case State::kBoot:
      return next == State::kConfig;
    case State::kConfig:
      return next == State::kApReady;
    case State::kApReady:
      // Pode subir para o portal; pode voltar a CONFIG (re-provisionar).
      return next == State::kPortalReady || next == State::kConfig;
    case State::kPortalReady:
      return next == State::kDemoRunning || next == State::kApReady;
    case State::kDemoRunning:
      return next == State::kReveal || next == State::kPortalReady;
    case State::kReveal:
      return next == State::kPortalReady;
    case State::kStopped:
      // De STOPPED só se reinicia o ciclo via CONFIG (ou reboot).
      return next == State::kConfig;
    case State::kErrorSafe:
      // Só sai de ERROR_SAFE por reboot/stop controlado.
      return next == State::kStopped;
    default:
      return false;
  }
}

bool StateMachine::transitionTo(State next) {
  if (!canTransition(next)) {
    return false;
  }
  if (next != current_) {
    previous_ = current_;
    current_ = next;
  }
  return true;
}

}  // namespace mak
