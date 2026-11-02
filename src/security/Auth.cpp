#include "security/Auth.h"

#include <cstring>

#include "security/Crypto.h"

namespace mak {

bool Auth::login(const char* password, uint32_t nowMs, Session& out) {
  if (!settings_.hasOperatorPassword()) {
    return false;  // sem senha provisionada: login web desabilitado
  }
  if (!settings_.checkOperatorPassword(password)) {
    return false;
  }
  // Acha um slot livre ou o mais antigo a expirar.
  Session* slot = nullptr;
  for (auto& s : sessions_) {
    if (!s.active) {
      slot = &s;
      break;
    }
  }
  if (!slot) {
    // Reusa o slot que expira primeiro.
    slot = &sessions_[0];
    for (auto& s : sessions_) {
      if ((int32_t)(s.expiresAtMs - slot->expiresAtMs) < 0) slot = &s;
    }
  }
  crypto::randomHex(16, slot->token);
  crypto::randomHex(16, slot->csrf);
  slot->expiresAtMs = nowMs + cfg::kSessionTtlMs;
  slot->active = true;
  out = *slot;
  return true;
}

Session* Auth::find(const char* token, uint32_t nowMs) {
  if (!token || token[0] == '\0') return nullptr;
  for (auto& s : sessions_) {
    if (s.active && crypto::constantTimeEquals(s.token, token)) {
      if ((int32_t)(s.expiresAtMs - nowMs) <= 0) {
        s.active = false;
        return nullptr;
      }
      return &s;
    }
  }
  return nullptr;
}

bool Auth::isValidSession(const char* token, uint32_t nowMs) {
  return find(token, nowMs) != nullptr;
}

bool Auth::restoreSession(const char* token, uint32_t nowMs, Session& out) {
  Session* session = find(token, nowMs);
  if (!session) { out = Session{}; return false; }
  out = *session;
  return true;
}

bool Auth::checkCsrf(const char* token, const char* csrf, uint32_t nowMs) {
  Session* s = find(token, nowMs);
  if (!s || !csrf) return false;
  return crypto::constantTimeEquals(s->csrf, csrf);
}

void Auth::logout(const char* token) {
  if (!token) return;
  for (auto& s : sessions_) {
    if (s.active && crypto::constantTimeEquals(s.token, token)) {
      s = Session{};
    }
  }
}

void Auth::sweep(uint32_t nowMs) {
  for (auto& s : sessions_) {
    if (s.active && (int32_t)(s.expiresAtMs - nowMs) <= 0) {
      s = Session{};
    }
  }
}

void Auth::revokeAll() {
  for (auto& s : sessions_) {
    s = Session{};
  }
}

uint8_t Auth::activeCount() const {
  uint8_t n = 0;
  for (const auto& s : sessions_) {
    if (s.active) ++n;
  }
  return n;
}

}  // namespace mak
