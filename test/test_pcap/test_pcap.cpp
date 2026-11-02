#include <unity.h>
#include "storage/PcapBuffer.h"
using mak::PcapBuffer;
uint32_t read32(const uint8_t* p) {
  return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}
void test_header_and_timestamp() {
  PcapBuffer b;
  TEST_ASSERT_EQUAL(24, b.size());
  TEST_ASSERT_EQUAL_HEX32(0xa1b2c3d4, read32(b.data()));
  TEST_ASSERT_EQUAL(105, read32(b.data() + 20));
  const uint8_t packet[] = {0x40, 0, 1};
  TEST_ASSERT_TRUE(b.append(packet, sizeof(packet), 4294967296ULL + 123));
  TEST_ASSERT_EQUAL(4294, read32(b.data() + 24));
  TEST_ASSERT_EQUAL(967419, read32(b.data() + 28));
  TEST_ASSERT_EQUAL(sizeof(packet), read32(b.data() + 32));
  TEST_ASSERT_EQUAL_MEMORY(packet, b.data() + 40, sizeof(packet));
}
void test_truncation_and_capacity() {
  PcapBuffer b;
  uint8_t packet[600] = {};
  TEST_ASSERT_TRUE(b.append(packet, sizeof(packet), 0));
  TEST_ASSERT_EQUAL(PcapBuffer::kSnaplen, read32(b.data() + 32));
  TEST_ASSERT_EQUAL(sizeof(packet), read32(b.data() + 36));
  while (b.append(packet, sizeof(packet), 0)) {}
  const size_t size = b.size();
  TEST_ASSERT_LESS_OR_EQUAL(PcapBuffer::kCapacity, size);
  TEST_ASSERT_EQUAL(1, b.dropped());
  TEST_ASSERT_FALSE(b.append(packet, sizeof(packet), 0));
  TEST_ASSERT_EQUAL(size, b.size());
  TEST_ASSERT_EQUAL(2, b.dropped());
  TEST_ASSERT_FALSE(b.append(nullptr, 1, 0));
  b.clear();
  TEST_ASSERT_EQUAL(24, b.size());
  TEST_ASSERT_EQUAL(0, b.packets());
  TEST_ASSERT_EQUAL(0, b.dropped());
  TEST_ASSERT_EQUAL(0, b.data()[40]);
}
int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_header_and_timestamp);
  RUN_TEST(test_truncation_and_capacity);
  return UNITY_END();
}
