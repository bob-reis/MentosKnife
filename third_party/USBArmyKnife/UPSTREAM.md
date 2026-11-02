# USBArmyKnife — importação seletiva

Origem: https://github.com/i-am-shodan/USBArmyKnife
Commit: `fcea8c5d3f0ce48652215183272b19f3e1cd8473`.
Licença: MIT, copyright 2024 i-am-shodan; texto integral em `LICENSE`.

`original/PcapBuffer.{h,cpp}` são cópias integrais dos arquivos
`src/Utilities/PcapBuffer.{h,cpp}` no commit acima. Não entram na compilação.
`manifest.json` registra hashes SHA-256 dos originais e da licença.

Adaptação compilada: `src/storage/PcapBuffer.{h,cpp}` na raiz do projeto.
Mantém a escrita little-endian dos cabeçalhos PCAP e registros. Substitui
filesystem/fila dinâmica por 8 KB fixos em RAM, limita snaplen a 512 bytes,
registra descarte, usa timestamp de uptime de 64 bits e link type 105 (802.11).
Não usa SD, SPIFFS, USB, tarefas de outro núcleo ou heap no callback Wi-Fi.

O restante do USBArmyKnife não foi incorporado ao firmware. Wi-Fi, dashboard,
DNS e TCP usam implementações locais; não são apresentados como ports do
Marauder. USB HID/MSC/NCM e agente USB não são compatíveis com o controlador
USB nativo do C3. Importar o Marauder inteiro exige trabalho específico com
SDK, NimBLE, armazenamento e uso do rádio; não é uma dependência desta versão.
