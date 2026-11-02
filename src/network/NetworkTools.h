#pragma once
#include <Arduino.h>
#include <IPAddress.h>

namespace mak {
class NetworkTools {
 public:
  static constexpr size_t kPortCount = 8;
  static const uint16_t ports[kPortCount];
  bool start(const char* target);
  void loop();
  void stop();
  void clear();
  String json() const;
  String summary() const;
  bool running() const { return running_; }
 private:
  IPAddress target_, local_, mask_;
  bool running_ = false;
  const char* state_ = "idle";
  size_t checked_ = 0;
  bool open_[kPortCount] = {};
  uint32_t startedMs_ = 0, finishedMs_ = 0;
};
}
