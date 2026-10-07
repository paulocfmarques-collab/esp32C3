#include "RGBLed.h"
#include "DisplayUtil.h"
#include "ESP32Gateway.h"
#include "NTPUtil.h"
#include "CommandProcessor.h"
#include "OTAManager.h"
#include "ClimaManager.h"
#include <WiFi.h>

constexpr uint8_t USER_BUTTON_PIN = 10; // Key2: paginas e pressao longa
constexpr uint8_t PAGE_BUTTON_PIN = 8; // Key1: paginas
constexpr uint8_t BOOT_BUTTON_PIN = 9; // BOOT: paginas
volatile bool pageButtonPending = false;
void ARDUINO_ISR_ATTR onPageButtonPressed() { pageButtonPending = true; }

DisplayUtil display;
RGBLed rgbLed;
ESP32Gateway gateway;
NTPUtil ntp;
CommandProcessor commandProcessor(display, gateway, ntp, rgbLed);

bool otaInicializadoCompleto = false;
uint32_t tempoUltimoComando = 0; // Monitor de ociosidade

void setup() 
{
  Serial.begin(115200);
  delay(100); 
  pinMode(USER_BUTTON_PIN, INPUT_PULLUP);
  pinMode(PAGE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PAGE_BUTTON_PIN), onPageButtonPressed, FALLING);
  attachInterrupt(digitalPinToInterrupt(BOOT_BUTTON_PIN), onPageButtonPressed, FALLING);

  display.begin();
  display.setRotation(1); 
  display.clear();
  display.println("Sistema Inicializando...");
  delay(200); 

  commandProcessor.begin();
  delay(100);

  rgbLed.begin();
  gateway.begin();
  ntp.carregarConfiguracoes();

  if (WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED)
  {
    ntp.initNTP();
    
    // Sincroniza o clima logo após obter o horário correto da rede
    ClimaManager::atualizar();
    
    OTAManager::begin("ESP32-C3-Gateway");
    otaInicializadoCompleto = true;
  }
  tempoUltimoComando = millis();
}


