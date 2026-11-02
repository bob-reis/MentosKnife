// SerialConsole.h — console serial para provisionamento e controle admin.
//
// Acesso físico (USB) é a forma recomendada de administrar o dispositivo na
// fase sem participantes (plano §7/§16). Não ecoa segredos. Não bloqueante.
#pragma once

#include <Arduino.h>

namespace mak {

class App;

class SerialConsole {
 public:
  explicit SerialConsole(App& app) : app_(app) {}

  void begin();
  void loop();  // lê linhas não bloqueante e executa comandos

 private:
  void handleLine(const String& line);
  void printHelp();
  void printStatus();

  App& app_;
  String buf_;
};

}  // namespace mak
