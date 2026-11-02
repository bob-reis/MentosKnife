#include "storage/PcapBuffer.h"
#include <cstring>

namespace mak {
void PcapBuffer::write16(uint16_t n) {
  data_[size_++] = n; data_[size_++] = n >> 8;
}
void PcapBuffer::write32(uint32_t n) {
  for (unsigned shift = 0; shift < 32; shift += 8) data_[size_++] = n >> shift;
}
void PcapBuffer::clear() {
  std::memset(data_, 0, sizeof(data_));
  size_ = 0; packets_ = dropped_ = 0;
  write32(0xa1b2c3d4); write16(2); write16(4);
  write32(0); write32(0); write32(kSnaplen);
  write32(105); // LINKTYPE_IEEE802_11, raw frames without FCS/radiotap.
}
bool PcapBuffer::append(const uint8_t* packet, size_t length, uint64_t uptimeUs) {
  if (!packet || !length || length > UINT32_MAX) { ++dropped_; return false; }
  const size_t captured = length > kSnaplen ? kSnaplen : length;
  if (captured + 16 > kCapacity - size_) { ++dropped_; return false; }
  write32(uptimeUs / 1000000); write32(uptimeUs % 1000000);
  write32(captured); write32(length);
  std::memcpy(data_ + size_, packet, captured);
  size_ += captured; ++packets_;
  return true;
}
}
