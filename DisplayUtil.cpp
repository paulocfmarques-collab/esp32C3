#include "DisplayUtil.h"
#include "ClimaManager.h"
#include <WiFi.h>

#include "BoardConfig.h"

#define COLOR_CYAN    0x07FF
#define COLOR_DARK    0x0000
#define COLOR_GRAY    0x5AEB
#define COLOR_WHITE   0xFFFF
#define COLOR_GREEN   0x07E0
#define COLOR_MAGENTA 0xF81F

DisplayUtil::DisplayUtil() {
    textColor = COLOR_WHITE;
    backgroundColor = COLOR_DARK;
    textSize = 1;
    ultimaHora = "";
    ultimaData = "";
    ponteiroHistorico = 0;
    for (int i = 0; i < 50; i++) historicoRSSI[i] = -100;
}

void DisplayUtil::begin() {
    // SPI por software nos pinos fixos do LCD desta placa.
    Arduino_DataBus* bus = new Arduino_SWSPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI);
    panel = new Arduino_ST7735(bus, TFT_RST, 0, TFT_IPS, TFT_WIDTH, TFT_HEIGHT,
                             TFT_OFFSET_X, TFT_OFFSET_Y, TFT_OFFSET_X, TFT_OFFSET_Y);
    panel->begin();
    panel->setRotation(0);
    gfx = new Arduino_Canvas(TFT_WIDTH, TFT_HEIGHT, panel);
    if (!gfx->begin()) {
        // Allocation fallback: retain display operation if RAM is insufficient.
        delete gfx;
        gfx = panel;
        Serial.println("[DISPLAY] Canvas unavailable; using direct rendering.");
    }
    gfx->setTextWrap(false);
    gfx->setRotation((1 + DISPLAY_ROTATION_OFFSET) % 4); 
    clear();
}

void DisplayUtil::clear() {
    lines.clear();
    ultimaHora = "";
    ultimaData = "";
    gfx->fillScreen(backgroundColor);
}

void DisplayUtil::setTextColor(uint16_t color) { textColor = color; }
void DisplayUtil::setBackgroundColor(uint16_t color) { backgroundColor = color; }
void DisplayUtil::setTextSize(uint8_t size) { textSize = constrain(size, 1, 2); }

void DisplayUtil::setRotation(uint8_t rotation) {
    lines.clear();
    ultimaHora = "";
    ultimaData = "";
    gfx->setRotation((rotation + DISPLAY_ROTATION_OFFSET) % 4);
    clear();
}

void DisplayUtil::showClock(String dateTime) {
    gfx->setRotation((1 + DISPLAY_ROTATION_OFFSET) % 4);
    gfx->setTextWrap(false);
    gfx->fillScreen(backgroundColor);
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_CYAN);
    gfx->setCursor(4, 4); gfx->print("GATEWAY ESP32-C3");
    drawWifiSignal();
    gfx->drawFastHLine(0, 16, gfx->width(), COLOR_GRAY);
    if (dateTime.length() < 19 || dateTime == "Erro ao obter data e hora") {
        gfx->setCursor(10, 48); gfx->print("Aguardando NTP");
        gfx->setCursor(10, 64); gfx->print("Verifique o WiFi");
        gfx->flush();
        return;
    }
    int separator = dateTime.indexOf(' ');
    String data = dateTime.substring(0, separator);
    String hora = dateTime.substring(separator + 1, separator + 9);
    String hhmm = hora.substring(0, 5);
    if (hora.substring(6,8).toInt() % 2) hhmm.setCharAt(2, ' ');
    gfx->setTextColor(COLOR_WHITE); gfx->setTextSize(3);
    gfx->setCursor(4, 24); gfx->print(hhmm);
    gfx->setTextSize(1); gfx->setTextColor(COLOR_CYAN);
    gfx->setCursor(100, 39); gfx->print(hora.substring(6, 8));
    gfx->setTextColor(COLOR_GREEN);
    gfx->setCursor((gfx->width()-data.length()*6)/2, 55); gfx->print(data);
    gfx->setTextColor(COLOR_CYAN); gfx->setCursor(4, 72);
    if (ClimaManager::sincronizado) gfx->printf("%.1f C", ClimaManager::temperatura);
    else gfx->print("Clima sem dados");
    gfx->setCursor(4, 84);
    if (ClimaManager::sincronizado) gfx->print(ClimaManager::obterTextoCondicao().substring(0,20));
    drawWifiHistory();
    gfx->fillRect(4, 118, 120, 3, COLOR_GRAY);
    gfx->fillRect(4, 118, hora.substring(6,8).toInt()*120/59, 3, COLOR_CYAN);
    ultimaHora = hora; ultimaData = data;
    gfx->flush();
}

