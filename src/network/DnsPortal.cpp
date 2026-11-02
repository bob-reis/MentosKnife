#include "network/DnsPortal.h"

#include <Arduino.h>
#include <WiFi.h>

#include <cstring>

#include "AppConfig.h"

namespace mak {

bool DnsPortal::begin() {
  up_ = udp_.begin(cfg::kDnsPort);
  return up_;
}

void DnsPortal::logQuery(const char* name) {
  if (!name || !name[0]) return;
  uint32_t now = millis();
  for (size_t i = 0; i < qCount_; ++i) {
    if (strncmp(queries_[i].name, name, cfg::kDnsNameLen) == 0) {
      queries_[i].count++;
      queries_[i].lastMs = now;
      return;
    }
  }
  size_t idx;
  if (qCount_ < cfg::kDnsRingCap) {
    idx = qCount_++;
  } else {
    idx = 0;
    for (size_t i = 1; i < qCount_; ++i)
      if (queries_[i].lastMs < queries_[idx].lastMs) idx = i;
  }
  strncpy(queries_[idx].name, name, cfg::kDnsNameLen - 1);
  queries_[idx].name[cfg::kDnsNameLen - 1] = '\0';
  queries_[idx].count = 1;
  queries_[idx].lastMs = now;
}

void DnsPortal::loop() {
  if (!up_) return;
  int size = udp_.parsePacket();
  if (size <= 0 || size > 512) {
    if (size > 512) udp_.clear();
    return;
  }
  uint8_t buf[512];
  int n = udp_.read(buf, sizeof(buf));
  if (n < 12) return;

  // Extrai o QNAME (rótulos até 0x00) a partir do offset 12.
  char name[cfg::kDnsNameLen];
  size_t w = 0;
  int pos = 12;
  while (pos < n && buf[pos] != 0x00) {
    int lbl = buf[pos++];
    if (lbl < 0 || lbl > 63 || pos + lbl > n) break;
    for (int i = 0; i < lbl && w < sizeof(name) - 1; ++i) name[w++] = (char)buf[pos + i];
    pos += lbl;
    if (pos < n && buf[pos] != 0x00 && w < sizeof(name) - 1) name[w++] = '.';
  }
  name[w] = '\0';
  logQuery(name);

  // Monta a resposta: eco da pergunta + 1 answer A apontando para o IP do AP.
  // Só responde a consultas A (IPv4). Para AAAA/outros tipos devolve NOERROR
  // sem resposta, para o aparelho cair no IPv4 (A) e concluir a detecção de
  // captive portal. Responder com tipo incompatível fazia o resolver descartar
  // a resposta.
  int qEnd = pos + 1 + 4;  // 0x00 + QTYPE(2) + QCLASS(2)
  if (qEnd > n) qEnd = n;
  uint16_t qtype = 0;
  if (pos + 3 <= n) qtype = (uint16_t)(buf[pos + 1] << 8) | buf[pos + 2];
  uint8_t resp[600];
  int r = 0;
  memcpy(resp, buf, qEnd);  // header + question
  r = qEnd;
  resp[2] = 0x81;           // QR=1, Opcode=0, RD=1 (eco)
  resp[3] = 0x80;           // RA=1, RCODE=0
  if (qtype == 0x0001) {
    resp[6] = 0x00; resp[7] = 0x01;  // ANCOUNT=1
    resp[8] = 0x00; resp[9] = 0x00;  // NSCOUNT=0
    resp[10] = 0x00; resp[11] = 0x00;  // ARCOUNT=0
    // Answer: ponteiro para o nome (0xC00C), tipo A, classe IN, TTL, RDLENGTH, IP.
    uint8_t ans[] = {0xC0, 0x0C, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x04};
    memcpy(resp + r, ans, sizeof(ans));
    r += sizeof(ans);
    IPAddress ip = WiFi.softAPIP();
    resp[r++] = ip[0]; resp[r++] = ip[1]; resp[r++] = ip[2]; resp[r++] = ip[3];
  } else {
    resp[6] = 0x00; resp[7] = 0x00;  // ANCOUNT=0 (sem AAAA/registro)
    resp[8] = 0x00; resp[9] = 0x00;
    resp[10] = 0x00; resp[11] = 0x00;
  }

  udp_.beginPacket(udp_.remoteIP(), udp_.remotePort());
  udp_.write(resp, r);
  udp_.endPacket();
}

void DnsPortal::stop() {
  if (up_) {
    udp_.stop();
    up_ = false;
  }
  qCount_ = 0;
}

}  // namespace mak
