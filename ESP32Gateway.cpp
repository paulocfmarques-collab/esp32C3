#include "ESP32Gateway.h"
#include "DisplayUtil.h"
#include "RGBLed.h"

extern DisplayUtil display;
extern RGBLed rgbLed;

namespace {
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
<meta http-equiv="refresh" content="10"><title>ESP32-C3 - Informacoes</title>
<style>
body{font:16px system-ui,sans-serif;background:#0b1220;color:#e5eefb;margin:0;padding:24px}
main{max-width:760px;margin:auto}h1{color:#67e8f9}h2{color:#a5f3fc;font-size:1.15rem}
section{background:#152238;border:1px solid #263953;border-radius:12px;padding:16px;margin:16px 0}
dl{display:grid;grid-template-columns: minmax(120px,1fr) 2fr;gap:10px;margin:0}
dt{color:#9fb3ca}dd{margin:0;overflow-wrap:anywhere}
small{color:#9fb3ca}
</style></head><body><main><h1>ESP32-C3 Gateway</h1>
<small>Informacoes atualizadas automaticamente a cada 10 segundos</small>
<section><h2>Sistema</h2><dl>
<dt>Modelo</dt><dd>)rawliteral";
  page += escapeHtml(String(ESP.getChipModel()));
  page += R"rawliteral(</dd><dt>Uptime</dt><dd>)rawliteral";
  page += String(uptimeSeconds / 86400UL) + " d " +
          String((uptimeSeconds / 3600UL) % 24UL) + " h " +
          String((uptimeSeconds / 60UL) % 60UL) + " min";
  page += R"rawliteral(</dd></dl></section>
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
  page += R"rawliteral(</dd></dl></section>
<p><a href="/wifi">Gerenciar redes Wi-Fi</a></p></main></body></html>)rawliteral";
  return page;
}
}

ESP32Gateway::ESP32Gateway()
  : server(80)
{
}

void ESP32Gateway::begin()
{
    loadWifiCredentials();

    // Tenta conectar ao WiFi
    if (!connectWifi())
    {
        initPortal();
    }
    else
    {
        server.on("/", HTTP_GET, [this]() {
          server.sendHeader("Location", "/info");
          server.send(302, "text/plain", "");
        });
        setupInfoRoutes();
        server.begin();
    }

    udp.begin(udpPort);
    networkSeenAt_ = millis();
} 

void ESP32Gateway::handleClient()
{
  checkNetworkWatchdog();
  if (WiFi.getMode() == WIFI_AP || WiFi.status() == WL_CONNECTED)
  {
    server.handleClient();
  }
}

void ESP32Gateway::setupInfoRoutes()
{
  server.on("/info", HTTP_GET, [this]() {
    server.send(200, "text/html; charset=utf-8", buildInfoPage());
  });
  setupWifiConfigRoutes();
}

void ESP32Gateway::setupWifiConfigRoutes()
{
  server.on("/wifi", HTTP_GET, [this]() {
    server.send(200, "text/html; charset=utf-8", buildConfigPage());
  });
  server.on("/salvar", HTTP_POST, [this]() {
    saveWifi();
  });
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
      if (ssidBytes == legacySsid.length() &&
          passwordBytes == legacyPassword.length() && nextBytes == 1 &&
          prefs.isKey("ssid0") && prefs.isKey("pass0") &&
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
<button type="submit">Salvar rede</button></form></section>
<section><h2>Redes salvas</h2><ol>)rawliteral";

  for (uint8_t slot = 0; slot < 5; slot++)
  {
    page += "<li>";
    page += savedNetworks_[slot].ssid.length() > 0
              ? escapeHtml(savedNetworks_[slot].ssid) : "<em>Vazio</em>";
    if (slot == nextWifiSlot_)
      page += " <small>(proximo slot para uma rede nova)</small>";
    if (WiFi.status() == WL_CONNECTED && savedNetworks_[slot].ssid == WiFi.SSID())
      page += " <strong>(conectada)</strong>";
    page += "</li>";
  }

  page += R"rawliteral(</ol><p>Senhas nao sao exibidas. As redes disponiveis serao verificadas no proximo reinicio.</p>
<a href="/info">Ver informacoes do dispositivo</a></section></main></body></html>)rawliteral";
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
  if (!hasSavedNetwork) return false;

  WiFi.mode(WIFI_STA);
  rgbLed.blue();
  Serial.println("Procurando redes Wi-Fi salvas...");

  display.println("Buscando redes...");
  
  // CORREÇÃO 1: Força o escaneamento síncrono (true) e ativa a busca de redes ocultas (false) se necessário
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
    if (!networkVisible[slot]) continue;

    const String& ssid = savedNetworks_[slot].ssid;
    const String& password = savedNetworks_[slot].password;
    
    display.println("Tentando SSID:");
    display.println(ssid);
    Serial.println("Tentando SSID: " + ssid);
    
    // CORREÇÃO 3: Desconecta limpando totalmente o cache de conexões anteriores antes de tentar a nova
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

bool ESP32Gateway::addWifiProfile(String ssid, const String& password, String& error) {
  ssid.trim();
  if (ssid.length() == 0 || ssid.length() > 32 || password.length() > 64) {
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
  {    error = "Nao foi possivel abrir o armazenamento.";
    return false;
  }

  size_t ssidBytes = prefs.putString(ssidKey.c_str(), ssid);
  size_t passwordBytes = prefs.putString(passwordKey.c_str(), password);
  if (ssidBytes != ssid.length() ||
      passwordBytes != password.length() ||
      !prefs.isKey(ssidKey.c_str()) || !prefs.isKey(passwordKey.c_str()) ||
      prefs.getString(ssidKey.c_str(), "") != ssid ||
      prefs.getString(passwordKey.c_str(), "") != password)
  {
    prefs.end();    error = "Falha ao gravar a rede.";
    return false;
  }

  if (existingSlot < 0)
  {
    uint8_t nextSlot = (slot + 1) % 5;
    if (prefs.putUChar("next", nextSlot) != 1)
    {
      prefs.end();    error = "Falha ao atualizar a lista circular.";
    return false;
    }
    nextWifiSlot_ = nextSlot;
  }
  prefs.end();

  savedNetworks_[slot].ssid = ssid;
  savedNetworks_[slot].password = password;
  return true;

}

void ESP32Gateway::saveWifi() {
  String error;
  if (!addWifiProfile(server.arg("ssid"), server.arg("senha"), error)) {
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

    server.on("/", HTTP_GET, [this]() {
      server.send(200, "text/html; charset=utf-8", buildConfigPage());
  });
    setupInfoRoutes();
  server.begin();
}

void ESP32Gateway::cleanConfig() 
{
  display.println("Limpando Memoria...");

  prefs.begin("wifi", false);
  prefs.clear(); 
  prefs.end();
  
  udp.beginPacket(udp.remoteIP(), udp.remotePort());
  udp.print("WiFi zerado. Reiniciando...\n");
  udp.endPacket();

  for(int i=0; i<10; i++) 
  {
    rgbLed.breathing(255, 0, 0, 100);
  }
  ESP.restart(); 
}

void ESP32Gateway::sendMessage(String message) 
{
  if (!udp.remoteIP())
  {
    return;
  }

  udp.beginPacket(udp.remoteIP(), udp.remotePort());
  udp.print(message);
  udp.endPacket();
}

String ESP32Gateway::savedSsid(uint8_t slot) const { return slot < 5 ? savedNetworks_[slot].ssid : String(""); }

void ESP32Gateway::checkNetworkWatchdog() {
  // Se estiver conectado, atualiza o timestamp e sai
  if (WiFi.status() == WL_CONNECTED) {
    networkSeenAt_ = millis();
    return;
  }
  
  // CORREÇÃO: Se estiver em modo Portal (Access Point), desativa o reinício automático
  // para permitir que o usuário configure o dispositivo sem pressa.
  if (WiFi.getMode() == WIFI_AP) {
    networkSeenAt_ = millis(); // Posterga o watchdog continuamente enquanto estiver no portal
    return;
  }

  // Se não está conectado e passou de 30 minutos
  if (millis() - networkSeenAt_ >= 30UL * 60UL * 1000UL) {
    Serial.println("[REDE] 30 minutos sem conexao; reiniciando busca.");
    display.println("Sem WiFi por 30min");
    display.println("Reiniciando busca...");
    delay(250);
    ESP.restart();
  }
}

