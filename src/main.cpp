// main.cpp — ponto de entrada do MentosArmyKnife-C3.
//
// Boot serial + heartbeat de LED (P0-02), inicializa o App e o console serial.
// Toda a lógica vive no App; aqui só há o laço cooperativo.
#include <Arduino.h>

#include "AppConfig.h"
#include "console/SerialConsole.h"
#include "core/App.h"

using namespace mak;

namespace {
App g_app;
SerialConsole g_console(g_app);
}  // namespace

// LED on-board da ESP32-C3 SuperMini (ativo-baixo). Ajustar após P0-01.
#ifndef MAK_LED_PIN
#define MAK_LED_PIN 8
#endif

void setup() {
  Serial.begin(115200);
  // Pequena espera só para o host enumerar a USB Serial/JTAG (fora de handler).
  uint32_t t0 = millis();
  while (!Serial && (millis() - t0) < 1500) {
  }
  pinMode(MAK_LED_PIN, OUTPUT);
  digitalWrite(MAK_LED_PIN, HIGH);  // apagado

  Serial.println();
  Serial.printf("MentosArmyKnife-C3 %s — ferramentas de rede / demonstracao autorizada\n",
                cfg::kFirmwareVersion);

  g_app.begin();
  g_console.begin();
}

void loop() {
  g_app.loop();
  g_console.loop();

  // Heartbeat de LED não bloqueante (sinal de "vivo").
  static uint32_t last = 0;
  static bool on = false;
  uint32_t now = millis();
  if ((uint32_t)(now - last) >= 1000) {
    last = now;
    on = !on;
    digitalWrite(MAK_LED_PIN, on ? LOW : HIGH);  // ativo-baixo
  }
}