void loop() 
{
  static bool showingDashboard = false;
  static uint32_t responseUntil = 0;
  static uint32_t lastClockUpdate = 0;
  static uint8_t currentPage = 0;
  static uint32_t statusPageShownAt = 0;
  static int lastButtonReading = HIGH;
  static int buttonState = HIGH;
  static uint32_t lastDebounceTime = 0;
  static uint32_t buttonPressedAt = 0;
  static uint8_t holdStage = 0;
  static bool pageButtonLatched = false;
  static uint32_t pageButtonReleasedAt = 0;
  String comando;

  gateway.handleClient();
  commandProcessor.update();  

  // Captura por interrupcao preserva toques durante redesenho ou consulta HTTP.
  if (pageButtonLatched) {
    pageButtonPending = false;
    if (digitalRead(PAGE_BUTTON_PIN) == HIGH && digitalRead(BOOT_BUTTON_PIN) == HIGH) {
      if (pageButtonReleasedAt == 0) pageButtonReleasedAt = millis();
      if (millis() - pageButtonReleasedAt >= 50) pageButtonLatched = false;
    } else pageButtonReleasedAt = 0;
  } else if (pageButtonPending) {
    pageButtonPending = false;
    pageButtonLatched = true;
    pageButtonReleasedAt = 0;
    currentPage = (currentPage + 1) % 5;
    display.setRotation(1);
    showingDashboard = true;
    lastClockUpdate = 0;
    statusPageShownAt = millis();
    tempoUltimoComando = millis();
    Serial.printf("[BOTAO] Pagina %u\n", currentPage);
  }

  int buttonReading = digitalRead(USER_BUTTON_PIN);
  if (buttonReading != lastButtonReading)
  {
    lastDebounceTime = millis();
  }
  if (millis() - lastDebounceTime >= 50 && buttonReading != buttonState)
  {
    buttonState = buttonReading;
    if (buttonState == LOW)
    {
      buttonPressedAt = millis();
      holdStage = 0;
    }
    else if (holdStage == 0)
    {
      currentPage = (currentPage + 1) % 5;
      display.setRotation(1);
      showingDashboard = true;
      lastClockUpdate = 0;
      statusPageShownAt = millis();
      tempoUltimoComando = millis();
    }
    else if (holdStage == 1)
    {
      display.showHoldMessage("REINICIANDO", "", 0x07FF);
      delay(500);
      ESP.restart();
    }
    else
    {
      display.showHoldMessage("WI-FI APAGADO", "Reiniciando...", 0xF800);
      delay(500);
      gateway.clearConfig();
    }
  }
  lastButtonReading = buttonReading;

  if (buttonState == LOW)
  {
    uint32_t held = millis() - buttonPressedAt;
    if (held >= 10000 && holdStage < 2)
    {
      holdStage = 2;
      rgbLed.red();
      display.showHoldMessage("APAGAR WI-FI", "Solte p/ confirmar", 0xF800);
    }
    else if (held >= 3000 && holdStage < 1)
    {
      holdStage = 1;
      rgbLed.blue();
      display.showHoldMessage("REINICIAR", "Solte (ou segure 10s)", 0x07FF);
    }
    tempoUltimoComando = millis();
    statusPageShownAt = millis();
  }

  if (otaInicializadoCompleto && WiFi.status() == WL_CONNECTED) 
  {
    OTAManager::handle();
    ClimaManager::atualizar(); // Mantém o clima sincronizado a cada 15 min
  }
  else if (!otaInicializadoCompleto && WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED)
  {
    OTAManager::begin("ESP32-C3-Gateway");
    otaInicializadoCompleto = true;
  }

  // Se receber comando UDP, zera o temporizador do Screensaver e acorda o display
  if (gateway.receiveCommand(comando))
  {
    display.setRotation(0);
    showingDashboard = false;
    commandProcessor.executeCommand(comando);
    responseUntil = millis() + 10000;
    tempoUltimoComando = millis(); // Reseta Protetor de Tela
  }

  commandProcessor.update();

  // Controle de estados da tela (Console Log -> Relógio/Dashboard -> Protetor de Tela)
  if (!showingDashboard && static_cast<int32_t>(millis() - responseUntil) >= 0)
  {
    showingDashboard = true;
    display.setRotation(1);
    display.clear(); 
    lastClockUpdate = 0;
  }

  if (showingDashboard && holdStage == 0)
  {
    if (currentPage != 0 && millis() - statusPageShownAt >= 30000)
    {
      currentPage = 0;
      lastClockUpdate = 0;
    }

    // Se estiver ocioso há mais de 15 minutos, roda a cascata Sci-Fi hacker
    if (millis() - tempoUltimoComando > 900000) 
    {
      display.desenharMatrixScreensaver();
      lastClockUpdate = millis(); // Evita desenhar o relógio por cima
    } 
    // Caso contrário, renderiza o painel completo atualizado de 1 em 1 segundo
    else if (millis() - lastClockUpdate >= 1000) 
    {
      if (currentPage == 0)
      {
        String dateTime;
        ntp.getDateTime(dateTime, 10);
        display.showClock(dateTime);
      }
      else if (currentPage == 1)
      {
        bool ntpSynchronized = ntp.isSincronizado();
        String ntpDateTime = ntp.getDateTime(50);
        display.showStatusPage(ntpSynchronized, ntpDateTime,
                               ntp.getFusoHora(), ntp.isDstAtivo());
      }
      else if (currentPage == 2)
      {
        display.showNetworkPage();
      }
      else if (currentPage == 3) display.showSystemPage();
      else if (currentPage == 4) {
        String ssids[5]; int connectedSlot = -1;
        for (int i=0;i<5;i++) {
          ssids[i]=gateway.savedSsid(i);
          if (WiFi.status()==WL_CONNECTED && ssids[i]==WiFi.SSID()) connectedSlot=i;
        }
        display.showSavedWifiPage(ssids,connectedSlot,gateway.nextSlot());
      }
      lastClockUpdate = millis();
    }
  }
}
