#include "web/ApiRoutes.h"

#include <Arduino.h>

#include "AppConfig.h"
#include "core/App.h"
#include "security/ConfigValidation.h"
#include "web/HttpUtil.h"

namespace mak {
namespace web {
namespace {

String jsonBool(bool b) { return b ? "true" : "false"; }
String jsonStr(const char* s);  // definido mais abaixo

void handleLogin(::WebServer& srv, App& app) {
  uint32_t now = millis();
  if (app.adminRate().isLockedOut(now)) {
    http::sendError(srv, 429, "login_locked");
    return;
  }
  if (!http::enforceBodyLimit(srv)) return;

  // Aceita form-urlencoded (campo "password").
  String pw = srv.hasArg("password") ? srv.arg("password") : String();
  Session s;
  if (!app.auth().login(pw.c_str(), now, s)) {
    app.adminRate().registerLoginFailure(now);
    http::sendError(srv, 401, "invalid_credentials");
    return;
  }
  app.adminRate().registerLoginSuccess();

  // Cookie de sessão endurecido. (HTTP local não garante confidencialidade;
  // ver docs/security.md — preferir fase admin sem participantes.)
  String cookie = String(cfg::kSessionCookieName) + "=" + s.token +
                  "; HttpOnly; SameSite=Strict; Path=/; Max-Age=" +
                  String(cfg::kSessionTtlMs / 1000);
  srv.sendHeader("Set-Cookie", cookie);
  http::sendJson(srv, 200, String("{\"ok\":true,\"csrf\":\"") + s.csrf + "\"}");
}

void handleLogout(::WebServer& srv, App& app) {
  if (!http::requireOperatorCsrf(srv, app)) return;
  String tok = http::sessionTokenFromCookie(srv);
  if (!tok.isEmpty()) app.auth().logout(tok.c_str());
  srv.sendHeader("Set-Cookie",
                 String(cfg::kSessionCookieName) + "=; HttpOnly; Path=/; Max-Age=0");
  http::sendJson(srv, 200, "{\"ok\":true}");
}

void handleSession(::WebServer& srv, App& app) {
  if (!http::requireOperator(srv, app)) return;
  const String token = http::sessionTokenFromCookie(srv);
  Session session;
  const uint32_t now = millis();
  if (!app.auth().restoreSession(token.c_str(), now, session)) {
    http::sendError(srv, 401, "unauthorized"); return;
  }
  // HttpOnly cookie remains private; only the matching CSRF token is returned.
  http::sendJson(srv, 200, String("{\"ok\":true,\"csrf\":\"") + session.csrf +
    "\",\"expires_in_ms\":" + String(session.expiresAtMs - now) + "}");
}

void handleHealth(::WebServer& srv, App& app) {
  if (!http::requireOperator(srv, app)) return;
  String j = "{\"ok\":true";
  j += ",\"fw\":\"" + String(cfg::kFirmwareVersion) + "\"";
  j += ",\"state\":\"" + String(toString(app.state().current())) + "\"";
  j += ",\"uptime_ms\":" + String(millis());
  j += ",\"heap_free\":" + String((uint32_t)ESP.getFreeHeap());
  j += ",\"ap_clients\":" + String(app.ap().clientCount());
  j += ",\"ap_ssid\":" + jsonStr(app.settings().apSsid().c_str());
  j += ",\"scenario\":\"" + String(app.currentScenario()) + "\"";
  j += ",\"discord_enabled\":" + jsonBool(app.discord().enabled());
  j += ",\"discord_uplink\":" + jsonBool(app.ap().uplinkConnected());
  j += ",\"discord_pending\":" + String((uint32_t)app.discord().pending());
  j += ",\"discord_report\":\"" + String(app.discord().reportState()) + "\"";
  j += ",\"discord_http\":" + String(app.discord().lastHttpCode());
  j += ",\"discord_dropped\":" + String(app.discord().dropped());
  j += "}";
  http::sendJson(srv, 200, j);
}

void handleModules(::WebServer& srv, App& app) {
  if (!http::requireOperator(srv, app)) return;
  String j = "{\"ok\":true,\"scenarios\":[";
  for (size_t i = 0; i < validation::scenarioCount(); ++i) {
    const ScenarioInfo* s = validation::scenarioAt(i);
    if (i) j += ",";
    j += "{\"id\":\"" + String(s->id) + "\",\"label\":\"" + String(s->label) + "\"}";
  }
  j += "],\"modules\":[\"AccessPoint\",\"DnsPortal\",\"WebServer\",\"WifiScanner\",\"BleScanner\",\"DiscordNotifier\",\"NetworkTools\",\"ReconMonitor\",\"PcapBuffer\"]}";
  http::sendJson(srv, 200, j);
}

void handleEvents(::WebServer& srv, App& app) {
  if (!http::requireOperator(srv, app)) return;
  EventBus& bus = app.events();
  String j = "{\"ok\":true,\"total\":" + String(bus.totalPublished()) + ",\"events\":[";
  for (size_t i = 0; i < bus.size(); ++i) {
    const Event* e = bus.at(i);
    if (!e) continue;
    if (i) j += ",";
    j += "{\"type\":\"" + String(toString(e->type)) + "\"";
    j += ",\"flag\":" + jsonBool(e->flag);
    j += ",\"count\":" + String(e->count);
    j += ",\"uptime_ms\":" + String(e->uptimeMs);
    j += ",\"scenario\":\"" + String(e->scenarioId) + "\"}";
  }
  j += "]}";
  http::sendJson(srv, 200, j);
}

void handleScenarioStart(::WebServer& srv, App& app) {
  if (!http::requireOperatorCsrf(srv, app)) return;
  String id = srv.hasArg("id") ? srv.arg("id") : String();
  if (!app.startScenario(id.c_str())) {
    http::sendError(srv, 400, "scenario_rejected");
    return;
  }
  http::sendJson(srv, 200, String("{\"ok\":true,\"scenario\":\"") + id + "\"}");
}

void handleScenarioStop(::WebServer& srv, App& app) {
  if (!http::requireOperatorCsrf(srv, app)) return;
  app.stopAll("operator_stop");
  http::sendJson(srv, 200, "{\"ok\":true}");
}

void handleReveal(::WebServer& srv, App& app, bool on) {
  if (!http::requireOperatorCsrf(srv, app)) return;
  bool ok = on ? app.activateReveal() : app.endReveal();
  if (!ok) {
    http::sendError(srv, 409, "invalid_state");
    return;
  }
  http::sendJson(srv, 200, "{\"ok\":true}");
}

void handleWifiScan(::WebServer& srv, App& app) {
  if (!http::requireOperatorCsrf(srv, app)) return;
  WifiScanSummary s;
  if (!app.runWifiScan(s)) {
    http::sendError(srv, 409, "scan_not_allowed");
    return;
  }
  String j = "{\"ok\":true,\"networks\":" + String(s.networksSeen) +
             ",\"open\":" + String(s.open) + ",\"wpa\":" + String(s.wpa) +
             ",\"rssi_max\":" + String(s.strongestRssi) + "}";
  http::sendJson(srv, 200, j);
}

void handleBleScan(::WebServer& srv, App& app) {
  if (!http::requireOperatorCsrf(srv, app)) return;
  BleScanSummary s;
  if (!app.runBleScan(s)) {
    http::sendError(srv, 409, "scan_not_allowed");
    return;
  }
  String j = "{\"ok\":true,\"advertisers\":" + String(s.advertisersSeen) +
             ",\"connectable\":" + String(s.connectable) +
             ",\"rssi_max\":" + String(s.strongestRssi) + "}";
  http::sendJson(srv, 200, j);
}

void handleBle(::WebServer& srv, App& app) {
  // Detalhamento dos anúncios BLE (nome/RSSI/conectável, SEM endereço).
  if (!http::requireOperator(srv, app)) return;
  String j = "{\"ok\":true,\"adverts\":[";
  for (size_t i = 0; i < app.bleCount(); ++i) {
    const BleAdv* a = app.bleAt(i);
    if (!a) continue;
    if (i) j += ",";
    j += "{\"name\":" + jsonStr(a->name[0] ? a->name : "(sem nome)") +
         ",\"rssi\":" + String(a->rssi) +
         ",\"connectable\":" + jsonBool(a->connectable) + "}";
  }
  j += "]}";
  http::sendJson(srv, 200, j);
}

String jsonStr(const char* s) {
  // Escapa aspas/barras p/ embutir em JSON.
  String o = "\"";
  for (const char* p = s; p && *p; ++p) {
    char c = *p;
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if ((unsigned char)c >= 0x20) o += c;
  }
  o += "\"";
  return o;
}

void handleReconScan(::WebServer& srv, App& app) {
  if (!http::requireOperatorCsrf(srv, app)) return;
  size_t aps = app.reconScan();
  if (!app.recon().scanValid()) { http::sendError(srv, 503, "wifi_scan_failed"); return; }
  http::sendJson(srv, 200, String("{\"ok\":true,\"aps\":") + aps +
                               ",\"clients\":" + app.recon().stationCount() + "}");
}

void handleRecon(::WebServer& srv, App& app) {
  if (!http::requireOperator(srv, app)) return;
  ReconMonitor& r = app.recon();
  String j = "{\"ok\":true,\"sniffing\":";
  j += r.sniffing() ? "true" : "false";
  j += ",\"aps\":[";
  for (size_t i = 0; i < r.apCount(); ++i) {
    const ApInfo* a = r.apAt(i);
    char bssid[cfg::kMacStrLen]; macToStr(a->bssid, bssid);
    if (i) j += ",";
    j += "{\"ssid\":" + jsonStr(a->ssid[0] ? a->ssid : "(oculto)") +
         ",\"bssid\":\"" + bssid + "\",\"ch\":" + String(a->channel) +
         ",\"rssi\":" + String(a->rssi) + ",\"enc\":\"" + encToString(a->enc) + "\"}";
  }
  j += "],\"clients\":[";
  for (size_t i = 0; i < r.stationCount(); ++i) {
    const StaInfo* s = r.stationAt(i);
    char mac[cfg::kMacStrLen]; macToStr(s->mac, mac);
    if (i) j += ",";
    j += "{\"mac\":\"" + String(mac) + "\",\"rssi\":" + String(s->rssi) + "}";
  }
  j += "]}";
  http::sendJson(srv, 200, j);
}

void handleSniff(::WebServer& srv, App& app) {
  if (!http::requireOperatorCsrf(srv, app)) return;
  bool on = srv.hasArg("on") && (srv.arg("on") == "1" || srv.arg("on") == "true");
  app.reconSniff(on);
  http::sendJson(srv, 200, String("{\"ok\":true,\"sniffing\":") +
                               (app.recon().sniffing() ? "true" : "false") + "}");
}

void handleProbes(::WebServer& srv, App& app) {
  if (!http::requireOperator(srv, app)) return;
  ReconMonitor& r = app.recon();
  String j = "{\"ok\":true,\"probes\":[";
  for (size_t i = 0; i < cfg::kProbeRingCap; ++i) {
    ProbeInfo snapshot;
    if (!r.copyProbe(i, snapshot)) break;
    const ProbeInfo* p = &snapshot;
    char mac[cfg::kMacStrLen]; macToStr(p->mac, mac);
    if (i) j += ",";
    j += "{\"mac\":\"" + String(mac) + "\",\"ssid\":" +
         jsonStr(p->ssid[0] ? p->ssid : "(wildcard)") + ",\"rssi\":" + String(p->rssi) +
         ",\"count\":" + String(p->count) + "}";
  }
  j += "]}";
  http::sendJson(srv, 200, j);
}

void handleDns(::WebServer& srv, App& app) {
  if (!http::requireOperator(srv, app)) return;
  DnsPortal& d = app.dns();
  String j = "{\"ok\":true,\"queries\":[";
  for (size_t i = 0; i < d.queryCount(); ++i) {
    const DnsQuery* q = d.queryAt(i);
    if (i) j += ",";
    j += "{\"name\":" + jsonStr(q->name) + ",\"count\":" + String(q->count) + "}";
  }
  j += "]}";
  http::sendJson(srv, 200, j);
}

void handleCaptures(::WebServer& srv, App& app) {
  // Conteúdo do portal: exige sessão; também incluído nos relatórios explícitos.
  if (!http::requireOperator(srv, app)) return;
  String j = "{\"ok\":true,\"captures\":[";
  for (size_t i = 0; i < app.captureCount(); ++i) {
    const Capture* c = app.captureAt(i);
    if (i) j += ",";
    j += "{\"user\":" + jsonStr(c->user) + ",\"pass\":" + jsonStr(c->pass) +
         ",\"ip\":" + jsonStr(c->ip) +
         ",\"template\":" + jsonStr(c->tmpl) + ",\"uptime_ms\":" + String(c->uptimeMs);
    // Fingerprint do mesmo aparelho (associado pelo id de sessão), se houver.
    const DeviceInfo* d = app.deviceBySid(c->sid);
    if (d) {
      j += ",\"device\":{\"os\":" + jsonStr(d->os) + ",\"browser\":" + jsonStr(d->browser) +
           ",\"device\":" + jsonStr(d->device) + ",\"screen\":" + jsonStr(d->screen) +
           ",\"lang\":" + jsonStr(d->lang) + "}";
    } else {
      j += ",\"device\":null";
    }
    j += "}";
  }
  j += "]}";
  http::sendJson(srv, 200, j);
}

void handleApSsid(::WebServer& srv, App& app) {
  if (!http::requireOperatorCsrf(srv, app)) return;
  const String ssid = srv.arg("ssid");
  if (!app.setApSsid(ssid.c_str())) {
    http::sendError(srv, 400, "invalid_ap_ssid");
    return;
  }
  http::sendJson(srv, 200, String("{\"ok\":true,\"ssid\":") + jsonStr(ssid.c_str()) +
                               ",\"rebooting\":true}");
}

void handlePortalTemplates(::WebServer& srv, App& app) {
  if (!http::requireOperator(srv, app)) return;
  String j = "{\"ok\":true,\"active\":\"" + String(app.settings().portalTemplate().c_str()) +
             "\",\"templates\":[";
  for (size_t i = 0; i < validation::templateCount(); ++i) {
    const PortalTemplate* t = validation::templateAt(i);
    if (i) j += ",";
    j += "{\"id\":\"" + String(t->id) + "\",\"label\":\"" + String(t->label) + "\"}";
  }
  j += "]}";
  http::sendJson(srv, 200, j);
}

void handlePortalTemplateSet(::WebServer& srv, App& app) {
  if (!http::requireOperatorCsrf(srv, app)) return;
  const String id = srv.arg("template");
  if (!app.setPortalTemplate(id.c_str())) {
    http::sendError(srv, 400, "invalid_template");
    return;
  }
  http::sendJson(srv, 200, String("{\"ok\":true,\"template\":") + jsonStr(id.c_str()) + "}");
}

void handleDevices(::WebServer& srv, App& app) {
  // Fingerprints coletados pelo portal (dados do navegador; sem credencial).
  if (!http::requireOperator(srv, app)) return;
  String j = "{\"ok\":true,\"devices\":[";
  for (size_t i = 0; i < app.deviceCount(); ++i) {
    const DeviceInfo* d = app.deviceAt(i);
    if (i) j += ",";
    j += "{\"ip\":" + jsonStr(d->ip) + ",\"os\":" + jsonStr(d->os) +
         ",\"browser\":" + jsonStr(d->browser) +
         ",\"device\":" + jsonStr(d->device) + ",\"platform\":" + jsonStr(d->platform) +
         ",\"lang\":" + jsonStr(d->lang) + ",\"screen\":" + jsonStr(d->screen) +
         ",\"touch\":" + jsonBool(d->touch) + ",\"cores\":" + jsonStr(d->cores) +
         ",\"mem\":" + jsonStr(d->mem) + ",\"conn\":" + jsonStr(d->conn) +
         ",\"battery\":" + jsonStr(d->battery) + ",\"tz\":" + jsonStr(d->tz) +
         ",\"ua\":" + jsonStr(d->ua) + ",\"template\":" + jsonStr(d->tmpl) +
         ",\"uptime_ms\":" + String(d->uptimeMs) + "}";
  }
  j += "]}";
  http::sendJson(srv, 200, j);
}

void handleDiscordTest(::WebServer& srv, App& app) {
  if (!http::requireOperatorCsrf(srv, app)) return;
  if (!app.discord().enabled()) {
    http::sendError(srv, 409, "discord_disabled");
    return;
  }
  app.connectUplink();
  bool sent = app.discord().sendTest();
  String j = String("{\"ok\":true,\"sent_now\":") + jsonBool(sent) +
             ",\"queued\":" + String((uint32_t)app.discord().pending()) + "}";
  http::sendJson(srv, 200, j);
}

}  // namespace

void registerApiRoutes(::WebServer& srv, App& app) {
  srv.on("/api/v1/session", HTTP_GET, [&srv, &app]() { handleSession(srv, app); });
  srv.on("/api/v1/login", HTTP_POST, [&srv, &app]() { handleLogin(srv, app); });
  srv.on("/api/v1/logout", HTTP_POST, [&srv, &app]() { handleLogout(srv, app); });
  srv.on("/api/v1/health", HTTP_GET, [&srv, &app]() { handleHealth(srv, app); });
  srv.on("/api/v1/modules", HTTP_GET, [&srv, &app]() { handleModules(srv, app); });
  srv.on("/api/v1/events", HTTP_GET, [&srv, &app]() { handleEvents(srv, app); });
  srv.on("/api/v1/scenarios/start", HTTP_POST, [&srv, &app]() { handleScenarioStart(srv, app); });
  srv.on("/api/v1/scenarios/stop", HTTP_POST, [&srv, &app]() { handleScenarioStop(srv, app); });
  srv.on("/api/v1/reveal/on", HTTP_POST, [&srv, &app]() { handleReveal(srv, app, true); });
  srv.on("/api/v1/reveal/off", HTTP_POST, [&srv, &app]() { handleReveal(srv, app, false); });
  srv.on("/api/v1/wifi/scan", HTTP_POST, [&srv, &app]() { handleWifiScan(srv, app); });
  srv.on("/api/v1/ble/scan", HTTP_POST, [&srv, &app]() { handleBleScan(srv, app); });
  srv.on("/api/v1/ble", HTTP_GET, [&srv, &app]() { handleBle(srv, app); });
  srv.on("/api/v1/discord/test", HTTP_POST, [&srv, &app]() { handleDiscordTest(srv, app); });

  // Configuração do AP (SSID) e do portal (template).
  srv.on("/api/v1/network/ap-ssid", HTTP_POST, [&srv, &app]() { handleApSsid(srv, app); });
  srv.on("/api/v1/portal/templates", HTTP_GET, [&srv, &app]() { handlePortalTemplates(srv, app); });
  srv.on("/api/v1/portal/template", HTTP_POST, [&srv, &app]() { handlePortalTemplateSet(srv, app); });

  // Console estilo ArmyKnife — feeds ao vivo.
  srv.on("/api/v1/recon", HTTP_GET, [&srv, &app]() { handleRecon(srv, app); });
  srv.on("/api/v1/recon/scan", HTTP_POST, [&srv, &app]() { handleReconScan(srv, app); });
  srv.on("/api/v1/recon/sniff", HTTP_POST, [&srv, &app]() { handleSniff(srv, app); });
  srv.on("/api/v1/probes", HTTP_GET, [&srv, &app]() { handleProbes(srv, app); });
  srv.on("/api/v1/dns", HTTP_GET, [&srv, &app]() { handleDns(srv, app); });
  srv.on("/api/v1/captures", HTTP_GET, [&srv, &app]() { handleCaptures(srv, app); });
  srv.on("/api/v1/devices", HTTP_GET, [&srv, &app]() { handleDevices(srv, app); });
}

}  // namespace web
}  // namespace mak
