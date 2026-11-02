#include "portal/PortalController.h"

#include <Arduino.h>
#include <LittleFS.h>

#include "AppConfig.h"
#include "core/App.h"
#include "portal/PortalEvents.h"
#include "security/ConfigValidation.h"
#include "web/HttpUtil.h"

namespace mak {
namespace portal {
namespace {

void serveFileOr(::WebServer& srv, const char* path, const char* fallbackHtml) {
  if (LittleFS.exists(path)) {
    File f = LittleFS.open(path, "r");
    http::sendNoStore(srv);
    srv.streamFile(f, "text/html");
    f.close();
  } else {
    http::sendNoStore(srv);
    srv.send(200, "text/html", fallbackHtml);
  }
}

void handleDemo(::WebServer& srv, App& app) {
  static const char* kFallback =
      "<!doctype html><meta charset=utf-8><title>Treinamento</title>"
      "<header style='background:#b91c1c;color:white;padding:20px'>"
      "TREINAMENTO — NÃO USE DADOS REAIS</header>"
      "<h1>Simulação indisponível</h1><p>Solicite ao operador a atualização do portal.</p>";
  const PortalTemplate* t =
      validation::findTemplate(app.settings().portalTemplate().c_str());
  serveFileOr(srv, t ? t->file : "/portal-visitantes.html", kFallback);
}

void handleSubmit(::WebServer& srv, App& app) {
  if (!http::enforceBodyLimit(srv)) return;
  // Portal de simulação: captura login/senha fictícios localmente. A senha é
  // mascarada em exportações/relatórios (ver cfg::kPasswordMask) e nunca é
  // exfiltrada em texto claro.
  if (!srv.hasArg("user") || !srv.hasArg("pass")) {
    http::sendError(srv, 400, "login_fields_required");
    return;
  }
  const String user = srv.arg("user");
  const String pass = srv.arg("pass");
  String sid = srv.arg("sid");
  if (sid.length() >= cfg::kSidLen) sid = sid.substring(0, cfg::kSidLen - 1);
  if (user.length() == 0 || user.length() >= cfg::kCredFieldLen ||
      pass.length() == 0 || pass.length() >= cfg::kCredFieldLen) {
    http::sendError(srv, 400, "invalid_field_length");
    return;
  }
  const String cip = srv.client().remoteIP().toString();
  const bool recorded = app.captureCredential(user.c_str(), pass.c_str(), sid.c_str(), cip.c_str());
  bool reveal = app.state().current() == State::kReveal;
  String j = PortalEvents::stateJson(recorded, reveal, app.currentScenario());
  http::sendJson(srv, recorded ? 200 : 409, j);
}

// Copia um argumento do formulário para um buffer de tamanho fixo (truncado).
void copyArg(::WebServer& srv, const char* name, char* dst, size_t size) {
  const String v = srv.arg(name);
  strncpy(dst, v.c_str(), size - 1);
  dst[size - 1] = '\0';
}

void handleTelemetry(::WebServer& srv, App& app) {
  // Coleta silenciosa do fingerprint enviado pelo JS do portal. Sem credencial;
  // apenas dados que o navegador expõe sem permissão. Efêmero em RAM.
  if (!http::enforceBodyLimit(srv)) return;
  DeviceInfo d{};
  copyArg(srv, "os", d.os, sizeof(d.os));
  copyArg(srv, "browser", d.browser, sizeof(d.browser));
  copyArg(srv, "device", d.device, sizeof(d.device));
  copyArg(srv, "platform", d.platform, sizeof(d.platform));
  copyArg(srv, "lang", d.lang, sizeof(d.lang));
  copyArg(srv, "screen", d.screen, sizeof(d.screen));
  copyArg(srv, "cores", d.cores, sizeof(d.cores));
  copyArg(srv, "mem", d.mem, sizeof(d.mem));
  copyArg(srv, "conn", d.conn, sizeof(d.conn));
  copyArg(srv, "battery", d.battery, sizeof(d.battery));
  copyArg(srv, "tz", d.tz, sizeof(d.tz));
  copyArg(srv, "ua", d.ua, sizeof(d.ua));
  copyArg(srv, "sid", d.sid, sizeof(d.sid));
  d.touch = srv.arg("touch") == "1";
  // IP que o cliente recebeu no SoftAP (DHCP), a partir da conexão HTTP.
  const String cip = srv.client().remoteIP().toString();
  strncpy(d.ip, cip.c_str(), sizeof(d.ip) - 1);
  d.ip[sizeof(d.ip) - 1] = '\0';
  // Aceita silenciosamente mesmo fora do gate (responde 204 sem revelar nada
  // ao participante); o registro em si respeita o estado em captureDeviceInfo.
  app.captureDeviceInfo(d);
  http::sendNoStore(srv);
  srv.send(204, "text/plain", "");
}

void handleState(::WebServer& srv, App& app) {
  bool accepting = app.state().current() == State::kPortalReady ||
                   app.state().acceptsDemoInteraction();
  bool reveal = app.state().current() == State::kReveal;
  http::sendJson(srv, 200,
                 PortalEvents::stateJson(accepting, reveal, app.currentScenario()));
}

}  // namespace

void registerPortalRoutes(::WebServer& srv, App& app) {
  srv.on("/portal", HTTP_GET, [&srv, &app]() { handleDemo(srv, app); });
  srv.on("/portal/submit", HTTP_POST, [&srv, &app]() { handleSubmit(srv, app); });
  srv.on("/portal/telemetry", HTTP_POST, [&srv, &app]() { handleTelemetry(srv, app); });
  srv.on("/portal/state", HTTP_GET, [&srv, &app]() { handleState(srv, app); });
}

}  // namespace portal
}  // namespace mak
