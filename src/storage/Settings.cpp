#include "storage/Settings.h"

#include <Preferences.h>

#include "security/ConfigValidation.h"
#include "security/Crypto.h"

namespace mak {
namespace {
constexpr const char* kNs = "mak";
Preferences prefs;
}  // namespace

bool Settings::begin() {
  if (!prefs.begin(kNs, /*readOnly=*/false)) {
    return false;
  }
  load();
  return true;
}

void Settings::load() {
  opSalt_ = prefs.getString("op_salt", "");
  opHash_ = prefs.getString("op_hash", "");
  hasOpPw_ = opSalt_.length() > 0 && opHash_.length() == 64;
  apSsid_ = prefs.getString("ap_ssid", "");
  if (apSsid_.length() == 0) apSsid_ = cfg::kApSsid;
  portalTemplate_ = prefs.getString("pt_tpl", "visitantes");
  discordEnabled_ = prefs.getBool("dc_en", false);
  discordWebhook_ = prefs.getString("dc_url", "");
  uplinkSsid_ = prefs.getString("up_ssid", "");
  uplinkPw_ = prefs.getString("up_pw", "");
}

bool Settings::setOperatorPassword(const char* password) {
  if (!validation::isStrongOperatorPassword(password)) {
    return false;
  }
  char salt[2 * 16 + 1];
  crypto::randomHex(16, salt);
  char hash[65];
  crypto::hashPassword(password, salt, hash);
  prefs.putString("op_salt", salt);
  prefs.putString("op_hash", hash);
  opSalt_ = salt;
  opHash_ = hash;
  hasOpPw_ = true;
  return true;
}

bool Settings::checkOperatorPassword(const char* password) const {
  if (!hasOpPw_ || !password) return false;
  char hash[65];
  crypto::hashPassword(password, opSalt_.c_str(), hash);
  return crypto::constantTimeEquals(hash, opHash_.c_str());
}

bool Settings::setApSsid(const char* ssid) {
  // 1..32 chars (máximo do SSID Wi-Fi). Guardrail de não imitar a WLAN real
  // é procedural (UI/docs), pois o firmware não conhece o SSID corporativo.
  if (!ssid || !ssid[0] || strlen(ssid) > cfg::kSsidMaxLen - 1) return false;
  prefs.putString("ap_ssid", ssid);
  apSsid_ = ssid;
  return true;
}

bool Settings::setPortalTemplate(const char* id) {
  if (!validation::isAllowedTemplate(id)) return false;
  prefs.putString("pt_tpl", id);
  portalTemplate_ = id;
  return true;
}

void Settings::setDiscordEnabled(bool en) {
  discordEnabled_ = en;
  prefs.putBool("dc_en", en);
}

bool Settings::setDiscordWebhook(const char* url) {
  if (!validation::isValidDiscordWebhook(url)) {
    return false;
  }
  prefs.putString("dc_url", url);
  discordWebhook_ = url;
  return true;
}

bool Settings::setUplink(const char* ssid, const char* password) {
  if (!ssid || !ssid[0] || strlen(ssid) > 32 || (password && strlen(password) > 63)) return false;
  prefs.putString("up_ssid", ssid);
  prefs.putString("up_pw", password ? password : "");
  uplinkSsid_ = ssid;
  uplinkPw_ = password ? password : "";
  return true;
}

void Settings::clearUplink() {
  prefs.remove("up_ssid");
  prefs.remove("up_pw");
  uplinkSsid_ = "";
  uplinkPw_ = "";
}

void Settings::wipeSecrets() {
  prefs.clear();
  opSalt_ = opHash_ = "";
  discordWebhook_ = uplinkSsid_ = uplinkPw_ = "";
  apSsid_ = cfg::kApSsid;
  portalTemplate_ = "visitantes";
  hasOpPw_ = false;
  discordEnabled_ = false;
}

}  // namespace mak
