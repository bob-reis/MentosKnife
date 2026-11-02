// ReconMonitor.h — recon Wi-Fi ao vivo (console estilo ArmyKnife, transparente).
//
// - scanAps(): varredura detalhada (SSID/BSSID/canal/RSSI/cripto).
// - refreshStations(): clientes conectados ao NOSSO SoftAP (MAC/RSSI).
// - sniffer de probe requests via modo promíscuo (aparelhos "gritando" redes
//   conhecidas). O promíscuo captura no canal do AP; roda junto com o SoftAP.
//
// Observação passiva. Não faz deauth, não injeta, não imita a rede do cliente.
#pragma once

#include "IModule.h"
#include "ReconTypes.h"
#include "storage/PcapBuffer.h"

namespace mak {

class ReconMonitor : public IModule {
 public:
  bool begin() override;
  void loop() override {}
  void stop() override;
  const char* name() const override { return "ReconMonitor"; }

  // Varredura de APs (síncrona, ~janela curta). Retorna a quantidade listada.
  size_t scanAps();
  size_t apCount() const { return apCount_; }
  const ApInfo* apAt(size_t i) const { return i < apCount_ ? &aps_[i] : nullptr; }

  // Clientes conectados ao nosso SoftAP.
  size_t refreshStations();
  size_t stationCount() const { return staCount_; }
  const StaInfo* stationAt(size_t i) const { return i < staCount_ ? &stas_[i] : nullptr; }

  // Sniffer de probe requests (modo promíscuo). on/off.
  bool sniffing() const { return sniffing_; }
  void sniffStart();
  void sniffStop();
  size_t probeCount() const { return probeCount_; }
  const ProbeInfo* probeAt(size_t i) const { return i < probeCount_ ? &probes_[i] : nullptr; }
  void clearProbes();
  void captureFrame(const uint8_t* frame, size_t length, uint64_t uptimeUs);
  size_t copyPcap(uint8_t* out, size_t capacity);
  void pcapStats(size_t& bytes, uint32_t& packets, uint32_t& dropped);
  bool copyProbe(size_t index, ProbeInfo& out);
  bool scanValid() const { return scanValid_; }

  // Chamado pelo callback promíscuo (interno). Público para o trampolim C.
  void onProbe(const uint8_t mac[6], const char* ssid, int8_t rssi, uint32_t now);

 private:
  ApInfo aps_[cfg::kApListCap];
  size_t apCount_ = 0;

  StaInfo stas_[cfg::kStationCap];
  size_t staCount_ = 0;

  ProbeInfo probes_[cfg::kProbeRingCap];
  size_t probeCount_ = 0;

  bool sniffing_ = false;
  bool scanValid_ = false;
  PcapBuffer pcap_;
};

}  // namespace mak
