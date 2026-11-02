// ApiRoutes.h — registra os endpoints /api/v1/* no servidor HTTP.
//
// Autorização: tudo exige sessão de operador; mutações exigem CSRF. A UI do
// participante NUNCA deve alcançar estes endpoints (plano §7).
#pragma once

#include <WebServer.h>

namespace mak {
class App;
namespace web {

void registerApiRoutes(::WebServer& srv, App& app);

}  // namespace web
}  // namespace mak
