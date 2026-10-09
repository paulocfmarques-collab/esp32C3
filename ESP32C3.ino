#include "RGBLed.h"
#include "DisplayUtil.h"
#include "ESP32Gateway.h"
#include "NTPUtil.h"
#include "CommandProcessor.h"
#include "OTAManager.h"
#include "ClimaManager.h"
#include <WiFi.h>
#include <Preferences.h>

constexpr uint8_t USER_BUTTON_PIN = 10; // K2: retorna pagina
constexpr uint8_t PAGE_BUTTON_PIN = 8; // Key1: paginas
constexpr uint8_t BOOT_BUTTON_PIN = 9; // BOOT: paginas
volatile bool pageButtonPending = false;
void ARDUINO_ISR_ATTR onPageButtonPressed() { pageButtonPending = true; }

constexpr uint8_t PAGE_COUNT = 7;
NetworkMonitor networkMonitor;
DisplayUtil display;
RGBLed rgbLed;
ESP32Gateway gateway;
NTPUtil ntp;
CommandProcessor commandProcessor(display, gateway, ntp, rgbLed);

bool otaInicializadoCompleto = false;
uint32_t tempoUltimoComando = 0; // Monitor de ociosidade

// Recebe uma linha sem esperar por timeout. LF, CR e CRLF sao aceitos.
// Em caso de excesso, descarta a linha inteira para nao executar um prefixo.
bool receiveSerialCommand(String& command)
{
  constexpr size_t MAX_COMMAND_LENGTH = 256;
  constexpr uint8_t MAX_BYTES_PER_LOOP = 64;
  static char buffer[MAX_COMMAND_LENGTH + 1];
  static size_t length = 0;
  static bool overflow = false;

  for (uint8_t count = 0; count < MAX_BYTES_PER_LOOP && Serial.available() > 0; ++count)
  {
    const int incoming = Serial.read();
    if (incoming < 0) break;
    const char ch = static_cast<char>(incoming);
    if (ch == '\r' || ch == '\n')
    {
      if (overflow)
      {
        Serial.println("[SERIAL ERRO] Comando excede 256 caracteres; linha descartada.");
        length = 0;
        overflow = false;
        continue;
      }
      if (length == 0) continue;
      buffer[length] = '\0';
      command = buffer;
      length = 0;
      command.trim();
      if (command.length() > 0) return true;
    }
    else if (!overflow)
    {
      if (ch == '\b' || ch == 127)
      {
        if (length > 0) --length;
      }
      else if (static_cast<uint8_t>(ch) >= 32)
      {
        if (length < MAX_COMMAND_LENGTH) buffer[length++] = ch;
        else overflow = true;
      }
    }
  }
  return false;
}

void clearSavedSettings() {
  bool success = true;
  for (const char* name : {"wifi", "ntp_cfg"}) {
    Preferences settings;
    if (!settings.begin(name, false)) { success = false; continue; }
    success = settings.clear() && success;
    settings.end();
  }
  display.showHoldMessage(success ? "CONFIG APAGADA" : "FALHA NA LIMPEZA",
                          "Solte BOOT", success ? 0x07E0 : 0xF800);
  while (digitalRead(BOOT_BUTTON_PIN) == LOW) delay(10);
  delay(300);
  ESP.restart();
}

