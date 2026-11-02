// AppConfig.h — constantes de compilação e limites do MentosArmyKnife-C3.
//
// Regras de segurança refletidas aqui:
//  - NÃO existe senha de operador embutida no binário. A senha é provisionada
//    por serial no primeiro boot (ver SerialConsole / Auth) e guardada com
//    hash+salt na NVS. Sem senha definida, o login web fica desabilitado.
//  - SSID de treinamento é fixo e explícito; NUNCA imita a WLAN real.
//  - Nenhum identificador persistente (MAC/IP/SSID corporativo/payload) é
//    gravado; a telemetria em RAM é apagada ao limpar/desligar.
#pragma once

#include <cstddef>
#include <cstdint>

namespace mak {
namespace cfg {

// ---- Identificação -------------------------------------------------------
#ifndef MAK_FW_VERSION
#define MAK_FW_VERSION "0.0.0-unknown"
#endif
constexpr const char* kFirmwareVersion = MAK_FW_VERSION;
constexpr const char* kDeviceName = "MentosArmyKnife-C3";

// ---- SoftAP --------------------------------------------------------------
// SSID de treinamento explícito; jamais configurar para imitar a rede do
// cliente (ver docs/security.md, item de autorização).
constexpr const char* kApSsid = "WiFi Visitantes";
// IP fixo do portal/dashboard no AP isolado.
constexpr uint8_t kApIp[4] = {192, 168, 4, 1};
constexpr uint8_t kApNetmask[4] = {255, 255, 255, 0};
constexpr uint8_t kApChannel = 6;
// Máximo de clientes simultâneos no AP (telefone de teste + operador).
constexpr uint8_t kApMaxClients = 4;
// O SoftAP de visitantes é sempre aberto (sem senha de rádio); não há
// política de senha de AP.

// ---- Servidor HTTP -------------------------------------------------------
constexpr uint16_t kHttpPort = 80;
constexpr uint16_t kDnsPort = 53;
// Limite de corpo de requisição aceito (bytes). Protege a RAM do C3.
constexpr size_t kMaxRequestBodyBytes = 1024;

// ---- Autenticação / sessão ----------------------------------------------
constexpr const char* kSessionCookieName = "mak_session";
constexpr const char* kCsrfHeaderName = "X-MAK-CSRF";
constexpr size_t kTokenHexLen = 32;            // 16 bytes aleatórios em hex
constexpr uint32_t kSessionTtlMs = 15 * 60 * 1000UL;   // 15 min de sessão
constexpr size_t kOperatorPasswordMinLen = 12;         // senha forte do operador
constexpr uint8_t kMaxSessions = 2;            // operador(es) simultâneo(s)

// ---- Rate limiting -------------------------------------------------------
// Janela e teto de tentativas (login e mutações) por cliente.
constexpr uint32_t kRateWindowMs = 10 * 1000UL;
constexpr uint8_t kRateMaxHitsPerWindow = 20;
constexpr uint8_t kMaxLoginFailsBeforeLockout = 5;
constexpr uint32_t kLoginLockoutMs = 60 * 1000UL;

// ---- Event bus / telemetria efêmera --------------------------------------
// Ring buffer em RAM; descarta o evento mais antigo ao encher.
constexpr size_t kEventRingCapacity = 48;
// Tamanho máximo do rótulo de cenário (string curta, allowlisted).
constexpr size_t kScenarioIdMaxLen = 32;

// ---- Scanners passivos (P1) ----------------------------------------------
constexpr uint32_t kWifiScanWindowMs = 3000;   // janela curta de varredura
constexpr uint32_t kBleScanWindowMs = 3000;
constexpr size_t kMaxScanResults = 24;          // agregado; sem persistência

// ---- Detalhamento BLE (anúncios observados, sem endereço) -----------------
// O endereço BLE (MAC) é deliberadamente OMITIDO do painel/exportações; só
// nome/RSSI/conectável. Nada é persistido (ephemeral em RAM).
constexpr size_t kBleAdvCap = 24;        // anúncios distintos mantidos
constexpr size_t kBleNameLen = 32;       // nome curto/complete (truncado)

// ---- Feeds ao vivo (console estilo ArmyKnife, tudo efêmero em RAM) --------
// IMPORTANTE: estes buffers guardam dados reais observados DURANTE uma
// demonstração AUTORIZADA, em rede isolada, exibidos ao vivo para fins
// didáticos. Exportáveis por download/Discord; apagados ao limpar/desligar.
constexpr size_t kApListCap = 24;       // APs vistos no recon
constexpr size_t kStationCap = 8;       // clientes conectados ao nosso AP
constexpr size_t kProbeRingCap = 40;    // probe requests capturados
constexpr size_t kDnsRingCap = 40;      // consultas DNS dos clientes
constexpr size_t kCaptureRingCap = 24;  // envios capturados pelo portal
// Fingerprints de aparelho capturados pelo portal (client-side, efêmero).
constexpr size_t kDeviceInfoRingCap = 16;
constexpr size_t kUaLen = 160;          // user-agent (truncado)
constexpr size_t kDevFieldLen = 32;     // campos curtos (so/navegador/tela/etc.)
constexpr size_t kTzLen = 40;           // timezone IANA
// Id de sessão gerado pelo navegador para associar o fingerprint ao login do
// mesmo aparelho (16 hex + terminador). Sem relação com MAC/IP.
constexpr size_t kSidLen = 17;
// IP que o cliente recebeu no SoftAP (DHCP), ex.: "192.168.4.2" (15 + nul).
constexpr size_t kIpLen = 16;
constexpr size_t kSsidMaxLen = 33;      // 32 + terminador
constexpr size_t kMacStrLen = 18;       // "aa:bb:cc:dd:ee:ff" + nul
constexpr size_t kCredFieldLen = 64;    // usuário/senha capturados (truncado)
// A senha capturada nunca é serializada em texto claro em exportações
// (download/relatório/Discord); este rótulo a substitui nesses caminhos.
constexpr const char* kPasswordMask = "<senha ocultada por privacidade>";
constexpr size_t kDnsNameLen = 64;
constexpr size_t kTemplateIdLen = 24;

// ---- Timeouts gerais ------------------------------------------------------
constexpr uint32_t kDemoAutoStopMs = 10 * 60 * 1000UL;  // trava de segurança
constexpr uint32_t kBootToApTargetMs = 20 * 1000UL;     // meta de "pronto"

}  // namespace cfg
}  // namespace mak
