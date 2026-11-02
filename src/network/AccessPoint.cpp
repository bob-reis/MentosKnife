#include "network/AccessPoint.h"

#include <Arduino.h>
#include <WiFi.h>

#include "AppConfig.h"

namespace mak {

bool AccessPoint::begin() {
  // Sem senha não subimos o AP: política de "sem senha default".
  return false;
}

bool AccessPoint::beginOpen(const char* ssid) {
  // AP sempre ABERTO: rede de visitantes de treinamento (Wi-Fi livre +
  // captive portal). Não há senha de rádio — é o cenário realista que o
  // participante vê. O console admin continua protegido pela senha do
  // operador; o isolamento do AP (sem bridge/NAT) é o guardrail de fato.
  if (!ssid || !ssid[0] || strlen(ssid) > cfg::kSsidMaxLen - 1) {
    return false;
  }

  IPAddress ip(cfg::kApIp[0], cfg::kApIp[1], cfg::kApIp[2], cfg::kApIp[3]);
  IPAddress gw = ip;
  IPAddress mask(cfg::kApNetmask[0], cfg::kApNetmask[1], cfg::kApNetmask[2],
                 cfg::kApNetmask[3]);

  WiFi.persistent(false);        // não grava credenciais na NVS do SDK
  WiFi.mode(WIFI_MODE_AP);
  if (!WiFi.softAPConfig(ip, gw, mask)) {
    return false;
  }
  // ssid_hidden=false, max_connection limitado; AP isolado é padrão do IDF
  // quando não há bridge. Canal fixo para previsibilidade. Senha nullptr =>
  // rede aberta (sem WPA2).
  bool ok = WiFi.softAP(ssid, /*passphrase=*/nullptr, cfg::kApChannel,
                        /*ssid_hidden=*/0, cfg::kApMaxClients);
  if (!ok) {
    return false;
  }
  strncpy(apIp_, WiFi.softAPIP().toString().c_str(), sizeof(apIp_) - 1);
  apIp_[sizeof(apIp_) - 1] = '\0';
  up_ = true;
  return true;
}

bool AccessPoint::startUplink(const char* ssid, const char* password) {
  if (!ssid || !ssid[0] || !up_) {
    return false;
  }
  uplinkRequested_ = true;
  // Mantém o AP e adiciona STA para egress de telemetria.
  WiFi.mode(WIFI_MODE_APSTA);
  WiFi.begin(ssid, password);
  return true;
}

void AccessPoint::stopUplink() {
  uplinkRequested_ = false;
  WiFi.disconnect(/*wifioff=*/false, /*eraseap=*/true);
  WiFi.mode(WIFI_MODE_AP);
}

bool AccessPoint::uplinkConnected() const {
  return uplinkRequested_ && WiFi.status() == WL_CONNECTED;
}

uint8_t AccessPoint::clientCount() const {
  return up_ ? (uint8_t)WiFi.softAPgetStationNum() : 0;
}

void AccessPoint::stop() {
  stopUplink();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  up_ = false;
}

}  // namespace mak
