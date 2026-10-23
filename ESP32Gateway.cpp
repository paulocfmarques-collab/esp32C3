#include "ESP32Gateway.h"
#include "DisplayUtil.h"
#include "RGBLed.h"
#include "ClimaManager.h"

extern DisplayUtil display;
extern RGBLed rgbLed;

namespace
{
String escapeHtml(String value);
const char* webStyles = R"css(
:root{color-scheme:dark}*{box-sizing:border-box}body{font:16px/1.6 system-ui,sans-serif;background:radial-gradient(ellipse at top left,#16324b,#0b1220 65%);color:#e5eefb;margin:0;padding:24px;min-height:100vh}
main{max-width:1000px;margin:auto}h1{font-size:clamp(1.8rem,5vw,2.7rem);line-height:1.2;margin:20px 0 12px}h1,h2{color:#a5f3fc}h2{font-size:1.15rem;margin-top:0}section{background:#152238;border:1px solid #2c425d;border-radius:18px;padding:24px;margin:18px 0;box-shadow:0 12px 30px #0002}
.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:18px}.grid section{margin:0}dl{display:grid;grid-template-columns:minmax(110px,1fr) 1.6fr;gap:10px;margin:0}dt,small,.muted{color:#a9bcd1}dd{margin:0;overflow-wrap:anywhere}a{color:#67e8f9}nav{display:flex;gap:12px;flex-wrap:wrap;margin:22px 0}nav a,.badge{border:1px solid #36536f;border-radius:999px;padding:6px 14px;text-decoration:none}.badge{display:inline-block;color:#a5f3fc;background:#11343e}label{display:block;margin:16px 0 6px}input{width:100%;padding:12px;background:#0b1627;color:#e5eefb;border:1px solid #48627e;border-radius:9px;font:inherit}button{margin-top:18px;padding:12px 20px;background:#67e8f9;color:#082532;border:0;border-radius:9px;font:inherit;font-weight:700;cursor:pointer}a:focus-visible,input:focus-visible,button:focus-visible{outline:3px solid #67e8f9;outline-offset:4px}li{padding:10px 0;border-bottom:1px solid #2c425d}code{color:#a5f3fc;overflow-wrap:anywhere}footer{margin:24px 0;color:#a9bcd1;font-size:.85rem}progress{width:100%;height:10px;accent-color:#67e8f9}@media(max-width:640px){body{padding:16px}.grid{grid-template-columns:1fr}section{padding:18px}dl{grid-template-columns:1fr 1.3fr}}
)css";

void addInfoRow(String& page, const char* label, const String& value)
{
    page += "<dt>" + String(label) + "</dt><dd>" + escapeHtml(value) + "</dd>";
}

String escapeHtml(String value)
{
    value.replace("&", "&amp;");
    value.replace("<", "&lt;");
    value.replace(">", "&gt;");
    value.replace("\"", "&quot;");
    value.replace("'", "&#39;");
    return value;
}

String buildInfoPage()
{
    bool wifiConnected = WiFi.status() == WL_CONNECTED;
    unsigned long uptimeSeconds = millis() / 1000UL;

    String page = R"rawliteral(
<!doctype html><html lang="pt-BR"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta http-equiv="refresh" content="10"><title>ESP32-C3 | Painel do dispositivo</title>
<style>
body{font:16px system-ui,sans-serif;background:#0b1220;color:#e5eefb;margin:0;padding:24px}
main{max-width:760px;margin:auto}h1{color:#67e8f9}h2{color:#a5f3fc;font-size:1.15rem}
section{background:#152238;border:1px solid #263953;border-radius:12px;padding:16px;margin:16px 0}
dl{display:grid;grid-template-columns: minmax(120px,1fr) 2fr;gap:10px;margin:0}
dt{color:#9fb3ca}dd{margin:0;overflow-wrap:anywhere}
small{color:#9fb3ca}
</style></head><body><main><span class="badge">Painel do dispositivo</span><h1>ESP32-C3 Gateway</h1>
<p class="muted">Acompanhe a conexão, os recursos do sistema e as condições do tempo.</p>
<nav aria-label="Navegação"><a href="/info" aria-current="page">Visão geral</a><a href="/wifi">Configurar Wi-Fi</a><a href="/prometheus">Prometheus</a><a href="/info">Atualizar agora</a></nav>
<small>Informacoes atualizadas automaticamente a cada 10 segundos</small>
<div class="grid"><section><h2>Sistema</h2><dl>
<dt>Modelo</dt><dd>)rawliteral";
    page += escapeHtml(String(ESP.getChipModel()));
    page += R"rawliteral(</dd><dt>Uptime</dt><dd>)rawliteral";
    page += String(uptimeSeconds / 86400UL) + " d " + String((uptimeSeconds / 3600UL) % 24UL) +
            " h " + String((uptimeSeconds / 60UL) % 60UL) + " min";
    page += "</dd>";
    addInfoRow(page, "CPU",
               String(ESP.getCpuFreqMHz()) + " MHz / " + String(ESP.getChipCores()) + " núcleo(s)");
    addInfoRow(page, "Revisão do chip", String(ESP.getChipRevision()));
    addInfoRow(page, "RAM livre",
               String(ESP.getFreeHeap() / 1024.0, 1) + " KB de " +
                   String(ESP.getHeapSize() / 1024.0, 1) + " KB");
    addInfoRow(page, "Menor RAM livre",
               String(ESP.getMinFreeHeap() / 1024.0, 1) + " KB desde a inicialização");
    addInfoRow(page, "Flash total", String(ESP.getFlashChipSize() / (1024.0 * 1024.0), 1) + " MB");
    addInfoRow(page, "Firmware", String(ESP.getSketchSize() / 1024.0, 1) + " KB");
    addInfoRow(page, "Espaço para OTA", String(ESP.getFreeSketchSpace() / 1024.0, 1) + " KB");
    addInfoRow(page, "Compilação", String(__DATE__) + " " + String(__TIME__));
    addInfoRow(page, "SDK", String(ESP.getSdkVersion()));
    page += R"rawliteral(</dl></section>
<section><h2>Rede</h2><dl><dt>Estado</dt><dd>)rawliteral";
    page += wifiConnected ? "Conectado" : "Desconectado";
    page += R"rawliteral(</dd><dt>SSID</dt><dd>)rawliteral";
    page += wifiConnected ? escapeHtml(WiFi.SSID()) : "--";
    page += R"rawliteral(</dd><dt>IP</dt><dd>)rawliteral";
    page += wifiConnected ? WiFi.localIP().toString() : "--";
    page += R"rawliteral(</dd><dt>Gateway</dt><dd>)rawliteral";
    page += wifiConnected ? WiFi.gatewayIP().toString() : "--";
    page += R"rawliteral(</dd><dt>Subnet</dt><dd>)rawliteral";
    page += wifiConnected ? WiFi.subnetMask().toString() : "--";
    page += R"rawliteral(</dd><dt>DNS</dt><dd>)rawliteral";
    page += wifiConnected ? WiFi.dnsIP(0).toString() : "--";
    page += R"rawliteral(</dd><dt>MAC</dt><dd>)rawliteral";
    page += WiFi.macAddress();
    page += R"rawliteral(</dd><dt>Canal</dt><dd>)rawliteral";
    page += wifiConnected ? String(WiFi.channel()) : "--";
    page += R"rawliteral(</dd><dt>RSSI</dt><dd>)rawliteral";
    page += wifiConnected ? String(WiFi.RSSI()) + " dBm" : "--";
    page += "</dd>";
    const int rssi = wifiConnected ? WiFi.RSSI() : -100;
    addInfoRow(page, "Qualidade do sinal",
               !wifiConnected ? "Sem conexão"
               : rssi >= -55  ? "Excelente"
               : rssi >= -67  ? "Boa"
               : rssi >= -75  ? "Regular"
                              : "Fraca; aproxime o dispositivo do roteador");
    addInfoRow(page, "Modo",
               WiFi.getMode() == WIFI_AP ? "Ponto de acesso de configuração" : "Cliente Wi-Fi");
    if (WiFi.getMode() == WIFI_AP)
    {
        addInfoRow(page, "Rede do portal", "ESP32_C3_CONFIG");
        addInfoRow(page, "IP do portal", WiFi.softAPIP().toString());
        addInfoRow(page, "Clientes no portal", String(WiFi.softAPgetStationNum()));
    }
    page += "</dl></section><section><h2>Clima · Porto Alegre</h2>";
    if (ClimaManager::sincronizado)
    {
        page += "<dl>";
        addInfoRow(page, "Temperatura", String(ClimaManager::temperatura, 1) + " °C");
        addInfoRow(page, "Condição", ClimaManager::obterTextoCondicao());
        addInfoRow(page, "Idade da leitura",
                   String((millis() - ClimaManager::ultimaAtualizacao) / 60000UL) + " min");
        if (ClimaManager::previsaoValida)
        {
            addInfoRow(page, "Data da previsão", ClimaManager::dataPrevisao);
            addInfoRow(page, "Mínima / máxima",
                       String(ClimaManager::minima, 1) + " / " + String(ClimaManager::maxima, 1) +
                           " °C");
            addInfoRow(page, "Probabilidade de chuva", String(ClimaManager::chuva) + "%");
        }
        page += "</dl><p class=\"muted\">Fonte: Open-Meteo. Confira a idade da leitura para "
                "avaliar a atualização.</p>";
    }
    else
    {
        page += "<p>Aguardando a primeira atualização do clima. É necessária uma conexão com a "
                "internet.</p>";
    }
    if (ClimaManager::ultimoErro.length())
        page += "<p>Última consulta: " + escapeHtml(ClimaManager::ultimoErro) + "</p>";
    page +=
        R"rawliteral(</section><section><h2>Como usar o gateway</h2><p>Cadastre até cinco redes em <a href="/wifi">Configurar Wi-Fi</a>. Ao iniciar, o dispositivo procura as redes salvas. Se nenhuma conectar, abre o portal <strong>ESP32_C3_CONFIG</strong>.</p><p>Comandos remotos usam <strong>UDP na porta 4210</strong>. Envie <code>help</code> para consultar os comandos, <code>health</code> para diagnóstico e <code>ntp_status</code> para verificar o relógio.</p><p>As atualizações de firmware usam ArduinoOTA pela rede Wi-Fi. Após 30 minutos sem conexão, o dispositivo reinicia automaticamente para tentar recuperar a rede.</p></section></div><footer>Atualização automática a cada 10 segundos · Dados obtidos do dispositivo</footer></main></body></html>)rawliteral";
    page.replace("</style>", String(webStyles) + "</style>");
    return page;
}
} // namespace

ESP32Gateway::ESP32Gateway() : server(80)
{
}

void ESP32Gateway::begin()
{
    metrics.begin();
    setupMetricsRoutes();
    loadWifiCredentials();

    // Tenta conectar ao WiFi
    if (!connectWifi())
    {
        initPortal();
    }
    else
    {
        server.on("/", HTTP_GET,
                  [this]()
                  {
                      server.sendHeader("Location", "/info");
                      server.send(302, "text/plain", "");
                  });
        setupInfoRoutes();
        server.begin();
    }

    metrics.setUdpListenerUp(udp.begin(udpPort) == 1);
    startMetricsServer();
    networkSeenAt_ = millis();
}

void ESP32Gateway::handleClient()
{
    checkNetworkWatchdog();
    metrics.update();
    if (metricsPortPending && millis() - metricsPortSavedAt >= 1000UL)
        startMetricsServer();
    if (WiFi.getMode() == WIFI_AP || WiFi.status() == WL_CONNECTED)
    {
        server.handleClient();
        if (activeMetricsPort != 80) metricsServer.handleClient();
    }
}

void ESP32Gateway::setupInfoRoutes()
{
    server.on("/info", HTTP_GET,
              [this]() { server.send(200, "text/html; charset=utf-8", buildInfoPage()); });
    setupWifiConfigRoutes();
}

void ESP32Gateway::setupWifiConfigRoutes()
{
    server.on("/wifi", HTTP_GET,
              [this]() { server.send(200, "text/html; charset=utf-8", buildConfigPage()); });
    server.on("/salvar", HTTP_POST, [this]() { saveWifi(); });
}

void ESP32Gateway::setupMetricsRoutes()
{
    auto recordHttp = [this](WebServer& web, Middleware::Callback next)
    {
        const int64_t start = esp_timer_get_time();
        const bool handled = next();
        metrics.recordHttp(esp_timer_get_time() - start, web.responseCode());
        return handled;
    };
    server.addMiddleware(recordHttp);
    metricsServer.addMiddleware(recordHttp);
    metricsServer.on("/metrics", HTTP_GET, [this]() { sendMetrics(metricsServer); });
    server.on("/metrics", HTTP_GET, [this]()
    {
        if (activeMetricsPort == 80) sendMetrics(server);
        else server.send(404, "text/plain; charset=utf-8", "Use a porta de metricas configurada.");
    });
    server.on("/prometheus", HTTP_GET, [this]() { showPrometheus(); });
    server.on("/prometheus", HTTP_POST, [this]() { savePrometheus(); });
    server.on("/prometheus.yml", HTTP_GET, [this]()
    {
        server.sendHeader("Cache-Control", "no-store");
        server.sendHeader("Content-Disposition", "attachment; filename=prometheus.yml");
        server.send(200, "text/yaml; charset=utf-8", metrics.scrapeConfiguration());
    });
}

void ESP32Gateway::sendMetrics(WebServer& web)
{
    web.sendHeader("Cache-Control", "no-store");
    web.send(200, "text/plain; version=0.0.4; charset=utf-8", metrics.exposition());
}

void ESP32Gateway::startMetricsServer()
{
    metricsServer.stop();
    activeMetricsPort = metrics.configuration().port;
    if (activeMetricsPort != 80) metricsServer.begin(activeMetricsPort);
    metricsPortPending = false;
    Serial.printf("[METRICS] Porta %u; interface web na porta 80.\n", activeMetricsPort);
}

void ESP32Gateway::showPrometheus(int status, const String& notice)
{
    const auto& config = metrics.configuration();
    String page = "<!doctype html><html lang='pt-BR'><head><meta charset='utf-8'>"
                  "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                  "<title>ESP32-C3 | Prometheus</title><style>";
    page += webStyles;
    page += "</style></head><body><main><h1>Prometheus</h1>"
            "<nav><a href='/info'>Painel</a><a href='/wifi'>Wi-Fi</a>"
            "<a href='/prometheus' aria-current='page'>Prometheus</a></nav>";
    if (notice.length())
        page += "<section role='status'>" + escapeHtml(notice) + "</section>";
    page += "<section><h2>Configuracao persistente</h2>"
            "<form action='/prometheus' method='post'>"
            "<label for='host'>Endereco do servidor Prometheus (IPv4 ou DNS)</label>"
            "<input id='host' name='host' maxlength='253' required placeholder='prometheus.local' value='";
    page += escapeHtml(String(config.host));
    page += "'><label for='port'>Porta de metricas do ESP32</label>"
            "<input id='port' name='port' type='number' min='1' max='65535' required value='";
    page += String(config.port);
    page += "'><label for='interval'>Intervalo de coleta (segundos)</label>"
            "<input id='interval' name='interval' type='number' min='5' max='3600' required value='";
    page += String(config.interval);
    page += "'><button type='submit'>Salvar nas preferencias</button></form>"
            "<p>Informe o servidor sem protocolo, caminho ou porta. Esse endereco e uma referencia "
            "salva: nao configura envio ativo nem restringe quem pode coletar. O Prometheus consulta "
            "este ESP32 na porta acima. Painel e configuracao continuam na porta 80.</p>"
            "<p>A porta muda cerca de um segundo depois de salvar, sem reiniciar. Se escolher 80, "
            "somente /metrics compartilha o servidor web.</p></section>"
            "<section><h2>Configuracao do servidor</h2>"
            "<p>Adicione o job abaixo ao servidor Prometheus e recarregue a configuracao. "
            "Salvar aqui nao configura o servidor automaticamente.</p><pre style='white-space:pre-wrap'>";
    page += escapeHtml(metrics.scrapeConfiguration());
    const String ip = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() :
                                                    WiFi.softAPIP().toString();
    page += "</pre><p><a href='/prometheus.yml'>Baixar YAML</a> | <a href='http://" +
            escapeHtml(ip) + ":" + String(config.port) +
            "/metrics' target='_blank' rel='noopener'>Ver metricas</a></p></section>"
            "<section><h2>Metricas reais</h2><p>Identificacao, disponibilidade, Wi-Fi, CPU, temperatura interna da CPU, idade dos dados de clima, tarefas, "
            "memoria, flash, botoes GPIO8/9/10, LED GPIO11 e requisicoes HTTP dos dois servidores. "
            "Uso de CPU requer estatisticas do SDK e duas amostras de cinco segundos.</p>"
            "<p>Energia, bateria, sensores ambientais, reles, ADC e MQTT nao estao integrados; "
            "essas series sao omitidas. O clima da Internet nao representa um sensor local.</p>"
            "<p>Boot e watchdog persistem na NVS. HTTP e desconexoes reiniciam no boot. "
            "Use a metrica up do Prometheus para detectar o dispositivo desligado.</p>"
            "<p>Os endpoints nao possuem autenticacao; use apenas em rede confiavel.</p></section>";
    if (!metrics.persistenceOk())
        page += "<section role='alert'>Contadores persistentes indisponiveis. Consulte a Serial.</section>";
    page += "</main></body></html>";
    server.sendHeader("Cache-Control", "no-store");
    server.send(status, "text/html; charset=utf-8", page);
}

void ESP32Gateway::savePrometheus()
{
    String host = server.arg("host");
    host.trim();
    uint16_t port, interval;
    if (!MetricsService::validHost(host) ||
        !MetricsService::parseNumber(server.arg("port"), 1, 65535, port) ||
        !MetricsService::parseNumber(server.arg("interval"), 5, 3600, interval))
    {
        showPrometheus(400, "Use IPv4/DNS sem protocolo, porta 1..65535 e intervalo 5..3600 segundos.");
        return;
    }
    if (!metrics.save(host, port, interval))
    {
        Serial.println("[METRICS] Falha ao salvar preferencias.");
        showPrometheus(503, "Falha ao salvar na NVS. A configuracao ativa foi preservada.");
        return;
    }
    metricsPortPending = port != activeMetricsPort;
    metricsPortSavedAt = millis();
    showPrometheus(200, "Preferencias salvas. Interface web na porta 80. Aplique o YAML no Prometheus.");
}

void ESP32Gateway::loadWifiCredentials()
{
    if (!prefs.begin("wifi", false))
    {
        Serial.println("[WIFI ERRO] Nao foi possivel abrir as credenciais salvas.");
        return;
    }

    bool hasSavedNetwork = false;
    for (uint8_t i = 0; i < 5; i++)
    {
        String ssidKey = "ssid" + String(i);
        String passwordKey = "pass" + String(i);
        savedNetworks_[i].ssid = prefs.getString(ssidKey.c_str(), "");
        savedNetworks_[i].password = prefs.getString(passwordKey.c_str(), "");
        hasSavedNetwork = hasSavedNetwork || savedNetworks_[i].ssid.length() > 0;
    }

    if (!hasSavedNetwork)
    {
        String legacySsid = prefs.getString("ssid", "");
        if (legacySsid.length() > 0)
        {
            String legacyPassword = prefs.getString("senha", "");
            size_t ssidBytes = prefs.putString("ssid0", legacySsid);
            size_t passwordBytes = prefs.putString("pass0", legacyPassword);
            size_t nextBytes = prefs.putUChar("next", 1);
            if (ssidBytes == legacySsid.length() && passwordBytes == legacyPassword.length() &&
                nextBytes == 1 && prefs.isKey("ssid0") && prefs.isKey("pass0") &&
                prefs.getString("ssid0", "") == legacySsid &&
                prefs.getString("pass0", "") == legacyPassword)
            {
                savedNetworks_[0].ssid = legacySsid;
                savedNetworks_[0].password = legacyPassword;
                prefs.remove("ssid");
                prefs.remove("senha");
            }
            else
            {
                Serial.println("[WIFI ERRO] Falha ao migrar a credencial Wi-Fi antiga.");
            }
        }
    }

    nextWifiSlot_ = prefs.getUChar("next", 0) % 5;
    prefs.end();
}

String ESP32Gateway::buildConfigPage() const
{
    String page = R"rawliteral(
<!doctype html><html lang="pt-BR"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Configuracao Wi-Fi</title><style>
body{font:16px system-ui,sans-serif;background:#0b1220;color:#e5eefb;margin:0;padding:24px}
main{max-width:600px;margin:auto}h1,h2{color:#67e8f9}
section{background:#152238;border:1px solid #263953;border-radius:12px;padding:18px;margin:16px 0}
label{display:block;margin:12px 0 5px}input{box-sizing:border-box;width:100%;padding:10px;border-radius:6px}
button{margin-top:14px;padding:10px 16px;background:#0891b2;color:white;border:0;border-radius:6px}
li{margin:8px 0}a{color:#67e8f9}
</style></head><body><main><h1>Wi-Fi do ESP32-C3</h1>
<section><h2>Adicionar rede</h2><p>A lista circular guarda cinco redes e suas senhas. Uma rede nova ocupa o proximo slot: 1, 2, 3, 4, 5 e volta ao 1, substituindo a rede anterior. Cadastrar um SSID existente atualiza a senha sem avancar a lista.</p>
<form action="/salvar" method="POST">
<label for="ssid">SSID</label><input id="ssid" name="ssid" maxlength="32" required>
<label for="senha">Senha (deixe vazia para rede aberta)</label>
<input id="senha" name="senha" type="password" maxlength="64">
<p><a href="/prometheus">Configurar Prometheus</a> (interface web na porta 80)</p>
<p class="muted">Use uma rede de 2,4 GHz. Ao salvar, o dispositivo reinicia e tenta conectar às redes cadastradas.</p>
<button type="submit">Salvar e reiniciar</button></form></section>
<section><h2>Redes salvas</h2><ol>)rawliteral";

    for (uint8_t slot = 0; slot < 5; slot++)
    {
        page += "<li>";
        page += savedNetworks_[slot].ssid.length() > 0 ? escapeHtml(savedNetworks_[slot].ssid)
                                                       : "<em>Vazio</em>";
        if (slot == nextWifiSlot_)
            page += " <small>(proximo slot para uma rede nova)</small>";
        if (WiFi.status() == WL_CONNECTED && savedNetworks_[slot].ssid == WiFi.SSID())
            page += " <strong>(conectada)</strong>";
        page += "</li>";
    }

    page +=
        R"rawliteral(</ol><p>Senhas nao sao exibidas. As redes disponiveis serao verificadas no proximo reinicio.</p>
<a href="/info">Ver informações do dispositivo</a></section><section><h2>Ajuda para conectar</h2><p>Informe o nome da rede exatamente como aparece no roteador, incluindo maiúsculas e espaços. A senha diferencia maiúsculas de minúsculas.</p><p>As redes ficam salvas mesmo após desligar. A lista comporta cinco perfis; ao completar a lista, uma nova rede substitui o perfil marcado como próximo slot. Atualizar uma rede existente apenas troca sua senha.</p><p>Se a conexão falhar, conecte-se a <strong>ESP32_C3_CONFIG</strong> e abra <strong>192.168.4.1</strong> para revisar o cadastro. Redes ocultas não são encontradas pela busca atual.</p></section><footer>As senhas não são exibidas neste painel.</footer></main></body></html>)rawliteral";
    page.replace("</style>", String(webStyles) + "</style>");
    return page;
}

void ESP32Gateway::clearConfig()
{
    cleanConfig();
}

bool ESP32Gateway::receiveCommand(String& comando)
{
    comando = "";

    int packetSize = udp.parsePacket();
    if (packetSize <= 0)
    {
        return false;
    }

    metrics.recordUdpPacket();

    char packetBuffer[256];
    int len = udp.read(packetBuffer, sizeof(packetBuffer) - 1);
    if (len <= 0)
    {
        return false;
    }

    packetBuffer[len] = '\0';
    comando = String(packetBuffer);
    comando.trim();

    return comando.length() > 0;
}

bool ESP32Gateway::connectWifi()
{
    bool hasSavedNetwork = false;
    for (uint8_t i = 0; i < 5; i++)
    {
        hasSavedNetwork = hasSavedNetwork || savedNetworks_[i].ssid.length() > 0;
    }
    if (!hasSavedNetwork)
        return false;

    WiFi.mode(WIFI_STA);
    rgbLed.blue();
    Serial.println("Procurando redes Wi-Fi salvas...");

    display.println("Buscando redes...");

    // CORREÇÃO 1: Força o escaneamento síncrono (true) e ativa a busca de redes ocultas (false) se
    // necessário
    int networkCount = WiFi.scanNetworks(false, false, false, 300);
    if (networkCount < 0)
    {
        Serial.printf("[WIFI ERRO] Falha ao escanear redes: %d\n", networkCount);
        WiFi.scanDelete();
        return false;
    }

    bool networkVisible[5] = {};
    for (int networkIndex = 0; networkIndex < networkCount; networkIndex++)
    {
        // CORREÇÃO 2: Garante a captura estável da String do SSID escaneado
        String detectedSsid = WiFi.SSID(networkIndex);
        for (uint8_t credentialIndex = 0; credentialIndex < 5; credentialIndex++)
        {
            if (savedNetworks_[credentialIndex].ssid.length() > 0 &&
                savedNetworks_[credentialIndex].ssid == detectedSsid)
            {
                networkVisible[credentialIndex] = true;
            }
        }
    }
    WiFi.scanDelete(); // Limpa a memória do scan

    for (uint8_t offset = 0; offset < 5; offset++)
    {
        uint8_t slot = (nextWifiSlot_ + offset) % 5;
        if (!networkVisible[slot])
            continue;

        const String& ssid = savedNetworks_[slot].ssid;
        const String& password = savedNetworks_[slot].password;

        display.println("Tentando SSID:");
        display.println(ssid);
        Serial.println("Tentando SSID: " + ssid);

        // CORREÇÃO 3: Desconecta limpando totalmente o cache de conexões anteriores antes de tentar
        // a nova
        WiFi.disconnect(true);
        delay(100);

        WiFi.begin(ssid.c_str(), password.c_str());

        // Aguarda até 10 segundos pela conexão
        for (uint8_t attempt = 0; attempt < 20 && WiFi.status() != WL_CONNECTED; attempt++)
        {
            delay(500);
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            rgbLed.green();
            display.println("Wi-Fi conectado");
            display.println(WiFi.localIP().toString());
            Serial.println("Conexao Wi-Fi estabelecida.");
            Serial.println(WiFi.localIP().toString());
            return true;
        }

        Serial.println("Falha ao conectar nesta rede; tentando a proxima.");
    }

    Serial.println("Nenhuma rede salva disponivel aceitou a conexao.");
    return false;
}

bool ESP32Gateway::addWifiProfile(String ssid, const String& password, String& error)
{
    ssid.trim();
    if (ssid.length() == 0 || ssid.length() > 32 || password.length() > 64)
    {
        error = "SSID ou senha invalidos.";
        return false;
    }
    int existingSlot = -1;
    for (uint8_t i = 0; i < 5; i++)
    {
        if (savedNetworks_[i].ssid == ssid)
        {
            existingSlot = i;
            break;
        }
    }

    uint8_t slot = existingSlot >= 0 ? static_cast<uint8_t>(existingSlot) : nextWifiSlot_;
    String ssidKey = "ssid" + String(slot);
    String passwordKey = "pass" + String(slot);

    if (!prefs.begin("wifi", false))
    {
        error = "Nao foi possivel abrir o armazenamento.";
        return false;
    }

    size_t ssidBytes = prefs.putString(ssidKey.c_str(), ssid);
    size_t passwordBytes = prefs.putString(passwordKey.c_str(), password);
    if (ssidBytes != ssid.length() || passwordBytes != password.length() ||
        !prefs.isKey(ssidKey.c_str()) || !prefs.isKey(passwordKey.c_str()) ||
        prefs.getString(ssidKey.c_str(), "") != ssid ||
        prefs.getString(passwordKey.c_str(), "") != password)
    {
        prefs.end();
        error = "Falha ao gravar a rede.";
        return false;
    }

    if (existingSlot < 0)
    {
        uint8_t nextSlot = (slot + 1) % 5;
        if (prefs.putUChar("next", nextSlot) != 1)
        {
            prefs.end();
            error = "Falha ao atualizar a lista circular.";
            return false;
        }
        nextWifiSlot_ = nextSlot;
    }
    prefs.end();

    savedNetworks_[slot].ssid = ssid;
    savedNetworks_[slot].password = password;
    return true;
}

void ESP32Gateway::saveWifi()
{
    String error;
    if (!addWifiProfile(server.arg("ssid"), server.arg("senha"), error))
    {
        server.send(400, "text/plain; charset=utf-8", error);
        return;
    }
    server.send(200, "text/html; charset=utf-8", "<h2>Rede salva. Reiniciando...</h2>");
    delay(2000);
    ESP.restart();
}

void ESP32Gateway::initPortal()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32_C3_CONFIG");
    rgbLed.yellow();

    display.println("Portal Ativo!");
    display.println("Wifi: ESP32_C3_CONFIG");
    display.println("IP: 192.168.4.1");

    server.on("/", HTTP_GET,
              [this]() { server.send(200, "text/html; charset=utf-8", buildConfigPage()); });
    setupInfoRoutes();
    server.begin();
}

void ESP32Gateway::cleanConfig()
{
    display.println("Limpando Memoria...");

    prefs.begin("wifi", false);
    prefs.clear();
    prefs.end();

    sendMessage("WiFi zerado. Reiniciando...\n");

    for (int i = 0; i < 10; i++)
    {
        rgbLed.breathing(255, 0, 0, 100);
    }
    ESP.restart();
}

void ESP32Gateway::sendMessage(String message)
{
    if (!udp.remoteIP() || !udp.remotePort())
    {
        return;
    }

    // Evita o envio automatico interno do buffer UDP (1460 bytes), cujo
    // resultado nao e propagado por write(). Conta cada datagrama explicitamente.
    constexpr size_t MAX_REPLY_PACKET = 1200;
    size_t offset = 0;
    do
    {
        const size_t remaining = message.length() - offset;
        const size_t length = remaining < MAX_REPLY_PACKET ? remaining : MAX_REPLY_PACKET;
        if (udp.beginPacket(udp.remoteIP(), udp.remotePort()) != 1)
        {
            metrics.recordUdpSend(false);
            return;
        }
        const size_t written = udp.write(
            reinterpret_cast<const uint8_t*>(message.c_str() + offset), length);
        if (written != length)
        {
            metrics.recordUdpSend(false);
            return;
        }
        const bool sent = udp.endPacket() == 1;
        metrics.recordUdpSend(sent);
        if (!sent) return;
        offset += length;
    } while (offset < message.length());
}

String ESP32Gateway::savedSsid(uint8_t slot) const
{
    return slot < 5 ? savedNetworks_[slot].ssid : String("");
}

void ESP32Gateway::checkNetworkWatchdog()
{
    // Se estiver conectado, atualiza o timestamp e sai
    if (WiFi.status() == WL_CONNECTED)
    {
        networkSeenAt_ = millis();
        return;
    }

    // CORREÇÃO: Se estiver em modo Portal (Access Point), desativa o reinício automático
    // para permitir que o usuário configure o dispositivo sem pressa.
    if (WiFi.getMode() == WIFI_AP)
    {
        networkSeenAt_ = millis(); // Posterga o watchdog continuamente enquanto estiver no portal
        return;
    }

    // Se não está conectado e passou de 30 minutos
    if (millis() - networkSeenAt_ >= 30UL * 60UL * 1000UL)
    {
        Serial.println("[REDE] 30 minutos sem conexao; reiniciando busca.");
        display.println("Sem WiFi por 30min");
        display.println("Reiniciando busca...");
        delay(250);
        ESP.restart();
    }
}
