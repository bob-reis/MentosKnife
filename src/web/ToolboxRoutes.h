#pragma once
#include <WebServer.h>
namespace mak {
class App;
namespace web { void registerToolboxRoutes(::WebServer& srv, App& app); }
}
