// ReconTypes.h — estruturas de dados da observação wireless e captura.
//
// Tudo é mantido em RAM, em buffers fixos, e exibido ao vivo no painel. Nada é
// persistido; é apagado ao limpar/desligar. Contexto: treinamento AUTORIZADO,
// rede isolada, participantes de teste, revelação imediata (ver docs/security.md).
#pragma once

#include <cstdint>

#include "AppConfig.h"

namespace mak {

// AP visto na varredura.
struct ApInfo {
  char ssid[cfg::kSsidMaxLen];
  uint8_t bssid[6];
  int8_t rssi;
  uint8_t channel;
  uint8_t enc;        // wifi_auth_mode_t
};

// Estação conectada ao NOSSO SoftAP.
struct StaInfo {
  uint8_t mac[6];
  int8_t rssi;
};

// Probe request observado (aparelho procurando uma rede conhecida).
struct ProbeInfo {
  uint8_t mac[6];
  char ssid[cfg::kSsidMaxLen];  // SSID procurado ("" = wildcard)
  int8_t rssi;
  uint16_t count;
  uint32_t lastMs;
};

// Consulta DNS feita por um cliente conectado (nome resolvido para o AP).
struct DnsQuery {
  char name[cfg::kDnsNameLen];
  uint16_t count;
  uint32_t lastMs;
};

// Anúncio BLE observado (sem endereço — privacidade). Nome pode ser vazio.
struct BleAdv {
  char name[cfg::kBleNameLen];
  int8_t rssi;
  bool connectable;
};

// Credencial capturada no portal (exibida ao vivo; exportável no relatório).
struct Capture {
  char user[cfg::kCredFieldLen];
  char pass[cfg::kCredFieldLen];
  char tmpl[cfg::kTemplateIdLen];  // template do portal que recebeu o envio
  char sid[cfg::kSidLen];          // id de sessão (associa ao fingerprint)
  char ip[cfg::kIpLen];            // IP do cliente no SoftAP (DHCP)
  uint32_t uptimeMs;
};

// Fingerprint do aparelho coletado pelo portal via JavaScript (dados que o
// navegador entrega sem pedir permissão). Efêmero em RAM, exibido ao vivo
// (admin + Discord) e no relatório; apagado em Limpar/Parar/Desligar.
struct DeviceInfo {
  char os[cfg::kDevFieldLen];
  char browser[cfg::kDevFieldLen];
  char device[cfg::kDevFieldLen];
  char platform[cfg::kDevFieldLen];
  char lang[cfg::kDevFieldLen];
  char screen[cfg::kDevFieldLen];
  char cores[cfg::kDevFieldLen];
  char mem[cfg::kDevFieldLen];
  char conn[cfg::kDevFieldLen];
  char battery[cfg::kDevFieldLen];
  char tz[cfg::kTzLen];
  char ua[cfg::kUaLen];
  bool touch;
  char tmpl[cfg::kTemplateIdLen];  // template do portal que coletou
  char sid[cfg::kSidLen];          // id de sessão (associa ao login)
  char ip[cfg::kIpLen];            // IP do cliente no SoftAP (DHCP)
  uint32_t uptimeMs;
};

const char* encToString(uint8_t enc);
void macToStr(const uint8_t mac[6], char out[cfg::kMacStrLen]);

}  // namespace mak
