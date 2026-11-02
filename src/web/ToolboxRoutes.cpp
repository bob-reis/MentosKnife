#include "web/ToolboxRoutes.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <mdns.h>
#include <esp_netif.h>
#include <memory>
#include <new>
#include "core/App.h"
#include "web/HttpUtil.h"
#include "web/Json.h"

namespace mak { namespace web {
namespace {
String lastDns = "null";
String lastDiscovery = "null";
String connectionJson(App& app) {
  return "{\"connected\":" + String(app.ap().uplinkConnected() ? "true" : "false") +
    ",\"status_code\":" + String(WiFi.status()) + ",\"ssid\":" + jsonString(WiFi.SSID().c_str()) +
    ",\"ip\":" + jsonString(WiFi.localIP().toString().c_str()) +
    ",\"gateway\":" + jsonString(WiFi.gatewayIP().toString().c_str()) +
    ",\"subnet\":" + jsonString(WiFi.subnetMask().toString().c_str()) +
    ",\"dns\":" + jsonString(WiFi.dnsIP().toString().c_str()) +
    ",\"channel\":" + String(WiFi.channel()) + "}";
}
String reportJson(App& app) {
  String j;
  j.reserve(8192);
  j = "{\"schema\":1,\"firmware\":" + jsonString(cfg::kFirmwareVersion) +
    ",\"uptime_ms\":" + String(millis()) + ",\"clock\":\"uptime, not wall clock\",\"connection\":" +
    connectionJson(app) + ",\"tcp\":" + app.networkTools().json() + ",\"dns_lookup\":" + lastDns +
    ",\"discovery\":" + lastDiscovery;
  j += ",\"wifi_scan_valid\":" + String(app.recon().scanValid() ? "true" : "false") + ",\"aps\":[";
  for (size_t i = 0; i < app.recon().apCount(); ++i) {
    const ApInfo* a = app.recon().apAt(i);
    char mac[cfg::kMacStrLen]; macToStr(a->bssid, mac);
    if (i) j += ',';
    j += "{\"ssid\":" + jsonString(a->ssid) + ",\"bssid\":" + jsonString(mac) +
      ",\"channel\":" + String(a->channel) + ",\"rssi\":" + String(a->rssi) +
      ",\"auth\":" + jsonString(encToString(a->enc)) + "}";
  }
  j += "],\"clients\":[";
  for (size_t i = 0; i < app.recon().stationCount(); ++i) {
    const StaInfo* c = app.recon().stationAt(i);
    char mac[cfg::kMacStrLen]; macToStr(c->mac, mac);
    if (i) j += ',';
    j += "{\"mac\":" + jsonString(mac) + ",\"rssi\":" + String(c->rssi) + "}";
  }
  j += "],\"events\":[";
  for (size_t i = 0; i < app.events().size(); ++i) {
    const Event* e = app.events().at(i);
    if (i) j += ',';
    j += "{\"type\":" + jsonString(toString(e->type)) + ",\"uptime_ms\":" + String(e->uptimeMs) +
      ",\"count\":" + String(e->count) + ",\"label\":" + jsonString(e->scenarioId) + "}";
  }
  j += "],\"probes\":[";
  for (size_t i = 0; i < cfg::kProbeRingCap; ++i) {
    ProbeInfo p;
    if (!app.recon().copyProbe(i, p)) break;
    char mac[cfg::kMacStrLen]; macToStr(p.mac, mac);
    if (i) j += ',';
    j += "{\"mac\":" + jsonString(mac) + ",\"ssid\":" + jsonString(p.ssid) +
      ",\"rssi\":" + String(p.rssi) + ",\"count\":" + String(p.count) +
      ",\"last_ms\":" + String(p.lastMs) + "}";
  }
  j += "],\"devices\":[";
  for (size_t i = 0; i < app.deviceCount(); ++i) {
    const DeviceInfo* d = app.deviceAt(i);
    if (i) j += ',';
    j += "{\"ip\":" + jsonString(d->ip) + ",\"os\":" + jsonString(d->os) +
      ",\"browser\":" + jsonString(d->browser) +
      ",\"device\":" + jsonString(d->device) + ",\"platform\":" + jsonString(d->platform) +
      ",\"lang\":" + jsonString(d->lang) + ",\"screen\":" + jsonString(d->screen) +
      ",\"touch\":" + String(d->touch ? "true" : "false") + ",\"cores\":" + jsonString(d->cores) +
      ",\"mem\":" + jsonString(d->mem) + ",\"conn\":" + jsonString(d->conn) +
      ",\"battery\":" + jsonString(d->battery) + ",\"tz\":" + jsonString(d->tz) +
      ",\"ua\":" + jsonString(d->ua) + ",\"template\":" + jsonString(d->tmpl) +
      ",\"uptime_ms\":" + String(d->uptimeMs) + "}";
  }
  j += "],\"dns_queries\":[";
  for (size_t i = 0; i < app.dns().queryCount(); ++i) {
    const DnsQuery* q = app.dns().queryAt(i);
    if (i) j += ',';
    j += "{\"name\":" + jsonString(q->name) + ",\"count\":" + String(q->count) + "}";
  }
  j += "],\"portal_submissions\":[";
  for (size_t i = 0; i < app.captureCount(); ++i) {
    const Capture* c = app.captureAt(i);
    if (i) j += ',';
    // A senha é mascarada na exportação (nunca sai do dispositivo em claro).
    j += "{\"user\":" + jsonString(c->user) + ",\"password\":" + jsonString(cfg::kPasswordMask) +
      ",\"ip\":" + jsonString(c->ip) +
      ",\"template\":" + jsonString(c->tmpl) + ",\"uptime_ms\":" + String(c->uptimeMs) + "}";
  }
  size_t bytes; uint32_t packets, dropped;
  app.recon().pcapStats(bytes, packets, dropped);
  j += "],\"pcap\":{\"scope\":\"probe requests only, current radio channel\",\"bytes\":" +
    String(bytes) + ",\"packets\":" + String(packets) + ",\"dropped\":" + String(dropped) + "}}";
  return j;
}
}

void registerToolboxRoutes(::WebServer& srv, App& app) {
  srv.on("/api/v1/network", HTTP_GET, [&]() {
    if (!http::requireOperator(srv, app)) return;
    http::sendJson(srv, 200, "{\"connection\":" + connectionJson(app) +
      ",\"tcp\":" + app.networkTools().json() + ",\"dns_lookup\":" + lastDns +
      ",\"discovery\":" + lastDiscovery + "}");
  });
  srv.on("/api/v1/network/connect", HTTP_POST, [&]() {
    if (!http::requireOperatorCsrf(srv, app)) return;
    const String ssid = srv.arg("ssid"), password = srv.arg("password");
    if (!ssid.length() || ssid.length() > 32 || password.length() > 63 ||
        (password.length() && password.length() < 8)) {
      http::sendError(srv, 400, "invalid_wifi_credentials"); return;
    }
    app.networkTools().stop();
    app.reconSniff(false);
    if (!app.provisionUplink(ssid.c_str(), password.c_str()) || !app.connectUplink()) {
      http::sendError(srv, 409, "connect_failed"); return;
    }
    http::sendJson(srv, 202, "{\"ok\":true,\"state\":\"connecting\"}");
  });
  srv.on("/api/v1/network/disconnect", HTTP_POST, [&]() {
    if (!http::requireOperatorCsrf(srv, app)) return;
    app.networkTools().stop(); app.disconnectUplink();
    http::sendJson(srv, 200, "{\"ok\":true}");
  });
  srv.on("/api/v1/network/forget", HTTP_POST, [&]() {
    if (!http::requireOperatorCsrf(srv, app)) return;
    app.networkTools().stop(); app.forgetUplink();
    http::sendJson(srv, 200, "{\"ok\":true}");
  });
  srv.on("/api/v1/network/scan", HTTP_POST, [&]() {
    if (!http::requireOperatorCsrf(srv, app)) return;
    if (!app.networkTools().start(srv.arg("target").c_str())) {
      http::sendError(srv, 409, "need_idle_scanner_and_host_in_connected_subnet"); return;
    }
    app.discord().notifyText("Recon TCP — iniciado", app.networkTools().summary().c_str(), 3447003);
    http::sendJson(srv, 202, app.networkTools().json());
  });
  srv.on("/api/v1/network/stop", HTTP_POST, [&]() {
    if (!http::requireOperatorCsrf(srv, app)) return;
    app.networkTools().stop();
    http::sendJson(srv, 200, app.networkTools().json());
  });
  srv.on("/api/v1/network/dns", HTTP_POST, [&]() {
    if (!http::requireOperatorCsrf(srv, app)) return;
    String host = srv.arg("host");
    bool valid = host.length() > 0 && host.length() <= 253;
    for (size_t i = 0; i < host.length(); ++i) {
      char c = host[i];
      if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '.')) valid = false;
    }
    if (!valid || !app.ap().uplinkConnected()) {
      http::sendError(srv, 400, "need_connected_wifi_and_valid_hostname"); return;
    }
    IPAddress ip;
    bool ok = WiFi.hostByName(host.c_str(), ip) == 1;
    lastDns = "{\"host\":" + jsonString(host.c_str()) + ",\"ok\":" + (ok ? "true" : "false") +
      ",\"ip\":" + jsonString(ip.toString().c_str()) + ",\"uptime_ms\":" + String(millis()) + "}";
    String summary = host + (ok ? " → " + ip.toString() : " → resolução falhou");
    app.discord().notifyText("Consulta DNS", summary.c_str(), ok ? 1752220 : 15158332);
    http::sendJson(srv, 200, lastDns);
  });
  srv.on("/api/v1/network/discover", HTTP_POST, [&]() {
    if (!http::requireOperatorCsrf(srv, app)) return;
    const String service = srv.arg("service");
    if (!app.ap().uplinkConnected() || (service != "http" && service != "https" &&
        service != "ipp" && service != "ssh" && service != "smb" && service != "workstation")) {
      http::sendError(srv, 400, "need_connected_wifi_and_supported_service"); return;
    }
    if (!MDNS.begin("mentosarmyknife")) { http::sendError(srv, 503, "mdns_init_failed"); return; }
    mdns_result_t* results = nullptr;
    const String type = "_" + service;
    const esp_err_t result = mdns_query_ptr(type.c_str(), "_tcp", 1500, 12, &results);
    String j = "{\"service\":" + jsonString(service.c_str()) + ",\"ok\":" +
      (result == ESP_OK ? "true" : "false") + ",\"uptime_ms\":" + String(millis()) + ",\"hosts\":[";
    size_t count = 0;
    const esp_netif_t* sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    for (mdns_result_t* r = results; r && count < 12; r = r->next) {
      if (r->esp_netif != sta) continue;
      for (mdns_ip_addr_t* a = r->addr; a && count < 12; a = a->next) {
        if (a->addr.type != ESP_IPADDR_TYPE_V4) continue;
        if (count++) j += ',';
        j += "{\"host\":" + jsonString(String(r->hostname ? r->hostname : "").substring(0, 128).c_str()) +
          ",\"ip\":" + jsonString(IPAddress(a->addr.u_addr.ip4.addr).toString().c_str()) +
          ",\"port\":" + String(r->port) + "}";
      }
    }
    mdns_query_results_free(results); MDNS.end();
    lastDiscovery = j + "]}";
    String summary = service + ": " + String(count) + " anúncio(s) IPv4 no uplink";
    if (result != ESP_OK) summary = "Falha na consulta de " + service;
    app.discord().notifyText("Descoberta mDNS", summary.c_str(), 3447003);
    http::sendJson(srv, 200, lastDiscovery);
  });
  srv.on("/api/v1/reports/download", HTTP_GET, [&]() {
    if (!http::requireOperator(srv, app)) return;
    srv.sendHeader("Content-Disposition", "attachment; filename=mentos-report.json");
    http::sendJson(srv, 200, reportJson(app));
  });
  srv.on("/api/v1/reports/discord", HTTP_POST, [&]() {
    if (!http::requireOperatorCsrf(srv, app)) return;
    if (!app.discord().enabled()) { http::sendError(srv, 409, "discord_disabled"); return; }
    String summary = "IP: " + WiFi.localIP().toString() + " · gateway: " + WiFi.gatewayIP().toString() +
      "\nRedes observadas: " + String(app.recon().apCount()) + "\n" + app.networkTools().summary();
    if (app.captureCount()) {
      const Capture* c = app.captureAt(app.captureCount() - 1);
      summary += "\nÚltimo envio do portal de laboratório\nUsuário: " + String(c->user) +
        "\nTemplate: " + String(c->tmpl) + "\nSenha: " + String(cfg::kPasswordMask) + "\n";
    }
    if (!app.discord().queueReport(reportJson(app), summary)) {
      http::sendError(srv, 409, "report_pending_or_too_large"); return;
    }
    http::sendJson(srv, 202, "{\"ok\":true,\"state\":\"queued\"}");
  });
  srv.on("/api/v1/reports/pcap", HTTP_GET, [&]() {
    if (!http::requireOperator(srv, app)) return;
    std::unique_ptr<uint8_t[]> buffer(new (std::nothrow) uint8_t[PcapBuffer::kCapacity]);
    if (!buffer) { http::sendError(srv, 503, "not_enough_memory"); return; }
    const size_t length = app.recon().copyPcap(buffer.get(), PcapBuffer::kCapacity);
    http::sendNoStore(srv);
    srv.sendHeader("Content-Disposition", "attachment; filename=mentos-probes.pcap");
    srv.setContentLength(length);
    srv.send(200, "application/vnd.tcpdump.pcap", "");
    srv.sendContent(reinterpret_cast<const char*>(buffer.get()), length);
  });
  srv.on("/api/v1/reports/clear", HTTP_POST, [&]() {
    if (!http::requireOperatorCsrf(srv, app)) return;
    app.wipeLiveData(); app.wipeTelemetry(); lastDns = lastDiscovery = "null"; app.discord().stop();
    http::sendJson(srv, 200, "{\"ok\":true}");
  });
}
}}
