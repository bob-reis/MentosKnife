// PortalController.h — rotas do portal educativo (lado do participante).
//
// Público (sem autenticação), mas isolado: só registra interação BOOLEANA e
// nunca alcança a API administrativa. Nenhum dado digitado é lido/guardado.
#pragma once

#include <WebServer.h>

namespace mak {
class App;
namespace portal {

void registerPortalRoutes(::WebServer& srv, App& app);

}  // namespace portal
}  // namespace mak
