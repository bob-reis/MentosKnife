#include "console/SerialConsole.h"

#include "core/App.h"
#include <WiFi.h>

namespace mak {
namespace {
// Divide "cmd resto da linha" em (cmd, arg) — arg é o restante cru.
void splitFirst(const String& line, String& cmd, String& arg) {
  int sp = line.indexOf(' ');
  if (sp < 0) {
    cmd = line;
    arg = "";
  } else {
    cmd = line.substring(0, sp);
    arg = line.substring(sp + 1);
    arg.trim();
  }
}
}  // namespace

void SerialConsole::begin() {
  printHelp();
  Serial.print("mak> ");
}

void SerialConsole::printHelp() {
  Serial.println();
  Serial.println("=== MentosArmyKnife-C3 — console de treinamento ===");
  Serial.println("help                      esta ajuda");
  Serial.println("status                    estado, heap, AP, discord");
  Serial.println("set-op-pass <senha>       senha do operador (>=12, 3+ classes)");
  Serial.println("set-discord-url <url>     webhook do Discord (segredo)");
  Serial.println("set-uplink <ssid> <senha> uplink STA para ferramentas e Discord");
  Serial.println("discord on|off|test       habilita/desabilita/testa o feed");
  Serial.println("start                     sobe AP + portal + HTTP");
  Serial.println("scenario <id>             inicia cenario allowlisted");
  Serial.println("reveal on|off             revelacao educativa");
  Serial.println("wifi-scan | ble-scan      varredura passiva (fora da demo)");
  Serial.println("recon                     varre APs + lista clientes do AP");
  Serial.println("sniff on|off              sniffer de probe requests (promiscuo)");
  Serial.println("probes | dns | caps       lista probes / consultas DNS / capturas");
  Serial.println("stop                      STOP global; preserva resultados");
  Serial.println("reboot                    reinicia o dispositivo (re-arma após STOP)");
  Serial.println("wipe-secrets              apaga credenciais (factory reset)");
  Serial.println("===================================================");
}

void SerialConsole::printStatus() {
  App& a = app_;
  Serial.printf("fw=%s state=%s heap=%u ap_clients=%u scenario='%s'\n",
                cfg::kFirmwareVersion, toString(a.state().current()),
                (unsigned)ESP.getFreeHeap(), a.ap().clientCount(), a.currentScenario());
  Serial.printf("discord: enabled=%d uplink=%d pending=%u\n",
                a.discord().enabled() ? 1 : 0, a.ap().uplinkConnected() ? 1 : 0,
                (unsigned)a.discord().pending());
  Serial.printf("network: ip=%s gateway=%s dns=%s\n", WiFi.localIP().toString().c_str(),
                WiFi.gatewayIP().toString().c_str(), WiFi.dnsIP().toString().c_str());
  Serial.printf("report: state=%s http=%d dropped=%u\n", a.discord().reportState(),
                a.discord().lastHttpCode(), (unsigned)a.discord().dropped());
  const Telemetry& t = a.telemetry();
  Serial.printf("telemetry: demo=%u scenarios=%u reveals=%u wifi=%u ble=%u\n",
                (unsigned)t.demoInteractions, (unsigned)t.scenariosStarted,
                (unsigned)t.reveals, (unsigned)t.wifiScans, (unsigned)t.bleScans);
}

void SerialConsole::handleLine(const String& lineIn) {
  String line = lineIn;
  line.trim();
  if (line.isEmpty()) return;

  String cmd, arg;
  splitFirst(line, cmd, arg);

  if (cmd == "help") {
    printHelp();
  } else if (cmd == "status") {
    printStatus();
  } else if (cmd == "set-op-pass") {
    Serial.println(app_.provisionOperatorPassword(arg.c_str())
                       ? "ok: senha do operador definida"
                       : "ERRO: senha fraca (>=12 chars, 3+ classes)");
  } else if (cmd == "set-discord-url") {
    Serial.println(app_.provisionDiscordWebhook(arg.c_str())
                       ? "ok: webhook definido"
                       : "ERRO: webhook invalido (https + discord.com/api/webhooks/)");
  } else if (cmd == "set-uplink") {
    String ssid, pass;
    splitFirst(arg, ssid, pass);
    Serial.println(app_.provisionUplink(ssid.c_str(), pass.c_str())
                       ? "ok: uplink definido"
                       : "ERRO: ssid vazio");
  } else if (cmd == "discord") {
    if (arg == "on") {
      app_.setDiscordEnabled(true);
      Serial.println("ok: discord habilitado");
    } else if (arg == "off") {
      app_.setDiscordEnabled(false);
      Serial.println("ok: discord desabilitado");
    } else if (arg == "test") {
      bool now = app_.discord().sendTest();
      Serial.printf("teste: enviado_agora=%d (senao, enfileirado)\n", now ? 1 : 0);
    } else {
      Serial.println("uso: discord on|off|test");
    }
  } else if (cmd == "start") {
    Serial.println(app_.startNetwork() ? "ok: rede no ar" : "ERRO: falha ao subir AP/portal");
  } else if (cmd == "scenario") {
    Serial.println(app_.startScenario(arg.c_str())
                       ? "ok: cenario iniciado"
                       : "ERRO: cenario fora da allowlist ou estado invalido");
  } else if (cmd == "reveal") {
    bool ok = (arg == "on") ? app_.activateReveal() : app_.endReveal();
    Serial.println(ok ? "ok" : "ERRO: estado invalido");
  } else if (cmd == "wifi-scan") {
    WifiScanSummary s;
    if (app_.runWifiScan(s))
      Serial.printf("wifi: redes=%u open=%u wpa=%u rssi_max=%d\n", s.networksSeen, s.open,
                    s.wpa, s.strongestRssi);
    else
      Serial.println("ERRO: scan nao permitido neste estado");
  } else if (cmd == "ble-scan") {
    BleScanSummary s;
    if (app_.runBleScan(s))
      Serial.printf("ble: anuncios=%u connectable=%u rssi_max=%d\n", s.advertisersSeen,
                    s.connectable, s.strongestRssi);
    else
      Serial.println("ERRO: scan nao permitido neste estado");
  } else if (cmd == "recon") {
    size_t aps = app_.reconScan();
    Serial.printf("recon: %u APs, %u clientes\n", (unsigned)aps,
                  (unsigned)app_.recon().stationCount());
    for (size_t i = 0; i < app_.recon().apCount() && i < 10; ++i) {
      const ApInfo* a = app_.recon().apAt(i);
      char b[cfg::kMacStrLen]; macToStr(a->bssid, b);
      Serial.printf("  %-20s %s ch%u %ddBm %s\n", a->ssid[0] ? a->ssid : "(oculto)", b,
                    a->channel, a->rssi, encToString(a->enc));
    }
    for (size_t i = 0; i < app_.recon().stationCount(); ++i) {
      const StaInfo* s = app_.recon().stationAt(i);
      char m[cfg::kMacStrLen]; macToStr(s->mac, m);
      Serial.printf("  cliente %s %ddBm\n", m, s->rssi);
    }
  } else if (cmd == "sniff") {
    app_.reconSniff(arg == "on");
    Serial.printf("sniff: %s\n", app_.recon().sniffing() ? "ON" : "OFF");
  } else if (cmd == "probes") {
    Serial.printf("probes: %u\n", (unsigned)app_.recon().probeCount());
    for (size_t i = 0; i < app_.recon().probeCount() && i < 20; ++i) {
      const ProbeInfo* p = app_.recon().probeAt(i);
      char m[cfg::kMacStrLen]; macToStr(p->mac, m);
      Serial.printf("  %s procura '%s' %ddBm x%u\n", m, p->ssid[0] ? p->ssid : "(wildcard)",
                    p->rssi, p->count);
    }
  } else if (cmd == "dns") {
    Serial.printf("dns: %u consultas\n", (unsigned)app_.dns().queryCount());
    for (size_t i = 0; i < app_.dns().queryCount() && i < 20; ++i) {
      const DnsQuery* q = app_.dns().queryAt(i);
      Serial.printf("  %s x%u\n", q->name, q->count);
    }
  } else if (cmd == "caps") {
    Serial.printf("capturas: %u\n", (unsigned)app_.captureCount());
    for (size_t i = 0; i < app_.captureCount(); ++i) {
      const Capture* c = app_.captureAt(i);
      Serial.printf("  user='%s' pass='%s'\n", c->user, c->pass);
    }
  } else if (cmd == "stop") {
    app_.stopAll("serial_stop");
    Serial.println("ok: STOP, resultados preservados");
  } else if (cmd == "reboot") {
    Serial.println("ok: reiniciando...");
    Serial.flush();
    ESP.restart();
  } else if (cmd == "wipe-secrets") {
    app_.settings().wipeSecrets();
    Serial.println("ok: segredos apagados (reinicie o dispositivo)");
  } else {
    Serial.println("comando desconhecido — 'help'");
  }
}

void SerialConsole::loop() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      String line = buf_;
      buf_ = "";
      handleLine(line);
      Serial.print("mak> ");
    } else if (buf_.length() < 160) {
      buf_ += c;
    }
  }
}

}  // namespace mak
