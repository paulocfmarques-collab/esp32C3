#ifndef DISPLAY_UTIL_H
#define DISPLAY_UTIL_H

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <vector>

class DisplayUtil {
private:
    Arduino_GFX* gfx;
    std::vector<String> lines;
    uint16_t textColor;
    uint16_t backgroundColor;
    uint8_t textSize;
    void drawWifiSignal();
    void drawWifiHistory();
    uint32_t lastRssiSample = 0;
    bool rssiSampled = false;
    
    String ultimaHora;
    String ultimaData;

    int historicoRSSI[50]; // Tamanho fixo do array explícito
    int ponteiroHistorico;


public:
    DisplayUtil();
    void begin();
    void clear();
    void setTextColor(uint16_t color);
    void setBackgroundColor(uint16_t color);
    void setTextSize(uint8_t size);
    void setRotation(uint8_t rotation);
    
    void showClock(String dateTime);
    void showStatusPage(bool synced, const String& dateTime, int32_t offset, bool dst);
    void showNetworkPage();
    void showSystemPage();
    void showSavedWifiPage(const String ssids[5], int connectedSlot, uint8_t nextSlot);
    void showHoldMessage(const String& line1, const String& line2, uint16_t color);
    void desenharMatrixScreensaver(); 
    
    void print(String text);
    void println(String text);
    void addLine(String line);
    void redraw();
    
    Arduino_GFX* getDisplay();
};

#endif
