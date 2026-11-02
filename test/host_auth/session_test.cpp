#include <cassert>
#include <cstdio>
#include <cstring>
#include "security/Auth.h"
namespace mak { namespace crypto {
void randomHex(size_t bytes, char* out) {
  static unsigned counter = 0;
  std::memset(out, '0', bytes * 2);
  std::snprintf(out + bytes * 2 - 8, 9, "%08x", ++counter);
}
bool constantTimeEquals(const char* a, const char* b) { return a && b && std::strcmp(a, b) == 0; }
}}
int main() {
  mak::Settings settings;
  mak::Auth auth(settings);
  mak::Session original, restored;
  assert(auth.login("test-password", 1000, original));
  assert(auth.restoreSession(original.token, 2000, restored));
  assert(std::strcmp(original.csrf, restored.csrf) == 0);
  assert(std::strcmp(original.token, restored.token) == 0);
  assert(original.expiresAtMs == restored.expiresAtMs);
  assert(auth.activeCount() == 1);
  assert(auth.checkCsrf(restored.token, restored.csrf, 2000));
  assert(!auth.checkCsrf(restored.token, "wrong", 2000));
  assert(!auth.restoreSession("invalid", 2000, restored));
  assert(!restored.active && restored.csrf[0] == '\0');
  assert(!auth.restoreSession(original.token, original.expiresAtMs, restored));
  assert(auth.login("test-password", 3000, original));
  auth.logout(original.token);
  assert(!auth.restoreSession(original.token, 3001, restored));
  assert(auth.login("test-password", UINT32_MAX - 1000, original));
  assert(auth.restoreSession(original.token, 1000, restored));
  assert(!auth.restoreSession(original.token, original.expiresAtMs, restored));
  std::puts("Session restore, CSRF, expiry, logout, invalid token and uptime wrap: passed");
}
