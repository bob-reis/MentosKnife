// Adapted from USBArmyKnife src/Utilities/PcapBuffer.{h,cpp} (MIT).
// Copyright (c) 2024 i-am-shodan
// See third_party/USBArmyKnife/LICENSE and UPSTREAM.md.
#pragma once
#include <cstddef>
#include <cstdint>

namespace mak {
// Fixed RAM capture; no filesystem, allocations or I/O in the Wi-Fi callback.
// Caller synchronizes append/read/clear. Time is uptime, not Unix time.
class PcapBuffer {
 public:
  static constexpr size_t kCapacity = 8192;
  static constexpr size_t kSnaplen = 512;
  PcapBuffer() { clear(); }
  void clear();
  bool append(const uint8_t* packet, size_t length, uint64_t uptimeUs);
  const uint8_t* data() const { return data_; }
  size_t size() const { return size_; }
  uint32_t packets() const { return packets_; }
  uint32_t dropped() const { return dropped_; }
 private:
  void write16(uint16_t n);
  void write32(uint32_t n);
  uint8_t data_[kCapacity] = {};
  size_t size_ = 0;
  uint32_t packets_ = 0, dropped_ = 0;
};
}
