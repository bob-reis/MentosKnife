# Direção do produto: ferramentas reais e operações visíveis

O MentosArmyKnife deve ser uma caixa de ferramentas operada pelo instrutor.
O treinamento usa os resultados reais das operações; o firmware não deve
impor uma sequência de roteiro, revelação ou cliques simulados.

Requisito do usuário: operação transparente para projeção em treinamento.
Cada ferramenta deve mostrar parâmetros, estado, resultados, erros e quando
os dados foram observados. Ausência de resultados e falha de execução devem
ser estados diferentes. Não apresentar dados fabricados como observações.

## Base encontrada no código

Esta inspeção confirma implementação, não validação em hardware:

| Recurso | Situação encontrada |
|---|---|
| Wi-Fi | ReconMonitor lista SSID, BSSID, canal, RSSI e autenticação. |
| Clientes do AP | ReconMonitor lista estações conectadas ao próprio AP. |
| Probes | Há monitor passivo e tabela de resultados. |
| DNS | Há registro de consultas recebidas pelo DNS local. |
| BLE | Scanner retorna agregados; ainda falta detalhamento no painel. |
| Portal | Há formulário e registro em RAM dos valores submetidos; o README descreve apenas interação booleana e precisa ser corrigido. |
| Interface | Abas de ferramentas coexistem com cenários e revelação obrigatórios para algumas operações. |
| STOP | Interrompe o sniffer e limpa dados; precisa de revisão do ciclo de vida de cada módulo. |

## Arquitetura pretendida

- Catálogo de ferramentas com capacidades e disponibilidade explícitas.
- Ações por ferramenta: configurar, iniciar, acompanhar e parar.
- Estado por operação: ociosa, executando, concluída, interrompida ou erro.
- Coordenação dos recursos compartilhados, especialmente o rádio Wi-Fi/BLE.
- Histórico visível de ações e resultados, independente do roteiro da aula.
- Modo de projeção com texto legível, atualização ao vivo e status da conexão.
- Parar operações e limpar resultados como ações distintas, permitindo
  discutir os resultados após interromper uma operação.

A transparência das operações não exige publicar senhas administrativas,
tokens de sessão ou webhooks. Acesso ao console continua autenticado.

## Limites e precisão da demonstração

- Probes mostram solicitações observadas; não comprovam o histórico de
  conexões de um aparelho nem a identidade de uma pessoa.
- O painel DNS mostra consultas recebidas pelo dispositivo; não representa
  todo o tráfego ou consultas criptografadas dos clientes.
- Resultados devem informar o canal/janela de observação quando aplicável.
- O USB Serial/JTAG nativo do ESP32-C3 tem função fixa. Recursos USB HID/MSC
  exigem avaliar outro controlador ou hardware adicional.

Referência técnica: [USB Serial/JTAG do ESP32-C3 — Espressif](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/api-guides/usb-serial-jtag-console.html).

## Escopo definido e primeira integração

Trazer o que é compatível com o C3, sem SD. O primeiro port é o escritor PCAP
em RAM do USBArmyKnife. Recon Wi-Fi/probes e DNS existentes continuam locais.
Foram adicionados conexão Wi-Fi pelo dashboard, consulta DNS, recon TCP de um
host, download JSON/PCAP, relatório Discord e modo de projeção.

Os relatórios incluem os valores do portal em texto claro para demonstrações
com contas de laboratório. Discord depende de uplink, NTP e TLS; download
local funciona sem Internet. Parar preserva resultados, Limpar remove dados
locais e envios pendentes. Veja `network-tools.md` para limites e validação.

Ainda não há port integral do Marauder, tshark embarcado, USB HID/MSC/NCM,
varredura de toda a sub-rede, identificação de versões ou detalhamento BLE.
