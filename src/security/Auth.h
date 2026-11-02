// Auth.h — sessões de operador com token + CSRF.
//
// Login valida a senha (hash+salt na NVS), cria sessão com TTL e devolve um
// token de sessão (cookie HttpOnly/SameSite) e um token CSRF (double-submit).
// Mutações exigem sessão válida + header CSRF correspondente.
#pragma once

#include <Arduino.h>

#include "AppConfig.h"
#include "storage/Settings.h"

namespace mak {

struct Session {
  char token[cfg::kTokenHexLen + 1] = {0};
  char csrf[cfg::kTokenHexLen + 1] = {0};
  uint32_t expiresAtMs = 0;
  bool active = false;
};

class Auth {
 public:
  explicit Auth(Settings& settings) : settings_(settings) {}

  // Tenta autenticar. Em sucesso, preenche out e retorna true.
  bool login(const char* password, uint32_t nowMs, Session& out);

  // Encerra a sessão do token informado.
  void logout(const char* token);

  // Sessão válida (token existe e não expirou).
  bool isValidSession(const char* token, uint32_t nowMs);
  // Restore the existing session after a page reload, without rotating tokens.
  bool restoreSession(const char* token, uint32_t nowMs, Session& out);

  // Sessão válida + CSRF correspondente (para mutações).
  bool checkCsrf(const char* token, const char* csrf, uint32_t nowMs);

  // Remove sessões expiradas (chamar periodicamente).
  void sweep(uint32_t nowMs);

  // Encerra todas as sessões (STOP/desligamento).
  void revokeAll();

  uint8_t activeCount() const;

 private:
  Session* find(const char* token, uint32_t nowMs);

  Settings& settings_;
  Session sessions_[cfg::kMaxSessions];
};

}  // namespace mak
