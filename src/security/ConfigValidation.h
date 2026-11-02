// ConfigValidation.h — validação de parâmetros e allowlist de cenários.
//
// Regra (plano §4/§7): a API valida autorização, modo e parâmetros; o
// dispatcher só executa cenários allowlisted. Nunca executar código/script
// enviado pelo visitante. Código puro — testável no host.
#pragma once

#include <cstddef>
#include <cstdint>

namespace mak {

// IDs de cenário permitidos. Qualquer outro valor é recusado.
// Mantido pequeno e explícito; adicionar aqui é a única forma de habilitar um
// cenário novo (revisável em code review).
struct ScenarioInfo {
  const char* id;
  const char* label;
  uint32_t autoStopMs;  // trava de segurança por cenário
};

// Template do portal educativo (visual do formulário de login fictício).
struct PortalTemplate {
  const char* id;     // ex.: "wifi", "corporate", "hotel"
  const char* label;  // rótulo legível para a UI
  const char* file;   // arquivo no LittleFS (ex.: "/portal-wifi.html")
};

namespace validation {

// Retorna o ScenarioInfo se `id` estiver na allowlist, senão nullptr.
const ScenarioInfo* findScenario(const char* id);

// Nº de cenários na allowlist (para /api/v1/modules e UI).
size_t scenarioCount();
const ScenarioInfo* scenarioAt(size_t index);

// true se `id` é um rótulo de cenário válido (allowlisted).
bool isAllowedScenario(const char* id);

// Templates do portal: allowlist id/label/arquivo (para API e UI).
size_t templateCount();
const PortalTemplate* templateAt(size_t index);
const PortalTemplate* findTemplate(const char* id);
bool isAllowedTemplate(const char* id);

// true se a senha do operador atende à política (comprimento + variedade).
bool isStrongOperatorPassword(const char* pw);

// Valida uma URL de webhook do Discord (esquema https e host discord(app).com).
bool isValidDiscordWebhook(const char* url);

// Sanitiza um rótulo curto para uso em JSON/log: mantém apenas
// [A-Za-z0-9 _-.], trunca em `maxLen`. Retorna o tamanho escrito.
size_t sanitizeLabel(const char* in, char* out, size_t maxLen);

}  // namespace validation
}  // namespace mak
