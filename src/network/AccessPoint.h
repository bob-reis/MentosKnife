// SoftAP local + uplink STA para ferramentas de rede e relatórios.
// Sem bridge/NAT; AP e STA compartilham rádio/canal.
#pragma once

#include <cstdint>

#include "IModule.h"

namespace mak {

class AccessPoint : public IModule {
 public:
  bool begin() override;   // não usar: sem SSID; use beginOpen()
  void loop() override {}
  void stop() override;
  const char* name() const override { return "AccessPoint"; }

  // Sobe o SoftAP isolado e ABERTO (sem senha) com o SSID fornecido. É uma
  // rede de visitantes de treinamento: o captive portal cuida da simulação.
  bool beginOpen(const char* ssid);

  // Conecta ao uplink escolhido pelo operador. Assíncrono:
  // verifique uplinkConnected().
  bool startUplink(const char* ssid, const char* password);
  void stopUplink();
  bool uplinkConnected() const;

  bool isUp() const { return up_; }
  uint8_t clientCount() const;
  const char* apIpString() const { return apIp_; }

 private:
  bool up_ = false;
  bool uplinkRequested_ = false;
  char apIp_[16] = {0};
};

}  // namespace mak
