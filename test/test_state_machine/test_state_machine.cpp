// Testes da máquina de estados (host, sem Arduino).
#include <unity.h>

#include "core/StateMachine.h"

using namespace mak;

void test_happy_path() {
  StateMachine sm;
  TEST_ASSERT_EQUAL(State::kBoot, sm.current());
  TEST_ASSERT_TRUE(sm.transitionTo(State::kConfig));
  TEST_ASSERT_TRUE(sm.transitionTo(State::kApReady));
  TEST_ASSERT_TRUE(sm.transitionTo(State::kPortalReady));
  TEST_ASSERT_TRUE(sm.transitionTo(State::kDemoRunning));
  TEST_ASSERT_TRUE(sm.transitionTo(State::kReveal));
  TEST_ASSERT_TRUE(sm.transitionTo(State::kPortalReady));
}

void test_invalid_transitions_rejected() {
  StateMachine sm;
  // BOOT não pode pular direto para DEMO_RUNNING.
  TEST_ASSERT_FALSE(sm.transitionTo(State::kDemoRunning));
  TEST_ASSERT_EQUAL(State::kBoot, sm.current());
}

void test_stop_from_any_operational() {
  StateMachine sm;
  sm.transitionTo(State::kConfig);
  sm.transitionTo(State::kApReady);
  sm.transitionTo(State::kPortalReady);
  sm.transitionTo(State::kDemoRunning);
  TEST_ASSERT_TRUE(sm.transitionTo(State::kStopped));
  TEST_ASSERT_EQUAL(State::kStopped, sm.current());
}

void test_error_safe_always_reachable() {
  StateMachine sm;
  TEST_ASSERT_TRUE(sm.transitionTo(State::kErrorSafe));
  TEST_ASSERT_EQUAL(State::kErrorSafe, sm.current());
  TEST_ASSERT_TRUE(sm.transitionTo(State::kStopped));
}

void test_demo_interaction_only_in_demo() {
  StateMachine sm;
  sm.transitionTo(State::kConfig);
  sm.transitionTo(State::kApReady);
  sm.transitionTo(State::kPortalReady);
  TEST_ASSERT_FALSE(sm.acceptsDemoInteraction());
  sm.transitionTo(State::kDemoRunning);
  TEST_ASSERT_TRUE(sm.acceptsDemoInteraction());
}

void test_scan_not_during_demo() {
  StateMachine sm;
  sm.transitionTo(State::kConfig);
  sm.transitionTo(State::kApReady);
  TEST_ASSERT_TRUE(sm.allowsWirelessScan());
  sm.transitionTo(State::kPortalReady);
  sm.transitionTo(State::kDemoRunning);
  TEST_ASSERT_FALSE(sm.allowsWirelessScan());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_happy_path);
  RUN_TEST(test_invalid_transitions_rejected);
  RUN_TEST(test_stop_from_any_operational);
  RUN_TEST(test_error_safe_always_reachable);
  RUN_TEST(test_demo_interaction_only_in_demo);
  RUN_TEST(test_scan_not_during_demo);
  return UNITY_END();
}