void DisplayUtil::desenharMatrixScreensaver() {
    int colunas = gfx->width() / 12;
    for (int i = 0; i < 3; i++) { 
        int x = random(0, colunas) * 12;
        int y = random(0, gfx->height() / 16) * 16;
        
        gfx->fillRect(x, y, 10, 14, backgroundColor); 
        
        uint16_t tomVerde = (random(0, 10) > 3) ? COLOR_GREEN : 0x0400;
        gfx->setTextColor(tomVerde);
        gfx->setTextSize(1);
        gfx->setCursor(x, y);
        gfx->print((char)random(33, 126)); 
    }
    delay(10); 
    gfx->flush();
}

void DisplayUtil::print(String text) { println(text); }

void DisplayUtil::println(String text) {
    text.replace("\r", ""); text.replace("\n", "");
    int charWidth = 6 * textSize; 
    int maxChars = (gfx->width() - 24) / charWidth;
    while (text.length() > maxChars) {
        addLine(text.substring(0, maxChars));
        text = text.substring(maxChars);
    }
    if (text.length() > 0) addLine(text);
    redraw();
}

void DisplayUtil::addLine(String line) {
    lines.push_back(line);
    int lineHeight = 8 * textSize; 
    int maxLines = (gfx->height() - 45) / lineHeight; 
    while ((int)lines.size() > maxLines) lines.erase(lines.begin());
}

void DisplayUtil::redraw() {
    gfx->fillScreen(backgroundColor);
    gfx->fillRect(0, 0, gfx->width(), 26, 0x2104); 
    gfx->drawFastHLine(0, 26, gfx->width(), COLOR_CYAN); 
    gfx->setTextColor(COLOR_WHITE); gfx->setTextSize(1); gfx->setCursor(12, 9);
    gfx->print("> CONSOLE C3"); drawWifiSignal();
    int y = 35; int lineHeight = 8 * textSize; gfx->setTextSize(textSize);
    for (size_t i = 0; i < lines.size(); i++) {
        if (lines[i].startsWith(">")) { gfx->setTextColor(COLOR_GREEN); gfx->setCursor(8, y); }
        else { gfx->setTextColor(0xCE79); gfx->setCursor(12, y); }
        gfx->println(lines[i]); y += lineHeight;
    }
    gfx->flush();
}

Arduino_GFX* DisplayUtil::getDisplay() { return panel; }

void DisplayUtil::showStatusPage(bool synced, const String& dateTime, int32_t offset, bool dst) {
 clear(); gfx->setTextSize(1); gfx->setTextColor(COLOR_CYAN);
 gfx->setCursor(4,4); gfx->print("ESTADO ESP32-C3"); drawWifiSignal();
 gfx->setTextColor(COLOR_WHITE);
 gfx->setCursor(4,24); gfx->printf("CPU: %u MHz", ESP.getCpuFreqMHz());
 gfx->setCursor(4,38); gfx->printf("RAM: %u KB", ESP.getFreeHeap()/1024);
 gfx->setCursor(4,52); gfx->print(synced ? "NTP: OK" : "NTP: aguardando");
 gfx->setCursor(4,66); gfx->printf("UTC: %ld DST: %s", (long)offset, dst ? "ON" : "OFF");
 gfx->setCursor(4,80); gfx->printf("Ligado: %lu s", (unsigned long)(millis()/1000));
 if (dateTime.length()==19) {
  gfx->setCursor(4,94); gfx->print(dateTime.substring(0,10));
  gfx->setCursor(4,108); gfx->print(dateTime.substring(11));
 }
    gfx->flush();
}
void DisplayUtil::showNetworkPage() {
 clear(); gfx->setTextSize(1); gfx->setTextColor(COLOR_CYAN);
 gfx->setCursor(4,4); gfx->print("REDE ESP32-C3"); drawWifiSignal();
 gfx->setTextColor(COLOR_WHITE);
 bool connected = WiFi.status()==WL_CONNECTED;
 gfx->setCursor(4,24); gfx->print(connected ? "WiFi conectado" : "WiFi desconectado");
 gfx->setCursor(4,38); gfx->print(WiFi.getMode()==WIFI_AP ? "ESP32_C3_CONFIG" : WiFi.SSID().substring(0,20));
 gfx->setCursor(4,52); gfx->print(WiFi.getMode()==WIFI_AP ? WiFi.softAPIP().toString() : WiFi.localIP().toString());
 gfx->setCursor(4,66); gfx->printf("Canal: %d", WiFi.channel());
 gfx->setCursor(4,80); if (connected) gfx->printf("RSSI: %d dBm", WiFi.RSSI());
 gfx->setCursor(4,94); gfx->print(WiFi.macAddress());
 gfx->setCursor(4,108); gfx->print("UDP: 4210 / Web: 80");
    gfx->flush();
}

