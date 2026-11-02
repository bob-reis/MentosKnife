// DnsPortal.h — DNS cativo local que REGISTRA as consultas (log ao vivo).
//
// Responde a toda consulta A com o IP do AP (comportamento de captive portal) e
// guarda os nomes consultados num ring em RAM, para o painel mostrar "o que os
// aparelhos conectados tentam acessar". Só resolução local; sem recursão/uplink.
#pragma once

#include <WiFiUdp.h>

#include "IModule.h"
#include "ReconTypes.h"

namespace mak {

class DnsPortal : public IModule {
 public:
  bool begin() override;
  void loop() override;
  void stop() override;
  const char* name() const override { return "DnsPortal"; }

  bool isUp() const { return up_; }

  size_t queryCount() const { return qCount_; }
  const DnsQuery* queryAt(size_t i) const { return i < qCount_ ? &queries_[i] : nullptr; }
  void clearQueries() { qCount_ = 0; }

 private:
  void logQuery(const char* name);

  WiFiUDP udp_;
  bool up_ = false;
  DnsQuery queries_[cfg::kDnsRingCap];
  size_t qCount_ = 0;
};

}  // namespace mak
