#include "network/WifiScanner.h"

#include <WiFi.h>

#include "AppConfig.h"

namespace mak {

bool WifiScanner::begin() { return true; }

WifiScanSummary WifiScanner::scanOnce() {
  WifiScanSummary s;
  // scanNetworks() exige a interface STA ativa; o SoftAP roda em modo AP puro,
  // então habilitamos STA (vira APSTA) antes de varrer, sem derrubar o AP.
  if (!(WiFi.getMode() & WIFI_MODE_STA)) {
    WiFi.mode(WIFI_MODE_APSTA);
  }
  // Varredura síncrona; async_scan poderia ser usado para não bloquear, mas
  // aqui a janela é curta e só ocorre fora da demo.
  int n = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/false,
                            /*passive=*/true, cfg::kWifiScanWindowMs);
  if (n < 0) {
    s.valid = false;
    last_ = s;
    return s;
  }
  int count = n > (int)cfg::kMaxScanResults ? (int)cfg::kMaxScanResults : n;
  s.networksSeen = (uint16_t)n;
  for (int i = 0; i < count; ++i) {
    int32_t rssi = WiFi.RSSI(i);
    if (rssi > s.strongestRssi) s.strongestRssi = (int8_t)rssi;
    if (rssi < s.weakestRssi) s.weakestRssi = (int8_t)rssi;
    if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) {
      ++s.open;
    } else {
      ++s.wpa;
    }
    // Deliberadamente NÃO guardamos BSSID/SSID: só o agregado importa.
  }
  s.valid = true;
  WiFi.scanDelete();
  last_ = s;
  return s;
}

void WifiScanner::stop() { WiFi.scanDelete(); }

}  // namespace mak
