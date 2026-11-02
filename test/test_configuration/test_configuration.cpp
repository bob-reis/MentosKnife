// Testes de validação de configuração e allowlist (host, sem Arduino).
#include <unity.h>

#include <cstring>

#include "security/ConfigValidation.h"

using namespace mak;
using namespace mak::validation;

void test_scenario_allowlist() {
  // Os três cenários padrão dos engajamentos.
  TEST_ASSERT_TRUE(isAllowedScenario("corp-ambiente"));
  TEST_ASSERT_TRUE(isAllowedScenario("fora-empresa"));
  TEST_ASSERT_TRUE(isAllowedScenario("pessoal-familia"));
  TEST_ASSERT_EQUAL(3, scenarioCount());
  TEST_ASSERT_FALSE(isAllowedScenario("rm-rf-everything"));
  TEST_ASSERT_FALSE(isAllowedScenario(""));
  TEST_ASSERT_NULL(findScenario("desconhecido"));
}

void test_template_allowlist() {
  // Templates genéricos (sem clonar marca real), com o reforço "fictício".
  TEST_ASSERT_EQUAL(4, templateCount());
  TEST_ASSERT_TRUE(isAllowedTemplate("visitantes"));
  TEST_ASSERT_TRUE(isAllowedTemplate("wifi"));
  TEST_ASSERT_TRUE(isAllowedTemplate("corporate"));
  TEST_ASSERT_TRUE(isAllowedTemplate("hotel"));
  TEST_ASSERT_FALSE(isAllowedTemplate(""));
  TEST_ASSERT_FALSE(isAllowedTemplate("real-brand-clone"));
  TEST_ASSERT_NULL(findTemplate("desconhecido"));

  // Cada template aponta para um arquivo LittleFS válido (caminho absoluto).
  for (size_t i = 0; i < templateCount(); ++i) {
    const PortalTemplate* t = templateAt(i);
    TEST_ASSERT_NOT_NULL(t);
    TEST_ASSERT_NOT_NULL(t->id);
    TEST_ASSERT_NOT_NULL(t->label);
    TEST_ASSERT_NOT_NULL(t->file);
    TEST_ASSERT_EQUAL('/', t->file[0]);  // "/portal-*.html"
    // findTemplate deve retornar o mesmo ponteiro pelo id.
    TEST_ASSERT_EQUAL_PTR(t, findTemplate(t->id));
  }
  // Índice fora da faixa devolve nullptr.
  TEST_ASSERT_NULL(templateAt(templateCount()));
}

void test_operator_password_strength() {
  TEST_ASSERT_FALSE(isStrongOperatorPassword("curta"));
  TEST_ASSERT_FALSE(isStrongOperatorPassword("somentelowercase"));   // 1 classe
  TEST_ASSERT_FALSE(isStrongOperatorPassword("Senha12"));            // curta
  TEST_ASSERT_TRUE(isStrongOperatorPassword("Treino2026!demo"));     // 4 classes, >=12
}

void test_discord_webhook_validation() {
  TEST_ASSERT_TRUE(isValidDiscordWebhook("https://discord.com/api/webhooks/123/abc"));
  TEST_ASSERT_TRUE(isValidDiscordWebhook("https://discordapp.com/api/webhooks/1/x"));
  TEST_ASSERT_FALSE(isValidDiscordWebhook("http://discord.com/api/webhooks/1/x"));  // sem TLS
  TEST_ASSERT_FALSE(isValidDiscordWebhook("https://evil.example/api/webhooks/1/x"));
  TEST_ASSERT_FALSE(isValidDiscordWebhook("https://discord.com/channels/1"));       // sem webhook
  TEST_ASSERT_FALSE(isValidDiscordWebhook(""));
}

void test_sanitize_label() {
  char out[32];
  size_t n = sanitizeLabel("phish email <script>", out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("phish email script", out);
  TEST_ASSERT_EQUAL(strlen(out), n);
  // Truncamento respeita maxLen.
  char small[5];
  sanitizeLabel("abcdefgh", small, sizeof(small));
  TEST_ASSERT_TRUE(strlen(small) <= 4);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_scenario_allowlist);
  RUN_TEST(test_template_allowlist);
  RUN_TEST(test_operator_password_strength);
  RUN_TEST(test_discord_webhook_validation);
  RUN_TEST(test_sanitize_label);
  return UNITY_END();
}
