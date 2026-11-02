#include "ble/BleScanner.h"

#include <NimBLEDevice.h>

#include <cstring>

#include "AppConfig.h"

namespace mak {

bool BleScanner::begin() {
  if (!initialized_) {
    NimBLEDevice::init(cfg::kDeviceName);
    initialized_ = true;
  }
  return true;
}

BleScanSummary BleScanner::scanOnce() {
  BleScanSummary s;
  advCount_ = 0;
  if (!initialized_) {
    begin();
  }
  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setActiveScan(false);  // passivo: não emite scan requests
  scan->setMaxResults(cfg::kMaxScanResults);

  // NimBLE 2.x: getResults(durationMs, continueScan=false) retorna resultados.
  NimBLEScanResults results = scan->getResults(cfg::kBleScanWindowMs, false);
  int n = results.getCount();
  s.advertisersSeen = (uint16_t)n;
  for (int i = 0; i < n; ++i) {
    const NimBLEAdvertisedDevice* d = results.getDevice(i);
    if (!d) continue;
    int rssi = d->getRSSI();
    if (rssi > s.strongestRssi) s.strongestRssi = (int8_t)rssi;
    if (d->isConnectable()) s.connectable++;

    // Detalhamento por anúncio (endereço OMITIDO — privacidade).
    if (advCount_ < cfg::kBleAdvCap) {
      BleAdv& a = advs_[advCount_++];
      const std::string name = d->getName();
      strncpy(a.name, name.c_str(), sizeof(a.name) - 1);
      a.name[sizeof(a.name) - 1] = '\0';
      a.rssi = (int8_t)rssi;
      a.connectable = d->isConnectable();
    }
  }
  s.valid = true;
  scan->clearResults();
  last_ = s;
  return s;
}

void BleScanner::stop() {
  if (initialized_) {
    NimBLEScan* scan = NimBLEDevice::getScan();
    if (scan) scan->clearResults();
    NimBLEDevice::deinit(true);
    initialized_ = false;
  }
}

}  // namespace mak
