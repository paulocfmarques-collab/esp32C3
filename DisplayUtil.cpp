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
    robotActive = false;
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

void DisplayUtil::desenharRobotScreensaver() {
    const uint32_t now = millis();
    if (!robotActive) {
        robotActive = true;
        robotStartedAt = now;
        robotFrameAt = now - 80;
    }
    // Limita a animacao sem bloquear comandos, botoes ou OTA.
    if (now - robotFrameAt < 80) return;
    robotFrameAt = now;
    const uint32_t elapsed = now - robotStartedAt;
    // Olhos: abertos, fechados, coracao, felizes, bravos, redondos,
    // sonolentos, X, estrela, >/<, lagrimas, curiosos, quadrados, olhar lateral.
    // Boca: sorriso, triste, reta, O, riso, zigue-zague, torta, beijo, dentes,
    // lingua, ausente, surpresa preenchida.
    struct Expression { uint8_t left, right, mouth, brows; };
    static const Expression expressions[] = {
        // Primeiras dez: mesma ordem da referencia, lendo cada linha da esquerda.
        {0,0,0,0}, {1,0,0,0}, {0,0,1,1}, {4,4,1,0}, {0,0,10,0},
        {2,2,10,0}, {0,0,11,0}, {1,1,2,0}, {3,3,0,0}, {0,0,1,0},
        {3,3,4,0}, {8,8,4,0}, {7,7,5,0}, {6,6,2,0}, {11,0,6,2},
        {0,11,3,2}, {9,9,4,0}, {2,2,7,0}, {4,4,8,0}, {0,0,9,0},
        {1,0,9,0}, {0,1,0,0}, {5,5,5,1}, {6,6,3,0}, {12,12,2,0},
        {13,13,6,0}, {14,14,0,0}, {8,8,0,0}, {3,3,8,0}, {7,7,3,0},
        {10,10,3,1}, {11,11,3,2}, {4,4,5,0}, {6,0,6,2}, {0,6,6,2},
        {2,1,7,0}, {9,9,9,0}, {12,12,8,0}, {5,5,4,0}, {3,1,0,0}
    };
    const uint32_t step = elapsed / 4000UL;
    const uint8_t face = step % (sizeof(expressions) / sizeof(expressions[0]));
    const Expression& expression = expressions[face];
    const uint16_t palette[] = {0x07FF, 0x05FF, 0xA81F, 0xF82A, 0x07E0,
                                0xF81F, 0xFFE0, 0xFC00, 0x07FF, 0xB7E0};
    // Referencia: ciano, com rosa para a expressao brava e os coracoes.
    const uint16_t color = face < 10 ? ((face == 3 || face == 5) ? 0xF82A : COLOR_CYAN)
        : palette[(step + step / 40UL) % (sizeof(palette) / sizeof(palette[0]))];
    const bool blink = elapsed % 3200UL >= 2960UL;
    // Pequenos movimentos evitam manter os mesmos pixels acesos.
    const int dx = int((elapsed / 900UL) % 3) - 1;
    const int dy = int((elapsed / 1300UL) % 3) - 1;
    auto x = [&](int v) { return (v + dx) * gfx->width() / 128; };
    auto y = [&](int v) { return (v + dy) * gfx->height() / 128; };
    auto line = [&](int ax, int ay, int bx, int by) {
        for (int t = -1; t <= 1; ++t)
            gfx->drawLine(x(ax), y(ay + t), x(bx), y(by + t), color);
    };
    auto heart = [&](int cx, int cy) {
        const int r = gfx->width() * 7 / 128;
        gfx->fillCircle(x(cx - 6), y(cy - 3), r, color);
        gfx->fillCircle(x(cx + 6), y(cy - 3), r, color);
        gfx->fillTriangle(x(cx - 13), y(cy - 1), x(cx + 13), y(cy - 1), x(cx), y(cy + 17), color);
    };
    auto smile = [&](bool sad) {
        for (int u = -19; u < 19; ++u) {
            const int a = u * u / 48;
            const int b = (u + 1) * (u + 1) / 48;
            line(64 + u, sad ? 97 + a : 105 - a,
                 65 + u, sad ? 97 + b : 105 - b);
        }
    };
    auto cyberneticEye = [&](int cx, uint8_t style) {
        const uint16_t steelDark = 0x1948;
        const uint16_t steel = 0x4B30;
        const uint16_t steelLight = 0x8D37;
        const uint16_t irisDark = 0x0188;
        const uint16_t irisBlue = 0x0352;
        const uint16_t ice = 0xBFFF;
        auto circle = [&](int px, int py, int radius, uint16_t ink, bool filled) {
            if (filled) gfx->fillCircle(x(px), y(py), gfx->width() * radius / 128, ink);
            else gfx->drawCircle(x(px), y(py), gfx->width() * radius / 128, ink);
        };
        auto detail = [&](int ax, int ay, int bx, int by, uint16_t ink) {
            gfx->drawLine(x(ax), y(ay), x(bx), y(by), ink);
        };
        // Direcoes fixas em escala 100: detalhes sem trigonometria por quadro.
        static const int8_t directions[24][2] = {
            {100,0}, {97,26}, {87,50}, {71,71}, {50,87}, {26,97},
            {0,100}, {-26,97}, {-50,87}, {-71,71}, {-87,50}, {-97,26},
            {-100,0}, {-97,-26}, {-87,-50}, {-71,-71}, {-50,-87}, {-26,-97},
            {0,-100}, {26,-97}, {50,-87}, {71,-71}, {87,-50}, {97,-26}
        };
        circle(cx, 49, 23, steelDark, true);
        gfx->fillTriangle(x(cx - 27), y(49), x(cx - 12), y(33), x(cx - 12), y(65), steel);
        gfx->fillTriangle(x(cx + 27), y(49), x(cx + 12), y(33), x(cx + 12), y(65), steel);
        circle(cx, 49, 22, steelLight, false);
        circle(cx, 49, 20, COLOR_DARK, true);
        circle(cx, 49, 19, steel, false);
        detail(cx - 25, 47, cx - 15, 35, steelLight);
        detail(cx + 15, 64, cx + 25, 51, steelLight);
        circle(cx - 23, 49, 1, ice, true);
        circle(cx + 23, 49, 1, ice, true);

        const int gaze = style == 13 ? -3 : style == 14 ? 3
            : int((elapsed / 1800UL) % 5) - 2;
        const int ix = cx + gaze;
        const int iy = 49;
        const int pupil = style == 11 ? 4 : style == 5 ? 8
            : 6 + int((elapsed / 2400UL) % 2);
        circle(ix, iy, 17, irisDark, true);
        circle(ix, iy, 16, COLOR_CYAN, false);
        circle(ix, iy, 14, irisBlue, false);
        circle(ix, iy, 11, irisBlue, false);
        const uint8_t phase = (elapsed / 160UL) % 24;
        for (uint8_t i = 0; i < 24; ++i) {
            const int ux = directions[i][0];
            const int uy = directions[i][1];
            const int inner = 9 + i % 3;
            const int outer = 14 + i % 2;
            const bool lit = (i + 24 - phase) % 24 < 4;
            detail(ix + ux * inner / 100, iy + uy * inner / 100,
                   ix + ux * outer / 100, iy + uy * outer / 100,
                   lit ? ice : COLOR_CYAN);
            gfx->drawPixel(x(ix + ux * 18 / 100), y(iy + uy * 18 / 100),
                           i % 3 == 0 ? ice : irisBlue);
            if (i % 4 == 0) {
                const int vx = directions[(i + 1) % 24][0];
                const int vy = directions[(i + 1) % 24][1];
                detail(ix + ux * 12 / 100, iy + uy * 12 / 100,
                       ix + vx * 12 / 100, iy + vy * 12 / 100, ice);
            }
        }
        circle(ix, iy, pupil + 1, ice, false);
        circle(ix, iy, pupil, COLOR_DARK, true);
        circle(ix - 7, iy - 8, 2, ice, true);
        gfx->drawPixel(x(ix + 9), y(iy + 9), COLOR_WHITE);

        if (style == 4) {
            gfx->fillTriangle(x(cx - 25), y(25), x(cx + 25), y(25),
                              x(cx < 64 ? cx + 25 : cx - 25), y(49), COLOR_DARK);
            detail(cx - 23, cx < 64 ? 36 : 48,
                   cx + 23, cx < 64 ? 48 : 36, COLOR_CYAN);
        }
        if (style == 6) {
            gfx->fillRect(x(cx - 28), y(25), gfx->width() * 56 / 128,
                          gfx->height() * 23 / 128, COLOR_DARK);
            detail(cx - 21, 48, cx + 21, 48, COLOR_CYAN);
        } else if (style == 12) {
            gfx->drawRect(x(cx - 24), y(25), gfx->width() * 49 / 128,
                          gfx->height() * 49 / 128, steelLight);
        }
    };
    gfx->fillScreen(COLOR_DARK);
    for (int eye = 0; eye < 2; ++eye) {
        const int cx = eye ? 94 : 34;
        const uint8_t style = eye ? expression.right : expression.left;
        if (blink || style == 1) {
            line(cx - 13, 49, cx + 13, 49);
        } else if (style == 2) {
            heart(cx, 46);
        } else if (style == 3) {
            // Olhos sorridentes em arco.
            for (int u = -13; u < 13; ++u)
                line(cx + u, 43 + u * u / 22, cx + u + 1, 43 + (u + 1) * (u + 1) / 22);
        } else if (style == 7) {
            line(cx - 12, 35, cx + 12, 63);
            line(cx + 12, 35, cx - 12, 63);
        } else if (style == 8) {
            // Estrelas feitas apenas com tracos luminosos.
            line(cx, 31, cx, 65);
            line(cx - 15, 48, cx + 15, 48);
            line(cx - 11, 36, cx + 11, 60);
            line(cx + 11, 36, cx - 11, 60);
        } else if (style == 9) {
            const int tip = eye ? -10 : 10;
            line(cx - tip, 35, cx + tip, 49);
            line(cx + tip, 49, cx - tip, 63);
        } else {
            cyberneticEye(cx, style);
            if (style == 10) {
                const int dropY = 72 + (elapsed / 120UL) % 8;
                gfx->fillCircle(x(cx), y(dropY), gfx->width() * 4 / 128, color);
            }
        }
        if (expression.brows == 1) {
            // Sobrancelhas preocupadas: pontas internas levantadas.
            line(cx - 13, eye ? 23 : 28, cx + 13, eye ? 28 : 23);
        } else if (expression.brows == 2) {
            line(cx - 12, eye ? 25 : 20, cx + 12, eye ? 25 : 24);
        }
    }
    const uint8_t mouth = expression.mouth;
    if (mouth == 10) {
        // Neutro e apaixonado, como na imagem: somente olhos.
    } else if (mouth == 11) {
        gfx->fillRoundRect(x(56), y(91), gfx->width() * 16 / 128,
                           gfx->height() * 20 / 128, gfx->width() * 7 / 128, color);
    } else if (mouth == 3 || mouth == 7) {
        const int radius = mouth == 7 ? 6 : 11;
        gfx->fillCircle(x(64), y(101), gfx->width() * radius / 128, color);
        gfx->fillCircle(x(64), y(101), gfx->width() * (radius - 4) / 128, COLOR_DARK);
    } else if (mouth == 2) {
        line(49, 101, 79, 101);
    } else if (mouth == 4 || mouth == 8 || mouth == 9) {
        gfx->fillRoundRect(x(43), y(90), gfx->width() * 42 / 128,
                           gfx->height() * 23 / 128, gfx->width() * 8 / 128, color);
        if (mouth == 4) {
            gfx->fillRoundRect(x(48), y(93), gfx->width() * 32 / 128,
                               gfx->height() * 12 / 128, gfx->width() * 4 / 128, COLOR_DARK);
        } else if (mouth == 8) {
            gfx->drawFastHLine(x(46), y(101), gfx->width() * 36 / 128, COLOR_DARK);
            for (int u = 53; u <= 75; u += 11)
                gfx->drawFastVLine(x(u), y(93), gfx->height() * 17 / 128, COLOR_DARK);
        } else {
            gfx->fillRect(x(48), y(94), gfx->width() * 32 / 128, gfx->height() * 10 / 128, COLOR_DARK);
            gfx->fillRoundRect(x(64), y(100), gfx->width() * 13 / 128,
                               gfx->height() * 18 / 128, gfx->width() * 5 / 128, 0xF82A);
        }
    } else if (mouth == 5) {
        for (int u = 0; u < 4; ++u)
            line(44 + u * 10, u % 2 ? 105 : 97, 54 + u * 10, u % 2 ? 97 : 105);
    } else if (mouth == 6) {
        line(47, 105, 65, 102);
        line(65, 102, 81, 94);
    } else {
        smile(mouth == 1);
    }
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
