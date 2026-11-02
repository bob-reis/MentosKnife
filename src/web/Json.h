#pragma once
#include <Arduino.h>
namespace mak {
inline String jsonString(const char* input) {
  String out = "\"";
  const char* hex = "0123456789abcdef";
  for (const unsigned char* p = reinterpret_cast<const unsigned char*>(input); p && *p; ++p) {
    if (*p == '"' || *p == '\\') { out += '\\'; out += char(*p); }
    else if (*p < 32) { out += "\\u00"; out += hex[*p >> 4]; out += hex[*p & 15]; }
    else out += char(*p);
  }
  return out + '"';
}
}
