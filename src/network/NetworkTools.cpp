#include "network/NetworkTools.h"
#include <WiFi.h>

namespace mak {
const uint16_t NetworkTools::ports[kPortCount] = {22, 53, 80, 443, 445, 3389, 8080, 8443};
bool NetworkTools::start(const char* target) {
  if (running_ || WiFi.status() != WL_CONNECTED) return false;
  IPAddress ip;
  if (!ip.fromString(target)) return false;
  const IPAddress local = WiFi.localIP(), mask = WiFi.subnetMask();
  const uint32_t network = uint32_t(local) & uint32_t(mask);
  if (!uint32_t(mask) || (uint32_t(ip) & uint32_t(mask)) != network ||
      uint32_t(ip) == network || uint32_t(ip) == (network | ~uint32_t(mask)) ||
      ip[0] == 0 || ip[0] == 127 || ip[0] >= 224 || ip == local || ip == WiFi.softAPIP()) return false;
  clear(); target_ = ip; local_ = local; mask_ = mask;
  running_ = true; state_ = "running"; startedMs_ = millis();
  return true;
}
void NetworkTools::loop() {
  if (!running_) return;
  if (WiFi.status() != WL_CONNECTED || WiFi.localIP() != local_ || WiFi.subnetMask() != mask_) {
    running_ = false; state_ = "connection_lost"; finishedMs_ = millis(); return;
  }
  WiFiClient client;
  // One bounded TCP connection per loop; no banners, exploits or login attempts.
  open_[checked_] = client.connect(target_, ports[checked_], 250) == 1;
  client.stop();
  if (++checked_ == kPortCount) {
    running_ = false; state_ = "complete"; finishedMs_ = millis();
  }
}
void NetworkTools::stop() {
  if (running_) { running_ = false; state_ = "stopped"; finishedMs_ = millis(); }
}
void NetworkTools::clear() {
  running_ = false; state_ = "idle"; checked_ = 0;
  startedMs_ = finishedMs_ = 0; target_ = IPAddress();
  for (bool& value : open_) value = false;
}
String NetworkTools::json() const {
  String j = "{\"state\":\"" + String(state_) + "\",\"target\":\"" + target_.toString() +
    "\",\"started_ms\":" + String(startedMs_) + ",\"finished_ms\":" + String(finishedMs_) +
    ",\"checked\":" + String(checked_) + ",\"total\":" + String(kPortCount) + ",\"ports\":[";
  for (size_t i = 0; i < checked_; ++i) {
    if (i) j += ',';
    j += "{\"port\":" + String(ports[i]) + ",\"result\":\"" +
      (open_[i] ? "open" : "no_connection") + "\"}";
  }
  return j + "]}";
}
String NetworkTools::summary() const {
  String s = "TCP " + target_.toString() + " — " + state_ + "\n";
  for (size_t i = 0; i < checked_; ++i)
    s += String(ports[i]) + ": " + (open_[i] ? "aberta" : "sem conexão") + "\n";
  return s;
}
}
