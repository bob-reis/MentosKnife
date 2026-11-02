# Demonstração: login HTTP sem SSL (treinamento de PSI)

Mostrar, em rede isolada, que credenciais enviadas por HTTP (sem TLS) viajam em
texto claro e podem ser lidas por quem captura o tráfego.

**Princípio:** somente credenciais **fictícias**, rede isolada (`Wifi-Café`),
participantes autorizados, revelação imediata. Nunca use senhas reais.

## Pré-requisitos

- Notebook **conectado ao AP `WiFi Visitantes`** (a rede isolada do dispositivo).
- `tshark` instalado (`sudo apt install tshark`).
- Aparelho de teste (participante) também conectado ao `WiFi Visitantes`.

## Passo a passo

1. No notebook, suba o app fictício:

   ```bash
   python3 scripts/demo-http-login.py            # porta 8080 (sem root)
   # ou, para o visual "http://192.168.4.x/" raiz:
   sudo python3 scripts/demo-http-login.py --port 80
   ```

   O script imprime a URL (ex.: `http://192.168.4.2:8080/`). Anote o IP.

2. Identifique a interface Wi-Fi do notebook:

   ```bash
   ip link     # ex.: wlan0 / wlp2s0
   ```

3. Inicie a captura de pacotes (em outro terminal):

   ```bash
   sudo tshark -i wlan0 -f 'tcp port 8080' -Y 'http.request' \
     -T fields -e frame.time -e ip.src -e http.request.uri -e http.file_data
   ```

   (`http.file_data` mostra o corpo do POST, incluindo `user=...&pass=...` em claro.)

4. No aparelho de teste, abra `http://<ip-do-notebook>:8080/`, preencha com
   credenciais **fictícias** e clique em **Entrar**.

5. No terminal do tshark, o POST aparece com a senha em texto claro — e o
   script também imprime `user=... pass=...` ao receber.

## Variações de captura

- Mostrar cada campo separadamente (dissector de formulário):

  ```bash
  sudo tshark -i wlan0 -f 'tcp port 8080' -Y 'urlencoded-form' \
    -T fields -e urlencoded-form.key -e urlencoded-form.value
  ```

- Gravar para analisar depois:

  ```bash
  sudo tshark -i wlan0 -f 'tcp port 8080' -w http-demo.pcap
  tshark -r http-demo.pcap -Y 'http.request.method == "POST"' -T fields \
    -e frame.time -e ip.src -e http.file_data
  ```

## Revelação (o que apontar)

- O login trafegou em **texto claro** (sem HTTPS/cadeado): `pass=<o que foi digitado>`.
- Quem está na mesma rede consegue ler esse campo com um sniffer.
- O **contraste** com o portal `/portal` do próprio dispositivo: ali a senha é
  mascarada no envio; aqui o HTTP cru revela tudo — ilustra por que HTTPS e
  validação de rede/certificado importam.
- Reforce: nunca digite a senha real de uma conta em página sem HTTPS, em rede
  desconhecida ou em portal de acesso à internet que não foi verificado.

## Limpeza

- Pare a captura (`Ctrl+C`) e o app (`Ctrl+C`).
- O app **não grava** as credenciais em disco — só imprime no console.
- Remova o arquivo `.pcap` se gravou (`rm -f http-demo.pcap`).
