#include "web/HttpUtil.h"

#include "AppConfig.h"
#include "core/App.h"

namespace mak {
namespace http {

String sessionTokenFromCookie(::WebServer& srv) {
  if (!srv.hasHeader("Cookie")) return String();
  String cookie = srv.header("Cookie");
  String key = String(cfg::kSessionCookieName) + "=";
  int idx = cookie.indexOf(key);
  if (idx < 0) return String();
  int start = idx + key.length();
  int end = cookie.indexOf(';', start);
  if (end < 0) end = cookie.length();
  String tok = cookie.substring(start, end);
  tok.trim();
  return tok;
}

void sendNoStore(::WebServer& srv) {
  srv.sendHeader("Cache-Control", "no-store");
  srv.sendHeader("X-Content-Type-Options", "nosniff");
  srv.sendHeader("Referrer-Policy", "no-referrer");
}

void sendJson(::WebServer& srv, int code, const String& json) {
  sendNoStore(srv);
  srv.send(code, "application/json", json);
}

void sendError(::WebServer& srv, int code, const char* message) {
  String j = String("{\"ok\":false,\"error\":\"") + message + "\"}";
  sendJson(srv, code, j);
}

bool enforceBodyLimit(::WebServer& srv) {
  if (srv.hasArg("plain") && srv.arg("plain").length() > cfg::kMaxRequestBodyBytes) {
    sendError(srv, 413, "payload_too_large");
    return false;
  }
  return true;
}

bool requireOperator(::WebServer& srv, App& app) {
  uint32_t now = millis();
  if (!app.adminRate().allow(now)) {
    sendError(srv, 429, "rate_limited");
    return false;
  }
  String tok = sessionTokenFromCookie(srv);
  if (tok.isEmpty() || !app.auth().isValidSession(tok.c_str(), now)) {
    sendError(srv, 401, "unauthorized");
    return false;
  }
  return true;
}

bool requireOperatorCsrf(::WebServer& srv, App& app) {
  if (!requireOperator(srv, app)) {
    return false;  // já respondeu
  }
  if (!enforceBodyLimit(srv)) {
    return false;
  }
  String tok = sessionTokenFromCookie(srv);
  String csrf = srv.hasHeader(cfg::kCsrfHeaderName)
                    ? srv.header(cfg::kCsrfHeaderName)
                    : String();
  if (csrf.isEmpty() || !app.auth().checkCsrf(tok.c_str(), csrf.c_str(), millis())) {
    sendError(srv, 403, "csrf_failed");
    return false;
  }
  return true;
}

}  // namespace http
}  // namespace mak
