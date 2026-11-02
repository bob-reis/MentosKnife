// Testes do ring buffer de eventos (host, sem Arduino).
#include <unity.h>

#include "core/EventBus.h"

using namespace mak;

static Event mk(EventType t, uint16_t c) { return Event(t, true, c, c); }

void test_publish_and_order() {
  EventBus bus;
  bus.publish(mk(EventType::kSystem, 1));
  bus.publish(mk(EventType::kDemoInteraction, 2));
  TEST_ASSERT_EQUAL(2, bus.size());
  TEST_ASSERT_EQUAL(1, bus.at(0)->count);  // mais antigo
  TEST_ASSERT_EQUAL(2, bus.at(1)->count);  // mais novo
}

void test_ring_discards_oldest() {
  EventBus bus;
  const size_t cap = bus.capacity();
  for (size_t i = 0; i < cap + 5; ++i) {
    bus.publish(mk(EventType::kSystem, (uint16_t)i));
  }
  TEST_ASSERT_EQUAL(cap, bus.size());
  // O mais antigo retido deve ser o índice 5 (0..4 descartados).
  TEST_ASSERT_EQUAL(5, bus.at(0)->count);
  TEST_ASSERT_EQUAL(cap + 4, bus.at(cap - 1)->count);
  TEST_ASSERT_EQUAL(cap + 5, bus.totalPublished());
}

void test_latest_of_type() {
  EventBus bus;
  bus.publish(mk(EventType::kDemoInteraction, 10));
  bus.publish(mk(EventType::kSystem, 20));
  bus.publish(mk(EventType::kDemoInteraction, 30));
  const Event* e = bus.latestOfType(EventType::kDemoInteraction);
  TEST_ASSERT_NOT_NULL(e);
  TEST_ASSERT_EQUAL(30, e->count);
}

void test_clear() {
  EventBus bus;
  bus.publish(mk(EventType::kSystem, 1));
  bus.clear();
  TEST_ASSERT_TRUE(bus.empty());
  TEST_ASSERT_NULL(bus.at(0));
}

void test_scenario_label_copy() {
  Event e;
  e.setScenario("phish-email-basic");
  TEST_ASSERT_EQUAL_STRING("phish-email-basic", e.scenarioId);
  // Rótulo longo é truncado sem estourar o buffer.
  e.setScenario("0123456789012345678901234567890123456789");
  TEST_ASSERT_TRUE(strlen(e.scenarioId) < cfg::kScenarioIdMaxLen);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_publish_and_order);
  RUN_TEST(test_ring_discards_oldest);
  RUN_TEST(test_latest_of_type);
  RUN_TEST(test_clear);
  RUN_TEST(test_scenario_label_copy);
  return UNITY_END();
}
