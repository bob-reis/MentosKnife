#include "network/ReconMonitor.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_timer.h>

#include <cstring>

namespace mak {

const char* encToString(uint8_t enc) {
  switch (enc) {
    case WIFI_AUTH_OPEN: return "ABERTA";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
    case WIFI_AUTH_WPA3_PSK: return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-EAP";
    default: return "?";
  }
}

void macToStr(const uint8_t mac[6], char out[cfg::kMacStrLen]) {
  snprintf(out, cfg::kMacStrLen, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1],
           mac[2], mac[3], mac[4], mac[5]);
}

namespace {
ReconMonitor* g_instance = nullptr;
portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;

// Callback do modo promíscuo (contexto da task Wi-Fi) — mantém-se curto.
void IRAM_ATTR promiscCb(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT || !g_instance) return;
  const wifi_promiscuous_pkt_t* p = (const wifi_promiscuous_pkt_t*)buf;
  const uint8_t* fr = p->payload;
  // Frame control byte0: probe request = 0x40 (type=mgmt, subtype=0100).
  int len = p->rx_ctrl.sig_len - 4; // IDF includes the four-byte FCS.
  if (len < 26) return;
  if (fr[0] != 0x40) return;
  g_instance->captureFrame(fr, len, esp_timer_get_time());
  const uint8_t* src = fr + 10;  // addr2
  // Tagged params a partir do offset 24; tag 0 = SSID.
  char ssid[cfg::kSsidMaxLen] = {0};
  if (fr[24] == 0x00) {
    int slen = fr[25];
    if (slen > 0 && (26 + slen) <= len) {
      if (slen > (int)cfg::kSsidMaxLen - 1) slen = cfg::kSsidMaxLen - 1;
      memcpy(ssid, fr + 26, slen);
      ssid[slen] = '\0';
    }
  }
  g_instance->onProbe(src, ssid, p->rx_ctrl.rssi, millis());
}
}  // namespace

bool ReconMonitor::begin() {
  g_instance = this;
  return true;
}

size_t ReconMonitor::scanAps() {
  // scanNetworks() exige a interface STA ativa. O SoftAP sobe em modo AP puro
  // (vira APSTA só durante o uplink); sem STA, a varredura retorna -2 e vira
  // wifi_scan_failed. Habilitamos STA sem derrubar o AP de visitantes.
  if (!(WiFi.getMode() & WIFI_MODE_STA)) {
    WiFi.mode(WIFI_MODE_APSTA);
  }
  int n = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/true,
                           /*passive=*/true, /*max_ms_per_chan=*/120);
  scanValid_ = n >= 0;
  apCount_ = 0;
  if (n <= 0) {
    WiFi.scanDelete();
    return 0;
  }
  for (int i = 0; i < n && apCount_ < cfg::kApListCap; ++i) {
    ApInfo& a = aps_[apCount_++];
    String ssid = WiFi.SSID(i);
    strncpy(a.ssid, ssid.c_str(), sizeof(a.ssid) - 1);
    a.ssid[sizeof(a.ssid) - 1] = '\0';
    const uint8_t* b = WiFi.BSSID(i);
    if (b) memcpy(a.bssid, b, 6); else memset(a.bssid, 0, 6);
    a.rssi = (int8_t)WiFi.RSSI(i);
    a.channel = (uint8_t)WiFi.channel(i);
    a.enc = (uint8_t)WiFi.encryptionType(i);
  }
  WiFi.scanDelete();
  return apCount_;
}

size_t ReconMonitor::refreshStations() {
  staCount_ = 0;
  wifi_sta_list_t list;
  if (esp_wifi_ap_get_sta_list(&list) != ESP_OK) return 0;
  for (int i = 0; i < list.num && staCount_ < cfg::kStationCap; ++i) {
    StaInfo& s = stas_[staCount_++];
    memcpy(s.mac, list.sta[i].mac, 6);
    s.rssi = (int8_t)list.sta[i].rssi;
  }
  return staCount_;
}

void ReconMonitor::onProbe(const uint8_t mac[6], const char* ssid, int8_t rssi, uint32_t now) {
  portENTER_CRITICAL(&g_mux);
  // Deduplica por MAC+SSID; incrementa contagem.
  for (size_t i = 0; i < probeCount_; ++i) {
    if (memcmp(probes_[i].mac, mac, 6) == 0 && strncmp(probes_[i].ssid, ssid, cfg::kSsidMaxLen) == 0) {
      probes_[i].count++;
      probes_[i].rssi = rssi;
      probes_[i].lastMs = now;
      portEXIT_CRITICAL(&g_mux);
      return;
    }
  }
  size_t idx;
  if (probeCount_ < cfg::kProbeRingCap) {
    idx = probeCount_++;
  } else {
    // Substitui o mais antigo (menor lastMs).
    idx = 0;
    for (size_t i = 1; i < probeCount_; ++i)
      if (probes_[i].lastMs < probes_[idx].lastMs) idx = i;
  }
  memcpy(probes_[idx].mac, mac, 6);
  strncpy(probes_[idx].ssid, ssid, cfg::kSsidMaxLen - 1);
  probes_[idx].ssid[cfg::kSsidMaxLen - 1] = '\0';
  probes_[idx].rssi = rssi;
  probes_[idx].count = 1;
  probes_[idx].lastMs = now;
  portEXIT_CRITICAL(&g_mux);
}

void ReconMonitor::sniffStart() {
  if (sniffing_) return;
  wifi_promiscuous_filter_t filter;
  filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
  esp_wifi_set_promiscuous_filter(&filter);
  esp_wifi_set_promiscuous_rx_cb(&promiscCb);
  if (esp_wifi_set_promiscuous(true) == ESP_OK) {
    sniffing_ = true;
  }
}

void ReconMonitor::sniffStop() {
  if (!sniffing_) return;
  esp_wifi_set_promiscuous(false);
  sniffing_ = false;
}

void ReconMonitor::clearProbes() {
  portENTER_CRITICAL(&g_mux);
  probeCount_ = 0;
  pcap_.clear();
  portEXIT_CRITICAL(&g_mux);
}

void ReconMonitor::captureFrame(const uint8_t* frame, size_t length, uint64_t uptimeUs) {
  portENTER_CRITICAL(&g_mux);
  pcap_.append(frame, length, uptimeUs);
  portEXIT_CRITICAL(&g_mux);
}

size_t ReconMonitor::copyPcap(uint8_t* out, size_t capacity) {
  portENTER_CRITICAL(&g_mux);
  const size_t n = pcap_.size();
  if (capacity >= n) memcpy(out, pcap_.data(), n);
  portEXIT_CRITICAL(&g_mux);
  return capacity >= n ? n : 0;
}

void ReconMonitor::pcapStats(size_t& bytes, uint32_t& packets, uint32_t& dropped) {
  portENTER_CRITICAL(&g_mux);
  bytes = pcap_.size(); packets = pcap_.packets(); dropped = pcap_.dropped();
  portEXIT_CRITICAL(&g_mux);
}

bool ReconMonitor::copyProbe(size_t index, ProbeInfo& out) {
  portENTER_CRITICAL(&g_mux);
  const bool valid = index < probeCount_;
  if (valid) out = probes_[index];
  portEXIT_CRITICAL(&g_mux);
  return valid;
}

void ReconMonitor::stop() {
  sniffStop();
  apCount_ = 0;
  staCount_ = 0;
  clearProbes();
}

}  // namespace mak