void DisplayUtil::showHoldMessage(const String& line1, const String& line2, uint16_t color) {
 clear(); gfx->setTextSize(1); gfx->setTextColor(color);
 gfx->setCursor(4,44); gfx->print(line1.substring(0,20));
 gfx->setCursor(4,64); gfx->print(line2.substring(0,20));
    gfx->flush();
}
void DisplayUtil::showSystemPage() {
 clear(); gfx->setTextSize(1); gfx->setTextColor(COLOR_CYAN);
 gfx->setCursor(4,4); gfx->print("SISTEMA ESP32-C3"); drawWifiSignal(); gfx->setTextColor(COLOR_WHITE);
 gfx->setCursor(4,24); gfx->print(ESP.getChipModel());
 gfx->setCursor(4,38); gfx->printf("CPU: %u MHz",ESP.getCpuFreqMHz());
 gfx->setCursor(4,52); gfx->printf("RAM livre: %u KB",ESP.getFreeHeap()/1024);
 gfx->setCursor(4,66); gfx->printf("RAM minima: %u KB",ESP.getMinFreeHeap()/1024);
 gfx->setCursor(4,80); gfx->printf("Flash: %u MB",ESP.getFlashChipSize()/1024/1024);
 gfx->setCursor(4,94); gfx->printf("Firmware: %u KB",ESP.getSketchSize()/1024);
 gfx->setCursor(4,108); gfx->printf("Ligado: %lu s",(unsigned long)(millis()/1000));
    gfx->flush();
}
void DisplayUtil::showSavedWifiPage(const String ssids[5], int connectedSlot, uint8_t nextSlot) {
 clear(); gfx->setTextSize(1); gfx->setTextColor(COLOR_CYAN);
 gfx->setCursor(4,4); gfx->print("REDES SALVAS"); drawWifiSignal();
 for (int i=0;i<5;i++) {
  gfx->setTextColor(i==connectedSlot ? COLOR_GREEN : COLOR_WHITE);
  gfx->setCursor(4,24+i*16); gfx->print(String(i+1)+":"+ssids[i].substring(0,17));
 }
 gfx->setTextColor(COLOR_CYAN); gfx->setCursor(4,110);
 gfx->print("Proximo slot: "+String(nextSlot+1));
    gfx->flush();
}

void DisplayUtil::drawWifiSignal() {
    const bool connected = WiFi.status() == WL_CONNECTED;
    const int rssi = connected ? WiFi.RSSI() : -100;
    if (!rssiSampled || millis() - lastRssiSample >= 1000) {
        lastRssiSample = millis(); rssiSampled = true;
        historicoRSSI[ponteiroHistorico] = connected ? constrain(rssi, -99, -30) : -100;
        ponteiroHistorico = (ponteiroHistorico + 1) % 50;
    }
    // Escala local para mostrar pequenas oscilacoes reais, como no grafico.
    // Mantem pelo menos 8 dBm de amplitude (aproximadamente 2 dBm por barra).
    int low = 0, high = -100;
    for (int i = 0; i < 50; ++i) {
        if (historicoRSSI[i] <= -100) continue;
        low = min(low, historicoRSSI[i]);
        high = max(high, historicoRSSI[i]);
    }
    if (high - low < 8) {
        const int center = (high + low) / 2;
        low = center - 4; high = center + 4;
    }
    const int active = connected ? constrain(1 + (constrain(rssi, low, high) - low) * 4 / (high - low + 1), 1, 4) : 0;
    // Cor e quantidade acompanham a mesma escala de variacao recente.
    const uint16_t signalColor = active <= 1 ? 0xF800 : (active == 2 ? 0xFFE0 : COLOR_GREEN);
    const int x = gfx->width() - 22;
    gfx->fillRect(x, 1, 20, 14, backgroundColor);
    for (int bar = 0; bar < 4; ++bar) {
        const int height = (bar + 1) * 3;
        if (bar < active) gfx->fillRect(x + bar * 5, 14 - height, 3, height, signalColor);
        else gfx->drawRect(x + bar * 5, 14 - height, 3, height, COLOR_GRAY);
    }
}

