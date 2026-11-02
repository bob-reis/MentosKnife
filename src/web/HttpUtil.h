// HttpUtil.h — auxiliares de HTTP: cookie de sessão, guarda de operador,
// verificação CSRF, limite de payload e respostas JSON compactas.
#pragma once

#include <Arduino.h>
#include <WebServer.h>

namespace mak {

class App;

namespace http {

// Extrai o valor do cookie de sessão (mak_session) do header "Cookie".
String sessionTokenFromCookie(::WebServer& srv);

// Garante que a requisição tem sessão de operador válida. Em falha, responde
// 401 e retorna false. Também aplica rate limiting administrativo.
bool requireOperator(::WebServer& srv, App& app);

// Para mutações: exige sessão válida + header CSRF correspondente. Em falha,
// responde 401/403 e retorna false.
bool requireOperatorCsrf(::WebServer& srv, App& app);

// Rejeita corpos maiores que o limite (responde 413). Retorna true se ok.
bool enforceBodyLimit(::WebServer& srv);

// Respostas utilitárias.
void sendJson(::WebServer& srv, int code, const String& json);
void sendError(::WebServer& srv, int code, const char* message);
void sendNoStore(::WebServer& srv);  // cabeçalhos anti-cache

}  // namespace http
}  // namespace mak
