#include "web/WebServer.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>

#include "AppConfig.h"
#include "core/App.h"
#include "portal/PortalController.h"
#include "web/ApiRoutes.h"
#include "web/ToolboxRoutes.h"
#include "web/HttpUtil.h"

namespace mak {

uint16_t WebServer::cfg_httpPort() { return cfg::kHttpPort; }

namespace {
String portalUrl() {
  return String("http://") + WiFi.softAPIP().toString() + "/portal";
}
}  // namespace

bool WebServer::begin() {
  if (!LittleFS.begin(/*formatOnFail=*/false)) {
    // Sem FS: ainda servimos fallbacks embutidos; registra aviso no serial.
#if TRAINING_MODE
    Serial.println("[web] LittleFS indisponivel; usando conteudo embutido");
#endif
  }

  // Headers necessários para sessão/CSRF (devem ser coletados antes do begin).
  const char* headers[] = {"Cookie", cfg::kCsrfHeaderName};
  server_.collectHeaders(headers, sizeof(headers) / sizeof(headers[0]));

  // Dashboard estático (sem segredos). A API é que exige autenticação.
  server_.on("/", HTTP_GET, [this]() {
    if (LittleFS.exists("/index.html")) {
      File f = LittleFS.open("/index.html", "r");
      http::sendNoStore(server_);
      server_.streamFile(f, "text/html");
      f.close();
    } else {
      http::sendNoStore(server_);
      server_.send(200, "text/html",
                   "<!doctype html><meta charset=utf-8><title>MentosArmyKnife-C3</title>"
                   "<h1>Dashboard indisponivel</h1><p>Envie data/ para o LittleFS "
                   "(pio run -t uploadfs).</p>");
    }
  });
  server_.serveStatic("/style.css", LittleFS, "/style.css");
  server_.serveStatic("/app.js", LittleFS, "/app.js");
  server_.serveStatic("/qrcode.min.js", LittleFS, "/qrcode.min.js");
  // Coletor de fingerprint usado pelos templates do portal.
  server_.serveStatic("/portal-fp.js", LittleFS, "/portal-fp.js");

  registerCaptiveAndStatic();
  web::registerApiRoutes(server_, app_);
  web::registerToolboxRoutes(server_, app_);
  portal::registerPortalRoutes(server_, app_);

  server_.begin();
  up_ = true;
  return true;
}

void WebServer::registerCaptiveAndStatic() {
  auto toPortal = [this]() {
    server_.sendHeader("Location", portalUrl(), true);
    server_.send(302, "text/plain", "");
  };
  // Android
  server_.on("/generate_204", HTTP_GET, toPortal);
  server_.on("/gen_204", HTTP_GET, toPortal);
  // Windows NCSI
  server_.on("/ncsi.txt", HTTP_GET, toPortal);
  server_.on("/connecttest.txt", HTTP_GET, toPortal);
  // Apple
  server_.on("/hotspot-detect.html", HTTP_GET, [this]() {
    http::sendNoStore(server_);
    server_.send(200, "text/html",
                 String("<!doctype html><meta http-equiv=refresh content='0;url=") +
                     portalUrl() + "'>");
  });
  // Qualquer outra rota desconhecida leva ao portal (comportamento cativo).
  server_.onNotFound([this, toPortal]() { toPortal(); });
}

void WebServer::loop() {
  if (up_) server_.handleClient();
}

void WebServer::stop() {
  if (up_) {
    server_.stop();
    up_ = false;
  }
}

}  // namespace mak
