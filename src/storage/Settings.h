// Settings.h — configuração persistente em NVS (Preferences).
//
// Guarda apenas o necessário para operar: hash+salt da senha do operador,
// senha do AP, e a config opcional de uplink/Discord. Senhas do AP e uplink ficam disponíveis ao SDK Wi-Fi. Credenciais de
// participante/visitante jamais são gravadas.
#pragma once

#include <Arduino.h>

#include "AppConfig.h"

namespace mak {

class Settings {
 public:
  bool begin();  // abre o namespace NVS

  // --- Operador --------------------------------------------------------
  bool hasOperatorPassword() const { return hasOpPw_; }
  // Define a senha (valida força), gerando salt novo. Retorna false se fraca.
  bool setOperatorPassword(const char* password);
  // Verifica a senha contra o hash+salt guardados.
  bool checkOperatorPassword(const char* password) const;

  // --- SoftAP ----------------------------------------------------------
  // O AP de visitantes é sempre ABERTO (sem senha de rádio); não há senha
  // de AP para provisionar. SSID do AP de treinamento (configurável; default
  // cfg::kApSsid).
  bool setApSsid(const char* ssid);
  const String& apSsid() const { return apSsid_; }

  // --- Portal ----------------------------------------------------------
  // Template ativo (allowlist em ConfigValidation). Persiste o id.
  bool setPortalTemplate(const char* id);
  const String& portalTemplate() const { return portalTemplate_; }

  // --- Uplink / Discord (opcional, default DESABILITADO) ---------------
  bool discordEnabled() const { return discordEnabled_; }
  void setDiscordEnabled(bool en);
  bool setDiscordWebhook(const char* url);  // valida https + host discord
  const String& discordWebhook() const { return discordWebhook_; }
  bool setUplink(const char* ssid, const char* password);
  // Apaga o uplink salvo (SSID/senha) da NVS — "esquecer rede".
  void clearUplink();
  const String& uplinkSsid() const { return uplinkSsid_; }
  const String& uplinkPassword() const { return uplinkPw_; }
  bool uplinkConfigured() const { return uplinkSsid_.length() > 0; }

  // Apaga toda a config sensível (factory reset parcial).
  void wipeSecrets();

 private:
  void load();

  bool hasOpPw_ = false;
  String opSalt_;   // hex
  String opHash_;   // hex
  String apSsid_;
  String portalTemplate_;

  bool discordEnabled_ = false;
  String discordWebhook_;
  String uplinkSsid_;
  String uplinkPw_;
};

}  // namespace mak
