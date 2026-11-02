// portal-fp.js — coletor de fingerprint do portal de treinamento.
//
// AVISO DIDÁTICO: este script roda no navegador do participante e coleta, SEM
// pedir qualquer permissão, os dados que o browser expõe a qualquer página.
// É exatamente a técnica que queremos demonstrar: basta abrir a página (após
// conectar no Wi-Fi aberto / ler o QR) para o operador ver o fingerprint.
// Tudo é efêmero em RAM no dispositivo (apagado em Limpar/Parar/Desligar) e
// NÃO inclui senha. O IP público/geolocalização NÃO funcionam aqui (o AP é
// isolado, sem internet) — essa parte fica no site hospedado, via QR code.
(() => {
  'use strict';

  const ua = navigator.userAgent || '';

  // Id de sessão por aba: gerado uma vez e reusado no login (form) para o
  // dispositivo associar o fingerprint às credenciais do MESMO aparelho.
  function sessionId() {
    try {
      let s = sessionStorage.getItem('mak_sid');
      if (!s) {
        const a = new Uint8Array(8);
        (crypto || window.crypto).getRandomValues(a);
        s = Array.from(a, (b) => b.toString(16).padStart(2, '0')).join('');
        sessionStorage.setItem('mak_sid', s);
      }
      return s;
    } catch (_) {
      return String(Date.now()).slice(-12);
    }
  }
  const sid = sessionId();

  function detectOS() {
    if (/Windows NT 10/.test(ua)) return 'Windows 10/11';
    if (/Windows NT 6\.3/.test(ua)) return 'Windows 8.1';
    if (/Windows NT 6\.1/.test(ua)) return 'Windows 7';
    if (/Windows/.test(ua)) return 'Windows';
    if (/iPhone/.test(ua)) return 'iOS (iPhone)';
    if (/iPad/.test(ua)) return 'iPadOS';
    if (/Android\s([\d.]+)/.test(ua)) return 'Android ' + RegExp.$1;
    if (/Android/.test(ua)) return 'Android';
    if (/Mac OS X/.test(ua)) return 'macOS';
    if (/CrOS/.test(ua)) return 'Chrome OS';
    if (/Linux/.test(ua)) return 'Linux';
    return 'Não detectado';
  }

  function detectBrowser() {
    if (/Edg\/([\d.]+)/.test(ua)) return 'Edge ' + RegExp.$1;
    if (/(OPR|Opera)\/([\d.]+)/.test(ua)) return 'Opera ' + RegExp.$2;
    if (/SamsungBrowser\/([\d.]+)/.test(ua)) return 'Samsung ' + RegExp.$1;
    if (/Firefox\/([\d.]+)/.test(ua)) return 'Firefox ' + RegExp.$1;
    if (/Chrome\/([\d.]+)/.test(ua)) return 'Chrome ' + RegExp.$1;
    if (/Version\/([\d.]+).*Safari/.test(ua)) return 'Safari ' + RegExp.$1;
    return 'Não detectado';
  }

  function detectDevice() {
    if (/iPhone/.test(ua)) return 'iPhone';
    if (/iPad/.test(ua)) return 'iPad';
    if ((m = ua.match(/SM-[A-Z0-9]+/))) return 'Samsung ' + m[0];
    if (/Pixel[^;)]*/.test(ua)) return RegExp.lastMatch.trim();
    if (/(Redmi|POCO|Xiaomi|Mi)\s?[^;)]*/.test(ua)) return RegExp.lastMatch.trim();
    if (/(moto|Motorola)[^;)]*/.test(ua)) return RegExp.lastMatch.trim();
    if (/(HUAWEI|Honor|OnePlus)[^;)]*/.test(ua)) return RegExp.lastMatch.trim();
    if (/Mobile|Android/.test(ua)) return 'Smartphone';
    if (/Tablet|iPad/.test(ua)) return 'Tablet';
    return 'Desktop/Notebook';
  }

  let m;
  const conn = navigator.connection || navigator.mozConnection || navigator.webkitConnection;

  async function collect() {
    let battery = '—';
    try {
      if ('getBattery' in navigator) {
        const b = await navigator.getBattery();
        battery = Math.round(b.level * 100) + '%' + (b.charging ? ' (carregando)' : '');
      }
    } catch (_) { /* sem bateria exposta */ }

    return {
      os: detectOS(),
      browser: detectBrowser(),
      device: detectDevice(),
      platform: navigator.platform || '—',
      lang: navigator.language || '—',
      screen: `${screen.width}x${screen.height}@${window.devicePixelRatio || 1}x`,
      touch: ('ontouchstart' in window || navigator.maxTouchPoints > 0) ? '1' : '0',
      cores: navigator.hardwareConcurrency ? String(navigator.hardwareConcurrency) : '—',
      mem: navigator.deviceMemory ? `~${navigator.deviceMemory}GB` : '—',
      conn: conn && conn.effectiveType ? conn.effectiveType.toUpperCase() : '—',
      battery: battery,
      tz: (() => { try { return Intl.DateTimeFormat().resolvedOptions().timeZone; } catch (_) { return '—'; } })(),
      ua: ua.slice(0, 180),
      sid: sid
    };
  }

  async function send() {
    try {
      const d = await collect();
      const body = Object.keys(d)
        .map((k) => k + '=' + encodeURIComponent(d[k]))
        .join('&');
      // keepalive: entrega mesmo se a página for fechada logo após abrir.
      await fetch('/portal/telemetry', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: body,
        keepalive: true
      });
    } catch (_) { /* silencioso: nunca interrompe a experiência do participante */ }
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', send);
  } else {
    send();
  }
})();
