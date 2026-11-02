#!/usr/bin/env python3
"""demo-http-login.py — app fictício de login corporativo SEM TLS (treinamento de PSI).

Serve um portal corporativo fictício via HTTP (sem SSL) e registra no console o
que foi digitado no POST, para demonstrar — com uma captura de pacotes — que
credenciais viajam em texto claro em HTTP.

USO EXCLUSIVAMENTE COM CREDENCIAIS FICTÍCIAS, em rede isolada e com
participantes autorizados (ver docs/security.md). NUNCA use senhas reais.

Uso:
    python3 scripts/demo-http-login.py [--port 8080]

O formulário fica em http://<ip-do-notebook>/ e o POST em /login.
Padrão de porta: 8080 (não exige root). Use --port 80 apenas se rodar como root.
"""
import argparse
import datetime
import socket
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs

HTML = """<!doctype html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Portal do Colaborador — AcmeCorp</title>
<style>
*{box-sizing:border-box}
body{margin:0;min-height:100vh;display:flex;align-items:center;justify-content:center;
     background:#eef2f6;color:#1f2d3a;font:16px system-ui,-apple-system,sans-serif;padding:20px}
.card{width:100%;max-width:400px;background:#fff;border-radius:12px;padding:36px;
      box-shadow:0 12px 40px rgba(15,35,60,.12)}
.brand{display:flex;align-items:center;gap:12px;margin-bottom:24px}
.logo{width:46px;height:46px;border-radius:10px;background:#0a5cd6;color:#fff;
      display:flex;align-items:center;justify-content:center;font-size:22px;font-weight:800}
h1{font-size:1.3rem;margin:0 0 6px}
.sub{color:#5c6b78;margin:0 0 24px;font-size:.95rem;line-height:1.5}
label{display:block;font-size:.85rem;font-weight:700;margin:16px 0 6px}
input{width:100%;padding:13px 14px;border:1px solid #c4cfd8;border-radius:8px;
      font-size:1rem;color:#1f2d3a}
input:focus{outline:none;border-color:#0a5cd6;box-shadow:0 0 0 3px rgba(10,92,214,.18)}
button{width:100%;margin-top:24px;padding:14px;border:none;border-radius:8px;
       background:#0a5cd6;color:#fff;font:700 1rem system-ui;cursor:pointer}
button:hover{background:#0849ad}
.note{margin-top:20px;font-size:.8rem;color:#8a99a6;text-align:center}
</style>
</head>
<body>
<div class="card">
  <div class="brand"><div class="logo">A</div><div><strong style="font-size:1.05rem">AcmeCorp</strong></div></div>
  <h1>Portal do Colaborador</h1>
  <p class="sub">Entre com suas credenciais corporativas para acessar a intranet.</p>
  <form method="post" action="/login">
    <label for="user">E-mail corporativo</label>
    <input id="user" name="user" type="email" placeholder="voce@acmecorp.com" required>
    <label for="pass">Senha</label>
    <input id="pass" name="pass" type="password" placeholder="••••••••" required>
    <button type="submit">Entrar</button>
  </form>
  <p class="note">Acesso restrito a colaboradores autorizados.</p>
</div>
</body>
</html>"""

SUCCESS = """<!doctype html>
<html lang="pt-BR">
<head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Bem-vindo — AcmeCorp</title>
<style>
body{margin:0;min-height:100vh;display:flex;align-items:center;justify-content:center;
     background:#eef2f6;font:16px system-ui,sans-serif;color:#1f2d3a;padding:20px}
.box{background:#fff;border-radius:12px;padding:40px;max-width:420px;box-shadow:0 12px 40px rgba(15,35,60,.12);text-align:center}
h1{font-size:1.3rem;margin:0 0 12px}
p{color:#5c6b78;line-height:1.6}
</style>
</head>
<body>
<div class="box">
  <h1>✅ Acesso autorizado</h1>
  <p>Bem-vindo à intranet da AcmeCorp. Esta é uma simulação — nenhuma autenticação real ocorreu.</p>
</div>
</body>
</html>"""


class Handler(BaseHTTPRequestHandler):
    server_version = "AcmeCorp/1.0"

    def log_message(self, *_):
        pass  # silencia o log de acesso padrão

    def _html(self, code, body):
        data = body.encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        if self.path in ("/", "/login"):
            self._html(200, HTML)
        else:
            self._html(404, "<h1>404</h1>")

    def do_POST(self):
        if self.path != "/login":
            self._html(404, "<h1>404</h1>")
            return
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length).decode("utf-8", "replace")
        fields = parse_qs(body)
        user = fields.get("user", [""])[0]
        pwd = fields.get("pass", [""])[0]
        ts = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        print(f"[{ts}] user={user!r} pass={pwd!r}", flush=True)
        self._html(200, SUCCESS)


def lan_ip():
    # Descobre o IP do notebook na interface que alcança o gateway do AP
    # isolado (192.168.4.1). Sem dependências externas.
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("192.168.4.1", 80))
        return s.getsockname()[0]
    finally:
        s.close()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=8080)
    args = ap.parse_args()

    ip = lan_ip()
    httpd = ThreadingHTTPServer(("0.0.0.0", args.port), Handler)
    print(f"Servidor em http://{ip}:{args.port}/  (gateway do AP: 192.168.4.1)")
    print("AVISO: use apenas credenciais FICTÍCIAS (treinamento de PSI).")
    print("Ctrl+C para encerrar.")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nEncerrado.")


if __name__ == "__main__":
    main()
