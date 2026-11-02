#pragma once
#include <cstring>
namespace mak {
class Settings {
 public:
  bool hasOperatorPassword() const { return true; }
  bool checkOperatorPassword(const char* value) const {
    return value && std::strcmp(value, "test-password") == 0;
  }
};
}
