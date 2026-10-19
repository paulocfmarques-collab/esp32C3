#ifndef ESP32_GATEWAY_H
#define ESP32_GATEWAY_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <Preferences.h>
#include "MetricsService.h"

class ESP32Gateway
{
public:
    ESP32Gateway();
    void begin();
    void handleClient();
    bool receiveCommand(String& comando);
    void sendMessage(String message);
    void clearConfig();
    bool addWifiProfile(String ssid, const String& password, String& error);
    String savedSsid(uint8_t slot) const;
    uint8_t nextSlot() const { return nextWifiSlot_; }
    bool resetCounters() { return metrics.resetCounters(); }
    

private:
    struct WifiCredential
    {
        String ssid;
        String password;
    };

    bool connectWifi();
    void checkNetworkWatchdog();
    uint32_t networkSeenAt_ = 0;
    void loadWifiCredentials();
    String buildConfigPage() const;
    void initPortal();
    void setupInfoRoutes();
    void setupWifiConfigRoutes();
    void saveWifi();
    void cleanConfig();
    void setupMetricsRoutes();
    void startMetricsServer();
    void sendMetrics(WebServer& web);
    void showPrometheus(int status = 200, const String& notice = "");
    void savePrometheus();

    WiFiUDP udp;
    const int udpPort = 4210;
    WebServer server;
    WebServer metricsServer{9120};
    MetricsService metrics;
    uint16_t activeMetricsPort = 9120;
    bool metricsPortPending = false;
    uint32_t metricsPortSavedAt = 0;
    Preferences prefs;
    WifiCredential savedNetworks_[5];
    uint8_t nextWifiSlot_ = 0;
};

#endif // ESP32_GATEWAY_H