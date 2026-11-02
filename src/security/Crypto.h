// Crypto.h — utilitários mínimos: SHA-256, aleatoriedade e comparação segura.
#pragma once

#include <Arduino.h>

namespace mak {
namespace crypto {

// SHA-256(input) em hex minúsculo (64 chars). `out` deve ter >= 65 bytes.
void sha256Hex(const uint8_t* data, size_t len, char* out);
void sha256Hex(const String& s, char* out);

// Gera `nBytes` aleatórios (esp_random) em hex. `out` deve ter >= 2*nBytes+1.
void randomHex(size_t nBytes, char* out);

// Comparação em tempo (quase) constante para tokens/hashes (anti-timing).
bool constantTimeEquals(const char* a, const char* b);

// Hash de senha com salt: SHA-256(salt || ":" || password). `outHex` >= 65.
void hashPassword(const char* password, const char* saltHex, char* outHex);

}  // namespace crypto
}  // namespace mak
