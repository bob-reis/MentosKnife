# Ferramentas e contextos de demonstração

| Contexto | Operação real | O que discutir |
|---|---|---|
| Empresa | Conectar à rede de laboratório, consultar DNS e verificar portas de um host | Exposição de serviços e segmentação; conexão aceita não implica falha explorável. |
| Hotel/café | Recon Wi-Fi, canal e autenticação; monitor de probes; portal de laboratório | SSID não autentica o estabelecimento; formulários podem receber os valores digitados. |
| Pessoal/família | Observação Wi-Fi e envios no portal com contas de teste | Divulgação de dados, reutilização de senhas e verificação de endereços. |

As ferramentas operam independentemente. O dashboard não apresenta seleção de cenários ou revelação. O portal legado
e suas ações por serial/API continuam disponíveis; a área Envios exibe
os dados já recebidos.

## API adicionada

Todas as rotas exigem sessão de operador; POST também exige CSRF.

| Método | Rota | Função |
|---|---|---|
| GET | `/api/v1/network` | Conexão, último TCP e última consulta DNS |
| POST | `/api/v1/network/connect` | Campos `ssid`, `password`; salva configuração na NVS |
| POST | `/api/v1/network/disconnect` | Interrompe recon e desliga uplink, sem reconexão automática nesta sessão |
| POST | `/api/v1/network/scan` | Campo `target`: IPv4 de host na sub-rede STA |
| POST | `/api/v1/network/stop` | Interrompe TCP, mantém resultados |
| POST | `/api/v1/network/discover` | Campo `service`: http, https, ipp, ssh, smb ou workstation; até 12 anúncios IPv4 do uplink |
| POST | `/api/v1/network/dns` | Campo `host`: nome a resolver |
| GET | `/api/v1/reports/download` | JSON com os dados atualmente retidos |
| GET | `/api/v1/reports/pcap` | Snapshot PCAP do buffer de probes |
| POST | `/api/v1/reports/discord` | Snapshot JSON na fila para envio como anexo |
| POST | `/api/v1/reports/clear` | Limpa dados locais e fila; não apaga Discord/downloads |

O relatório mantém valores do portal em texto claro. Os buffers limitam
quantidade e comprimento dos campos (usuário e senha: até 63 bytes cada).
A senha do Wi-Fi usada para conectar, a senha administrativa e o webhook
não integram os resultados. A senha do uplink é configuração persistida na NVS.

## Teste de bancada pendente

1. Conectar ao AP, autenticar no painel, buscar SSIDs e conectar a uma rede de teste.
2. Confirmar IP/gateway/DNS; testar senha errada e desconexão manual.
3. Verificar um host com uma das oito portas abertas e outro com portas filtradas.
4. Interromper TCP e confirmar preservação dos resultados; desconectar durante scan.
5. Resolver um nome existente e um inexistente; descobrir uma impressora IPP ou servidor HTTP que anuncie mDNS.
6. Ligar o monitor de probes, baixar PCAP e abrir no tshark/Wireshark.
7. Enviar valores de laboratório ao portal; conferir texto no painel e no JSON.
8. Enviar relatório ao Discord; conferir anexo, HTTP e estado `sent`.
9. Repetir sem Internet: relatório pendente deve continuar baixável; testar Limpar.
10. Confirmar que STOP preserva e Limpar apaga resultados; testar reinício do portal.

Os testes de host não enviam mensagens a serviços externos. O firmware pode
enviar eventos de boot automaticamente se o Discord já estiver habilitado.
