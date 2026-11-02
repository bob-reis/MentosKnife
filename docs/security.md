# Operação, dados e autorização

O dispositivo executa ferramentas reais em redes e equipamentos incluídos no
escopo do treinamento. O painel foi projetado para tornar as operações e seus
resultados visíveis. Não há execução oculta ou agente instalado em computadores.

## Dados

- Recon Wi-Fi, probes, DNS, TCP, mDNS e envios ao portal ficam em buffers RAM.
- Os valores enviados ao portal são exibidos em texto claro e incluídos no
  JSON. O envio manual ao Discord inclui o JSON e o último envio do portal no
  resumo. Use contas de laboratório para essa demonstração.
- Parar preserva os dados para análise. Limpar remove resultados locais e
  cancela envios pendentes; desligar perde a RAM. Arquivos baixados e mensagens
  entregues ao Discord permanecem fora do controle do dispositivo.
- A configuração de Wi-Fi e Discord é persistida na NVS. A senha do operador
  usa hash com salt. O SoftAP de visitantes é aberto (sem senha de rádio): o
  isolamento do AP (sem bridge/NAT) é o guardrail, não uma senha de Wi-Fi. A
  senha de uplink e o webhook precisam permanecer disponíveis para o firmware;
  não aparecem nos relatórios.

## Painel e API

Não há senha administrativa padrão. Provisionamento é feito por serial.
A API exige sessão de operador; mutações também exigem o token CSRF.
GET `/api/v1/session` restaura o CSRF da sessão válida no cookie HttpOnly;
a senha não é armazenada no navegador. Recarregar não renova o prazo de
15 minutos. Expiração, logout e reboot exigem nova autenticação. O painel
local usa HTTP, cookie HttpOnly/SameSite e limitação de requisições. HTTP não
fornece confidencialidade para a comunicação local.

O AP é mantido durante a conexão STA ao uplink. O firmware não implementa
NAT/bridge. Conectar o dispositivo a uma rede autoriza apenas as operações
previstas pelo instrutor. TCP recebe um IP de host na sub-rede conectada;
não tenta logins, explora falhas nem percorre automaticamente todos os hosts.
mDNS procura um tipo de serviço por vez e limita resultados.

## Discord

O webhook é provisionado por serial e habilitado explicitamente. A ação
Enviar relatório do dashboard tira um snapshot dos resultados atuais.
Com o feed habilitado, resultados TCP/DNS e contagens mDNS também geram
mensagens ao vivo. O JSON pode conter nomes de redes, endereços e valores do portal; eles não
são mascarados. O painel deixa essa inclusão explícita antes do envio.

HTTPS valida a cadeia de certificados usando o bundle do ESP-IDF. NTP precisa
sincronizar o horário; falhas de relógio, TLS, rede ou HTTP mantêm a mensagem
pendente. Não existe fallback TLS inseguro. Redirecionamentos HTTP são
recusados. Menções automáticas do Discord são desabilitadas no payload.

Há no máximo um relatório pendente (até 24 KB de JSON) e dez eventos curtos.
A fila de eventos descarta o mais antigo ao encher e expõe o contador.
Falhas têm intervalo de nova tentativa de pelo menos 15 segundos. O limite
`Retry-After` enviado pelo serviço também é respeitado. Status de entrega
não equivale a confirmação de leitura do anexo pelo instrutor.

## Limites técnicos

PCAP registra probes no canal observado, sem descriptografar tráfego. mDNS
não é inventário completo; anúncios podem ser inexatos. TCP aberto é apenas
conexão aceita. Não interpretar esses resultados como identidade comprovada,
histórico de navegação ou vulnerabilidade confirmada.