void setup() 
{
  Serial.begin(115200);
  delay(100); 
  Serial.println("[SERIAL] Comandos a 115200 baud. Use Nova linha ou CRLF; digite help.");
  pinMode(USER_BUTTON_PIN, INPUT_PULLUP);
  pinMode(PAGE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

  display.begin();
  pageButtonPending = false;
  attachInterrupt(digitalPinToInterrupt(PAGE_BUTTON_PIN), onPageButtonPressed, FALLING);
  display.setRotation(1); 
  display.clear();
  display.println("Sistema Inicializando...");
  delay(200); 

  commandProcessor.begin();
  delay(100);

  rgbLed.begin();
  gateway.begin();
  networkMonitor.begin();
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
  static bool pageButtonLatched = false;
  static uint32_t pageButtonReleasedAt = 0;
  String comando;

  static int bootState = HIGH;
  static int lastBootReading = HIGH;
  static uint32_t bootDebounceAt = 0;
  static uint32_t bootPressedAt = 0;
  const int bootReading = digitalRead(BOOT_BUTTON_PIN);
  if (bootReading != lastBootReading) bootDebounceAt = millis();
  if (millis() - bootDebounceAt >= 50 && bootReading != bootState) {
    bootState = bootReading;
    if (bootState == LOW) {
      bootPressedAt = millis();
      display.showHoldMessage("LIMPAR CONFIG?", "BOOT por 5 segundos", 0xFFE0);
    } else {
      display.clear();
      lastClockUpdate = 0;
    }
  }
  lastBootReading = bootReading;
  if (bootState == LOW) {
    if (millis() - bootPressedAt >= 5000) clearSavedSettings();
    delay(5);
    return; // Preserva a mensagem de confirmacao durante a pressao.
  }

  gateway.handleClient();
  networkMonitor.update();
  commandProcessor.update();  

  // Captura por interrupcao preserva toques durante redesenho ou consulta HTTP.
  if (pageButtonLatched) {
    pageButtonPending = false;
    if (digitalRead(PAGE_BUTTON_PIN) == HIGH) {
      if (pageButtonReleasedAt == 0) pageButtonReleasedAt = millis();
      if (millis() - pageButtonReleasedAt >= 50) pageButtonLatched = false;
    } else pageButtonReleasedAt = 0;
  } else if (pageButtonPending) {
    pageButtonPending = false;
    pageButtonLatched = true;
    pageButtonReleasedAt = 0;
    currentPage = (currentPage + 1) % PAGE_COUNT;
    display.setRotation(1);
    showingDashboard = true;
    lastClockUpdate = 0;
    statusPageShownAt = millis();
    tempoUltimoComando = millis();
    Serial.printf("[BOTAO] Pagina %u\n", currentPage);
  }

  // K2 retorna uma pagina ao soltar; sem apagar ou reiniciar por pressao longa.
  const int buttonReading = digitalRead(USER_BUTTON_PIN);
  if (buttonReading != lastButtonReading) lastDebounceTime = millis();
  if (millis() - lastDebounceTime >= 50 && buttonReading != buttonState) {
    buttonState = buttonReading;
    if (buttonState == HIGH) {
      currentPage = (currentPage + PAGE_COUNT - 1) % PAGE_COUNT;
      display.setRotation(1);
      showingDashboard = true;
      lastClockUpdate = 0;
      statusPageShownAt = millis();
      tempoUltimoComando = millis();
    }
  }
  lastButtonReading = buttonReading;

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

  // Serial e UDP usam o mesmo processador e acordam o display.
  auto processCommand = [&](const String& receivedCommand)
  {
    display.setRotation(1); // Console: 90 degrees clockwise from its previous orientation.
    showingDashboard = false;
    commandProcessor.executeCommand(receivedCommand);
    responseUntil = millis() + 10000;
    tempoUltimoComando = millis(); // Reseta Protetor de Tela
  };
  if (receiveSerialCommand(comando)) processCommand(comando);
  if (gateway.receiveCommand(comando)) processCommand(comando);

  commandProcessor.update();

  // Controle de estados da tela (Console Log -> Relógio/Dashboard -> Protetor de Tela)
  if (!showingDashboard && static_cast<int32_t>(millis() - responseUntil) >= 0)
  {
    showingDashboard = true;
    display.setRotation(1);
    display.clear(); 
    lastClockUpdate = 0;
  }

  if (showingDashboard)
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
      else if (currentPage == 5) display.showMonitorPage(networkMonitor);
      else if (currentPage == 6) display.showForecastPage();
      lastClockUpdate = millis();
    }
  }
}
