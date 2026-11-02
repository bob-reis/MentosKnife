# Roteiro de treinamento (fala do instrutor)

> Objetivo: mostrar, de forma ética e memorável, como iscas de phishing /
> portal cativo funcionam em **três contextos** — e como se defender. Tudo em
> ambiente isolado, com aparelho de teste e dados sintéticos.

Os três cenários (allowlisted) são selecionáveis no painel ou pelo console
(`scenario <id>`):

| ID | Contexto | Quando usar |
|---|---|---|
| `corp-ambiente` | **Ambiente corporativo** | funcionários, acesso/VPN, e-mail de "TI", QR no escritório |
| `fora-empresa` | **Profissional fora da empresa** | hotéis, eventos, cafés — Wi-Fi aberto / evil twin |
| `pessoal-familia` | **Vida pessoal e família** | golpes de prêmio/entrega/Pix, redes sociais, crianças e idosos |

O portal e a revelação se adaptam automaticamente ao cenário escolhido; o painel
e o feed do Discord mostram qual cenário está em cena.

## Abertura (contexto geral)

"Esta caixinha de Mentos é, na verdade, um pequeno computador com Wi-Fi. Vou
mostrar como um atacante poderia criar redes e páginas convincentes em três
situações do nosso dia a dia. Nada aqui coleta senhas reais — é uma simulação
voluntária, com um aparelho de teste."

## Cenário 1 — Ambiente corporativo (`corp-ambiente`)

1. "Imaginem um e-mail ou aviso de **TI** pedindo para confirmar o acesso."
2. Participante abre o portal → moldura "Portal de acesso corporativo" → toca no
   botão. Painel/Discord: **Interação detectada — Ambiente corporativo**.
3. Revelação + discussão: validar remetente/domínio, **SSO + MFA resistente a
   phishing**, não reutilizar a senha corporativa, cuidado com QR em crachás e
   impressoras, **reportar ao SOC**.

## Cenário 2 — Fora da empresa (`fora-empresa`)

1. "Agora vocês estão em um **hotel/evento/café** e aparece um Wi-Fi grátis."
2. Portal → "Wi-Fi gratuito — faça login" → toque. Painel/Discord marcam o
   contexto.
3. Revelação + discussão: desconfiar de Wi-Fi aberto (evil twin), confirmar o
   nome na recepção, preferir **4G/5G**, usar **VPN própria**, checar HTTPS,
   desativar conexão automática.

## Cenário 3 — Pessoal e família (`pessoal-familia`)

1. "Em casa: uma mensagem de **prêmio, entrega ou Pix** com urgência."
2. Portal → "Você tem uma recompensa pendente!" → toque.
3. Revelação + discussão: desconfiar de urgência/prêmios, conferir o link antes
   de tocar, **MFA** em contas pessoais, conversar com **crianças e idosos**,
   **senhas únicas + gerenciador**.

## Encerramento

"Vou apertar **STOP**: os registros temporários são apagados e desligo o
aparelho. Nenhum dado foi guardado." (executar STOP e desligar)

## Observações

- Rode os três em sequência ou escolha conforme o público. **Entre um cenário e
  o próximo:** toque em **Encerrar revelação** (volta ao portal pronto) e então
  inicie o próximo cenário. O **STOP** é o encerramento final da sessão (limpa a
  telemetria e bloqueia novos envios até reiniciar).
- O pop-up do captive portal **varia** entre Android/iOS — tenha a URL/QR de
  contingência (`http://192.168.4.1/portal`).
- Mantenha o tom educativo; o objetivo é capacitar, não constranger.
