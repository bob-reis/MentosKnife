// WebServer.h — servidor HTTP do dispositivo.
//
// Serve o dashboard estático (LittleFS), registra a API administrativa e o
// portal do participante, e trata a detecção de captive portal com fallback de
// URL manual (o pop-up varia entre Android/iOS — plano §8).
#pragma once

#include <WebServer.h>

namespace mak {

class App;

class WebServer {
 public:
  explicit WebServer(App& app) : app_(app), server_(cfg_httpPort()) {}

  bool begin();
  void loop();
  void stop();

 private:
  static uint16_t cfg_httpPort();
  void registerCaptiveAndStatic();

  App& app_;
  ::WebServer server_;
  bool up_ = false;
};

}  // namespace mak