void DisplayUtil::drawWifiHistory() {
    // Historico real de RSSI, uma amostra por segundo. Linha de um pixel.
    const int x = 4, y = 97, width = 120, height = 17;
    int low = 0, high = -100;
    for (int i = 0; i < 50; ++i) {
        if (historicoRSSI[i] <= -100) continue;
        low = min(low, historicoRSSI[i]); high = max(high, historicoRSSI[i]);
    }
    gfx->fillRect(x, y, width, height, backgroundColor);
    if (high == -100) return;
    // Escala adaptativa com amplitude minima de 8 dBm para enxergar variacoes pequenas.
    if (high - low < 8) {
        const int center = (high + low) / 2;
        low = center - 4; high = center + 4;
    }
    for (int i = 0; i < 49; ++i) {
        const int a = historicoRSSI[(ponteiroHistorico + i) % 50];
        const int b = historicoRSSI[(ponteiroHistorico + i + 1) % 50];
        if (a <= -100 || b <= -100) continue;
        const int x1 = x + i * (width - 1) / 49;
        const int x2 = x + (i + 1) * (width - 1) / 49;
        const int y1 = y + height - 1 - map(a, low, high, 0, height - 1);
        const int y2 = y + height - 1 - map(b, low, high, 0, height - 1);
        gfx->drawLine(x1, y1, x2, y2, COLOR_GREEN);
    }
}

void DisplayUtil::showMonitorPage(const NetworkMonitor& m) {
 clear(); gfx->setTextSize(1); gfx->setTextColor(COLOR_CYAN);
 gfx->setCursor(4,4); gfx->print("MONITOR REDE"); drawWifiSignal();
 gfx->setTextColor(COLOR_WHITE); gfx->setCursor(4,20);
 gfx->print(m.connected ? WiFi.gatewayIP().toString() : "WiFi desconectado");
 gfx->setCursor(4,32);
 if(!m.connected) gfx->print("Ping: sem rede");
 else if(!m.hasResult) gfx->print("Ping: aguardando");
 else if(m.replied) gfx->printf("Ping: %lu ms",(unsigned long)m.rtt);
 else gfx->print("Ping: sem resposta");
 gfx->setCursor(4,44); gfx->printf("Quedas:%lu Falha:%lu%%",(unsigned long)m.drops,
 (unsigned long)(m.attempts ? uint64_t(m.failures)*100/m.attempts : 0));
 gfx->setCursor(4,56); gfx->printf("Offline: %lu s",(unsigned long)m.offlineSeconds());
 int peak=10; for(int v:m.history) if(v>peak) peak=v;
 gfx->drawFastHLine(4,88,120,COLOR_GRAY);
 for(int i=0;i<24;i++) {
  int v=m.history[(m.head+i)%24], x=4+i*5;
  if(v==-1) gfx->drawFastVLine(x,68,20,0xF800);
  else if(v>=0) gfx->drawFastVLine(x,88-max(1,v*20/peak),max(1,v*20/peak),COLOR_GREEN);
 }
 for(int i=0;i<3;i++){gfx->setCursor(4,94+i*10);gfx->print(m.events[i].substring(0,20));}
 gfx->flush();
}
void DisplayUtil::showForecastPage() {
 clear(); gfx->setTextSize(1); gfx->setTextColor(COLOR_CYAN);
 gfx->setCursor(4,4); gfx->print("PREVISAO HOJE"); drawWifiSignal();
 gfx->setTextColor(COLOR_WHITE); gfx->setCursor(4,22);gfx->print("Porto Alegre - RS");
 if(!ClimaManager::previsaoValida) {
  gfx->setCursor(4,48);gfx->print("Aguardando previsao");
 } else {
  gfx->setCursor(4,36);gfx->print(ClimaManager::dataPrevisao);
  gfx->setTextColor(COLOR_CYAN);gfx->setCursor(4,54);gfx->printf("Minima: %.1f C",ClimaManager::minima);
  gfx->setTextColor(0xFFE0);gfx->setCursor(4,70);gfx->printf("Maxima: %.1f C",ClimaManager::maxima);
  gfx->setTextColor(COLOR_GREEN);gfx->setCursor(4,86);gfx->printf("Chuva: %d%%",ClimaManager::chuva);
  gfx->setTextColor(COLOR_WHITE);gfx->setCursor(4,104);
  gfx->printf("Ha %lu min",(unsigned long)((millis()-ClimaManager::previsaoAtualizada)/60000));
  gfx->setCursor(4,116);gfx->print(WiFi.status()==WL_CONNECTED ? "Max. diaria de chuva" : "Offline: dado salvo");
 }
 gfx->flush();
}
