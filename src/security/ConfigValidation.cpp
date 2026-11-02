#include "security/ConfigValidation.h"

#include <cctype>
#include <cstring>

#include "AppConfig.h"

namespace mak {
namespace {

// Allowlist de cenários do treinamento. Determinísticos e educativos.
// Os três contextos padrão dos engajamentos no cliente:
//   1) ambiente corporativo
//   2) profissional fora da empresa (hoteis, eventos, cafes)
//   3) vida pessoal e familia
constexpr ScenarioInfo kScenarios[] = {
    {"corp-ambiente", "Ambiente corporativo", 5 * 60 * 1000UL},
    {"fora-empresa", "Fora da empresa (hoteis, eventos, cafes)", 5 * 60 * 1000UL},
    {"pessoal-familia", "Pessoal e familia", 5 * 60 * 1000UL},
};
constexpr size_t kScenarioN = sizeof(kScenarios) / sizeof(kScenarios[0]);

// Templates do portal educativo. Genéricos (sem clonar marca real), todos
// com o reforço "fictício" e o mesmo POST em /portal/submit.
constexpr PortalTemplate kTemplates[] = {
    {"visitantes", "WiFi Visitantes (padrão)", "/portal-visitantes.html"},
    {"wifi", "Rede Wi-Fi / café", "/portal-wifi.html"},
    {"corporate", "SSO corporativo", "/portal-corporate.html"},
    {"hotel", "Rede de hotel", "/portal-hotel.html"},
};
constexpr size_t kTemplateN = sizeof(kTemplates) / sizeof(kTemplates[0]);

}  // namespace

namespace validation {

const ScenarioInfo* findScenario(const char* id) {
  if (!id) return nullptr;
  for (size_t i = 0; i < kScenarioN; ++i) {
    if (std::strncmp(id, kScenarios[i].id, cfg::kScenarioIdMaxLen) == 0) {
      return &kScenarios[i];
    }
  }
  return nullptr;
}

size_t scenarioCount() { return kScenarioN; }

const ScenarioInfo* scenarioAt(size_t index) {
  return index < kScenarioN ? &kScenarios[index] : nullptr;
}

bool isAllowedScenario(const char* id) { return findScenario(id) != nullptr; }

size_t templateCount() { return kTemplateN; }

const PortalTemplate* templateAt(size_t index) {
  return index < kTemplateN ? &kTemplates[index] : nullptr;
}

const PortalTemplate* findTemplate(const char* id) {
  if (!id) return nullptr;
  for (size_t i = 0; i < kTemplateN; ++i) {
    if (std::strncmp(id, kTemplates[i].id, cfg::kTemplateIdLen) == 0) {
      return &kTemplates[i];
    }
  }
  return nullptr;
}

bool isAllowedTemplate(const char* id) { return findTemplate(id) != nullptr; }

bool isStrongOperatorPassword(const char* pw) {
  if (!pw) return false;
  size_t len = std::strlen(pw);
  if (len < cfg::kOperatorPasswordMinLen) return false;
  bool lower = false, upper = false, digit = false, other = false;
  for (size_t i = 0; i < len; ++i) {
    unsigned char c = (unsigned char)pw[i];
    if (std::islower(c)) lower = true;
    else if (std::isupper(c)) upper = true;
    else if (std::isdigit(c)) digit = true;
    else other = true;
  }
  // Exige ao menos três classes de caracteres.
  int classes = (int)lower + (int)upper + (int)digit + (int)other;
  return classes >= 3;
}

bool isValidDiscordWebhook(const char* url) {
  if (!url) return false;
  // Deve começar com https:// (nunca http — evita vazar em claro).
  const char* kPrefix = "https://";
  if (std::strncmp(url, kPrefix, std::strlen(kPrefix)) != 0) return false;
  const char* host = url + std::strlen(kPrefix);
  // Aceita discord.com e discordapp.com; exige o caminho de webhooks.
  bool hostOk = std::strncmp(host, "discord.com/", 12) == 0 ||
                std::strncmp(host, "discordapp.com/", 15) == 0 ||
                std::strncmp(host, "canary.discord.com/", 19) == 0;
  if (!hostOk) return false;
  return std::strstr(url, "/api/webhooks/") != nullptr;
}

size_t sanitizeLabel(const char* in, char* out, size_t maxLen) {
  if (!out || maxLen == 0) return 0;
  size_t w = 0;
  if (in) {
    for (size_t i = 0; in[i] != '\0' && w + 1 < maxLen; ++i) {
      unsigned char c = (unsigned char)in[i];
      if (std::isalnum(c) || c == ' ' || c == '_' || c == '-' || c == '.') {
        out[w++] = (char)c;
      }
    }
  }
  out[w] = '\0';
  return w;
}

}  // namespace validation
}  // namespace mak
