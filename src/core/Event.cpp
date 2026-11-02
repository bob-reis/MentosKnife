#include "Event.h"

namespace mak {

const char* toString(EventType type) {
  switch (type) {
    case EventType::kNone: return "none";
    case EventType::kSystem: return "system";
    case EventType::kOperatorAuth: return "operator_auth";
    case EventType::kScenarioStart: return "scenario_start";
    case EventType::kScenarioStop: return "scenario_stop";
    case EventType::kDemoInteraction: return "demo_interaction";
    case EventType::kReveal: return "reveal";
    case EventType::kWifiScan: return "wifi_scan";
    case EventType::kBleScan: return "ble_scan";
    case EventType::kCredentialCapture: return "credential_capture";
    case EventType::kDeviceCapture: return "device_capture";
    default: return "unknown";
  }
}

}  // namespace mak
