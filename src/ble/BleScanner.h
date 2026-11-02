// BleScanner.h — discovery BLE passivo (P1), via NimBLE.
//
// Retorna um AGREGADO (quantidade de anúncios, faixa de RSSI, conectáveis) e,
// adicionalmente, o DETALHAMENTO por anúncio (nome/RSSI/conectável). Endereços
// BLE são OMITIDOS do painel e não são persistidos. SoftAP e BLE dividem o
// rádio 2,4 GHz: só executar em estados exclusivos (plano §1/§6).
#pragma once

#include <cstdint>

#include "IModule.h"
#include "ReconTypes.h"

namespace mak {

struct BleScanSummary {
  uint16_t advertisersSeen = 0;
  uint16_t connectable = 0;
  int8_t strongestRssi = -127;
  bool valid = false;
};

class BleScanner : public IModule {
 public:
  bool begin() override;
  void loop() override {}
  void stop() override;
  const char* name() const override { return "BleScanner"; }

  // Varredura passiva curta; preenche o agregado e o detalhamento por anúncio.
  BleScanSummary scanOnce();
  const BleScanSummary& last() const { return last_; }

  // Detalhamento dos anúncios observados (sem endereço). Ephemeral em RAM.
  size_t advCount() const { return advCount_; }
  const BleAdv* advAt(size_t i) const { return i < advCount_ ? &advs_[i] : nullptr; }

 private:
  bool initialized_ = false;
  BleScanSummary last_;
  BleAdv advs_[cfg::kBleAdvCap];
  size_t advCount_ = 0;
};

}  // namespace mak
