// WifiScanner.h — varredura Wi-Fi passiva por janelas curtas (P1).
//
// Retorna apenas um AGREGADO (contagens por banda/canal e faixa de RSSI).
// NÃO expõe BSSID/MAC nem SSID individuais no painel; nada é persistido.
// Só roda em estados exclusivos (nunca durante DEMO_RUNNING) para não
// comprometer o captive portal (plano §6).
#pragma once

#include <cstdint>

#include "IModule.h"

namespace mak {

struct WifiScanSummary {
  uint16_t networksSeen = 0;
  uint16_t open = 0;        // redes sem criptografia (exemplo didático)
  uint16_t wpa = 0;         // protegidas
  int8_t strongestRssi = -127;
  int8_t weakestRssi = 0;
  bool valid = false;
};

class WifiScanner : public IModule {
 public:
  bool begin() override;
  void loop() override {}
  void stop() override;
  const char* name() const override { return "WifiScanner"; }

  // Dispara uma varredura síncrona curta e preenche o agregado.
  WifiScanSummary scanOnce();

  const WifiScanSummary& last() const { return last_; }

 private:
  WifiScanSummary last_;
};

}  // namespace mak
