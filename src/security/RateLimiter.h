// RateLimiter.h — limitação simples por janela deslizante e lockout de login.
//
// Protege endpoints de mutação e o login contra força bruta no AP local.
// Chaveia por um identificador leve (ex.: índice de sessão ou hash de IP de
// cliente do AP) — sem persistir IPs.
#pragma once

#include <cstdint>

#include "AppConfig.h"

namespace mak {

class RateLimiter {
 public:
  // Registra um acesso no instante nowMs. Retorna false se excedeu o teto na
  // janela corrente (requisição deve ser recusada com 429).
  bool allow(uint32_t nowMs);

  // Fluxo de login: registra uma falha; retorna true se entrou em lockout.
  bool registerLoginFailure(uint32_t nowMs);
  void registerLoginSuccess();
  bool isLockedOut(uint32_t nowMs) const;

 private:
  uint32_t windowStartMs_ = 0;
  uint8_t hitsInWindow_ = 0;

  uint8_t loginFails_ = 0;
  uint32_t lockoutUntilMs_ = 0;
};

}  // namespace mak
