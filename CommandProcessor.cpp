#include "CommandProcessor.h"
#include "DisplayUtil.h"
#include "ESP32Gateway.h"
#include "NTPUtil.h"
#include "RGBLed.h"
#include "ClimaManager.h"

#include <WiFi.h>
#include <esp_system.h>
#include <esp_sleep.h>

#include <Preferences.h>

CommandProcessor::CommandProcessor(DisplayUtil& display, ESP32Gateway& gateway,
                                   NTPUtil& ntp, RGBLed& led)
    : display_(display), gateway_(gateway), ntp_(ntp), led_(led)
{
}

bool CommandProcessor::begin() { return true; }

void CommandProcessor::answerAll(String message)
{
  Serial.print(message);

  String displayMessage = message;
  displayMessage.replace("\r", "");
  displayMessage.replace("\n", "");
  if (displayMessage.length() > 0)
  {
    display_.println(displayMessage);
  }

  gateway_.sendMessage(message);

}

void CommandProcessor::startBlink(uint16_t pulses, uint32_t interval, bool continuous)
{
  blinkActive_ = true;
  blinkOn_ = true;
  blinkContinuous_ = continuous;
  pulsesRemaining_ = pulses;
  blinkInterval_ = interval;
  lastBlinkChange_ = millis();
  led_.white();
}

void CommandProcessor::executeCommand(String command)
{
  command.trim();
  if (command.length() == 0)
  {
    return;
  }

  String commandLog = "> " + command + "\n";
  String message = "";
  Serial.print(commandLog);
  display_.println("> " + command);

  
  if (command == "help")
  {
    message = "COMANDOS ESP32-C3\n"
              "[SISTEMA]\n"
              "help\n"
              "info\n"
              "status\n"
              "uptime\n"
              "reason\n"
              "version\n"
              "build\n"
              "alive\n"
              "reboot\n"
              "desliga\n"
              "temp\n"
              "cpu\n"
              "ram\n"
              "flash\n"
              "chip_info\n"
              "health\n"
              "[REDE]\n"
              "net_info\n"
              "mac\n"
              "reset_wifi\n"
              "rssi\n"
              "ip\n"
              "ssid\n"
              "channel\n"
              "wifi_status\n"
              "[HORARIO]\n"
              "time\n"
              "date\n"
              "ntp_status\n"
              "set_fuso:X\n"
              "set_time:AAAA-MM-DD HH:MM:SS\n"
              "dst_on\n"
              "dst_off\n"
              "[LED]\n"
              "led_on\n"
              "led_off\n"
              "led_breath\n"
              "led_pisca:P:I\n"
              "led_blink:I\n"
              "[CLIMA]\n"
              "clima\n"
              "clima_age\n"
              "clima_sync\n"
              "P = pulsos; I = intervalo em ms\n"
              "X = fuso UTC (ex.: -3)\n";
    answerAll(message);
  }
  else if (command == "desliga")
  {
    answerAll("Hibernando. Para ligar, pressione RESET.\n");
    delay(300); // Permite enviar a resposta UDP antes de desligar o radio.
    blinkActive_ = false;
    breathActive_ = false;
    led_.off();
    display_.clear();
    display_.getDisplay()->displayOff();
    WiFi.disconnect(false, false); // Preserva as credenciais.
    WiFi.mode(WIFI_OFF);
    detachInterrupt(digitalPinToInterrupt(8));
    detachInterrupt(digitalPinToInterrupt(9));
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    Serial.flush();
    esp_deep_sleep_start(); // Retorno apenas pelo RESET ou ciclo de alimentacao.
  }
  else if (command == "reboot")
  {
    answerAll("Reiniciando o sistema remotamente via UDP...\n");
    delay(500);
    ESP.restart();
  }
  else if (command == "clima_sync")
  {
    answerAll("Forcando atualizacao manual do clima...\n");
    // Zera o timer interno para burlar a trava de 15 minutos
    ClimaManager::ultimaAtualizacao = 0; 
    ClimaManager::atualizar();
    
    message = "Clima Atualizado ->\nTemp: " + String(ClimaManager::temperatura, 1) + 
              " C\nCondicao: " + ClimaManager::obterTextoCondicao() + "\n";
    answerAll(message);
  }
  
  
  else if (command == "info")
  {
    String dataHoraCompleta; ntp_.getDateTime(dataHoraCompleta, 100);
    String dataStr = "Sem Sinc.", horaStr = "--:--:--";
    if (dataHoraCompleta.length() >= 19) {
        dataStr = dataHoraCompleta.substring(0, 10);
        horaStr = dataHoraCompleta.substring(11, 19);
    }
    uint32_t heapLivre = ESP.getFreeHeap() / 1024;
    float flashLivre = (float)ESP.getFreeSketchSpace() / (1024.0 * 1024.0);

    message = "===== DEVICE INFO =====\n"
              "Hostname: ESP32C3\n"
              "Firmware: " + obterVersaoAutomatica() + "\n"
              "Build: " + String(__DATE__) + " " + String(__TIME__) + "\n" +
              "SSID: " + WiFi.SSID() + "\n" +
              "IP: " + WiFi.localIP().toString() + "\n" +
              "MAC: " + WiFi.macAddress() + "\n" +
              "RSSI: " + String(WiFi.RSSI()) + " dBm\n" +
              "Heap Livre: " + String(heapLivre) + " KB\n" +
              "Flash Livre: " + String(flashLivre, 1) + " MB\n" +
              "Data: " + dataStr + "\n" +
              "Hora: " + horaStr + "\n" +
              "Uptime: " + String(millis()) + " ms\n" +
              "Reset: " + obterMotivoReset() + "\n" + 
              "=======================\n";
    answerAll(message);              
  }
  else if (command == "reason")
  {
      message = "===== ULTIMO RESET =====\n"
                "Motivo: " + obterMotivoReset() + "\n"
                "Uptime Atual: " + String(millis() / 1000) + " s\n"
                "========================\n";
      answerAll(message);
  }
  else if (command == "reset_wifi")
  {
    answerAll("Limpando configuracao Wi-Fi...\n");
    gateway_.clearConfig();
  }
  else if (command == "version")
  {
      message = "Versao Firmware: " + obterVersaoAutomatica() + "\n";
      answerAll(message);
  }
  else if (command == "build")
  {
      message = "Build:\nData: " + String(__DATE__) + "\nHora: " + String(__TIME__) + "\n";
      answerAll(message);
  }
  else if (command == "status") {
        String statusWifi = (WiFi.status() == WL_CONNECTED) ? "OK" : "FALHA";
        uint32_t heapKB = ESP.getFreeHeap() / 1024;
        message = "ONLINE (MESTRE)\nWiFi: " + statusWifi + 
                  "\nNTP: " + (ntp_.isSincronizado() ? "OK" : "FALHA") + 
                  "\nHeap: " + String(heapKB) + " KB\n";
        answerAll(message);
  }
  else if (command == "led_on")
  {
    blinkActive_ = false;
    breathActive_ = false; // Desativa o modo de respiração se outro comando de LED for recebido
    led_.white();
    answerAll("LED ligado\n");
  }
  else if (command == "led_off")
  {
    blinkActive_ = false;
    breathActive_ = false; // Desativa o modo de respiração se outro comando de LED for recebido
    led_.off();
    answerAll("LED desligado\n");
  }
  else if (command.startsWith("led_pisca"))
  {
    uint16_t pulses = 10;
    uint32_t interval = 250;
    int firstColon = command.indexOf(':');
    int secondColon = command.indexOf(':', firstColon + 1);

    if (firstColon >= 0 && secondColon > firstColon)
    {
      int requestedPulses = command.substring(firstColon + 1, secondColon).toInt();
      long requestedInterval = command.substring(secondColon + 1).toInt();
      if (requestedPulses < 1 || requestedPulses > 100 ||
          requestedInterval < 1 || requestedInterval > 5000)
      {
        answerAll("Parametros LED_PISCA invalidos\n");
        return;
      }
      pulses = static_cast<uint16_t>(requestedPulses);
      interval = static_cast<uint32_t>(requestedInterval);
    }

    startBlink(pulses, interval, false);
    answerAll("LED piscando " + String(pulses) + " vezes\n");
  }
  else if (command.startsWith("led_blink"))
  {
    uint32_t interval = 250;
    int separator = command.indexOf(':');
    if (separator >= 0)
    {
      long requestedInterval = command.substring(separator + 1).toInt();
      if (requestedInterval < 50 || requestedInterval > 60000)
      {
        answerAll("Intervalo LED_BLINK invalido (50 a 60000 ms)\n");
        return;
      }
      interval = static_cast<uint32_t>(requestedInterval);
    }

    startBlink(0, interval, true);
    answerAll("Blink iniciado (" + String(interval) + " ms)\n");
  }
  else if (command == "dst_on")
  {
    Preferences prefs;
    prefs.begin("ntp_cfg", true);
    int currentFuso = prefs.getInt("fuso", -3);
    prefs.end();

    ntp_.atualizarConfiguracao(currentFuso, true);
    answerAll("Horário de Verão ativado (DST ON)\n");
  }
  else if (command == "dst_off")
  {
    Preferences prefs;
    prefs.begin("ntp_cfg", true);
    int currentFuso = prefs.getInt("fuso", -3);
    prefs.end();

    ntp_.atualizarConfiguracao(currentFuso, false);
    answerAll("Horário de Verão desativado (DST OFF)\n");
  }
  else if (command.startsWith("set_fuso:"))
  {
    int separator = command.indexOf(':');
    if (separator >= 0)
    {
      int novoFuso = command.substring(separator + 1).toInt();
      if (novoFuso < -12 || novoFuso > 14)
      {
        answerAll("Erro: Fuso inválido. Escolha de -12 a +14.\n");
        return;
      }
      
      Preferences prefs;
      prefs.begin("ntp_cfg", true);
      bool currentDst = prefs.getBool("dst");
      prefs.end();

      ntp_.atualizarConfiguracao(novoFuso, currentDst);
      answerAll("Novo fuso configurado: " + String(novoFuso) + "\n");
    }
  }
  else if (command.startsWith("set_time:"))
  {
    String v = command.substring(9);
    v.trim();
    int ano, mes, dia, hora, min, seg;
    if (v.length() != 19 ||
        sscanf(v.c_str(), "%d-%d-%d %d:%d:%d", &ano, &mes, &dia, &hora, &min, &seg) != 6 ||
        !ntp_.ajustarDataHora(ano, mes, dia, hora, min, seg))
    {
      answerAll("Erro: use set_time:AAAA-MM-DD HH:MM:SS\n");
    }
    else
    {
      answerAll("Data/hora ajustada: " + v + "\n");
    }
  }  else if (command == "temp") {
    message = "CPU Temp: " + String(temperatureRead()) + " C";
    answerAll(message);
  }
  else if (command == "cpu")
  {
    message = "CPU:\nModelo: " + String(ESP.getChipModel()) +
              "\nRevisao: " + String(ESP.getChipRevision()) +
              "\nNucleos: " + String(ESP.getChipCores()) +
              "\nCPU: " + String(ESP.getCpuFreqMHz()) + " MHz\n";
    answerAll(message);
  }
  else if (command == "ram")
  {
    message = "RAM:\nHeap Total: " + String(ESP.getHeapSize() / 1024.0) + " KB\n" +
              "Heap Livre: " + String(ESP.getFreeHeap() / 1024.0) + " KB\n" +
              "Menor Bloco Livre: " + String(ESP.getMinFreeHeap() / 1024.0) + " KB\n" +
              "Maior Bloco Livre: " + String(ESP.getMaxAllocHeap() / 1024.0) + " KB\n" +
              "RAM Utilizada: " + String(100.0 * (ESP.getHeapSize() - ESP.getFreeHeap()) / ESP.getHeapSize()) + " %\n";
    answerAll(message);
  }
  else if (command == "flash")
  {
    message = "FLASH:\nTamanho: " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB\n" +
              "Velocidade: " + String(ESP.getFlashChipSpeed() / 1000 / 1000) + " MHz\n" + 
              "Flash Mode: " + String(ESP.getFlashChipMode()) + "\n" +
              "Sketch Size: " + String(ESP.getSketchSize() / 1024 / 1024) + " MB\n" +
              "Free Sketch Space: " + String(ESP.getFreeSketchSpace() / 1024 / 1024) + " MB\n" +
              "Flash livre: " + String(100.0 * (ESP.getFlashChipSize() - ESP.getFreeSketchSpace()) / ESP.getFlashChipSize()) + " %\n";
    answerAll(message);
  }
  else if (command == "uptime")
  {
    uint64_t totalSegundos = millis() / 1000ULL;

    uint32_t dias = totalSegundos / 86400ULL;
    totalSegundos %= 86400ULL;

    uint8_t horas = totalSegundos / 3600ULL;
    totalSegundos %= 3600ULL;

    uint8_t minutos = totalSegundos / 60ULL;
    uint8_t segundos = totalSegundos % 60ULL;

    char buffer[100];

    snprintf(buffer, sizeof(buffer),
             "===== UPTIME =====\n"
             "%lu dias\n"
             "%02u:%02u:%02u\n"
             "==================\n",
             (unsigned long)dias,
             horas,
             minutos,
             segundos);

    answerAll(String(buffer));
  }
  else if (command == "ntp_status")
  {
      bool sincronizado = ntp_.isSincronizado();

      message = "===== NTP STATUS =====\n"
                "Estado: ";

      message += sincronizado ? "SINCRONIZADO\n" : "SEM SINCRONISMO\n";

      if (sincronizado)
      {
          String dataHora;
          ntp_.getDateTime(dataHora, 100);

          message += "Data/Hora: " + dataHora + "\n";
      }

      message += "======================\n";

      answerAll(message);
  }
  else if (command == "mac")
  {
    answerAll("MAC: " + WiFi.macAddress() + "\n");
  }
  else if (command == "net_info")
  {
    message = "NET:\nSSID: " + WiFi.SSID() + "\n" +
              "IP: " + WiFi.localIP().toString() + "\n" +
              "Gateway: " + WiFi.gatewayIP().toString() + "\n" +
              "Subnet: " + WiFi.subnetMask().toString() + "\n" +
              "DNS1: " + WiFi.dnsIP(0).toString() + "\n" +
              "DNS2: " + WiFi.dnsIP(1).toString() + "\n" +
              "RSSI: " + String(WiFi.RSSI()) + " dBm\n";
    answerAll(message);
  }
  else if (command == "time") {
      answerAll("Hora:\n" + ntp_.getSomenteHora() + "\n");
  }
  else if (command == "date") {
      String dataHoraCompleta; ntp_.getDateTime(dataHoraCompleta, 100);
      String data = (dataHoraCompleta.length() >= 10) ? dataHoraCompleta.substring(0, 10) : "Erro NTP";
      answerAll("Data:\n" + data + "\n");
  }
  
  
  
  
  
  
  
  
  else if (command == "led_breath")
  {
    blinkActive_ = false;
    breathActive_ = true; // Ativa o modo de respiração do LED
    answerAll("Modo do LED alterado para Pulsar (Breathing)\n");

  }
  
  else if (command == "alive") {
    answerAll("ip: " + WiFi.localIP().toString() + " - yes\n");
  }
  else if (command == "chip_info")
  {
      message = "===== CHIP INFO =====\n"
                "Modelo: " + String(ESP.getChipModel()) + "\n"
                "Revisao: " + String(ESP.getChipRevision()) + "\n"
                "Cores: " + String(ESP.getChipCores()) + "\n"
                "CPU: " + String(getCpuFrequencyMhz()) + " MHz\n"
                "SDK: " + String(ESP.getSdkVersion()) + "\n"
                "=====================\n";

      answerAll(message);
  }  
  else if (command == "rssi" || command == "ip" || command == "ssid" ||
           command == "channel" || command == "wifi_status")
  {
      if (WiFi.status() != WL_CONNECTED) message = "WiFi desconectado.\n";
      else if (command == "rssi") message = "RSSI: " + String(WiFi.RSSI()) + " dBm\n";
      else if (command == "ip") message = "IP: " + WiFi.localIP().toString() + "\n";
      else if (command == "ssid") message = "SSID: " + WiFi.SSID() + "\n";
      else if (command == "channel") message = "Canal: " + String(WiFi.channel()) + "\n";
      else message = "WiFi conectado\nSSID: " + WiFi.SSID() + "\nIP: " +
                     WiFi.localIP().toString() + "\nRSSI: " + String(WiFi.RSSI()) + " dBm\n";
      answerAll(message);
  }
  else if (command == "clima")
  {
      message = "===== CLIMA =====\n"
                "Temperatura: " +
                String(ClimaManager::temperatura, 1) +
                " C\n"
                "Condicao: " +
                ClimaManager::obterTextoCondicao() +
                "\n"
                "Codigo WMO: " +
                String(ClimaManager::codigoCondicao) +
                "\n"
                "=================\n";

      answerAll(message);
  }
  else if (command == "clima_age")
   {
    if (!ClimaManager::sincronizado)
    {
        answerAll("Clima ainda nao foi atualizado.\n");
    }
    else
    {
        uint32_t segundos =
            (millis() - ClimaManager::ultimaAtualizacao) / 1000UL;

        uint32_t minutos = segundos / 60;
        segundos %= 60;

        message = "===== CLIMA AGE =====\n"
                  "Ultima atualizacao:\n" +
                  String(minutos) + " min " +
                  String(segundos) + " s atras\n"
                  "=====================\n";

        answerAll(message);
    }
  }
  else if (command == "health")
  {
      executarHealth();
  }
  else
  {
      answerAll("Comando invalido\n");
  }
}

String CommandProcessor::obterMotivoReset() {
    esp_reset_reason_t reason = esp_reset_reason();
    switch (reason) {
        case ESP_RST_UNKNOWN:   return "DESCONHECIDO";
        case ESP_RST_POWERON:   return "POWER-ON (Tomada/VCC)";
        case ESP_RST_EXT:       return "PINO RESET (Botao EN)";
        case ESP_RST_SW:        return "SOFTWARE / OUTROS";
        case ESP_RST_PANIC:     return "CRASH / PANIC (Exception)";
        case ESP_RST_INT_WDT:   return "WATCHDOG INTERNO (Core Travado)";
        case ESP_RST_TASK_WDT:  return "TASK WATCHDOG (Loop Travado)";
        case ESP_RST_WDT:       return "OUTROS WATCHDOGS";
        case ESP_RST_DEEPSLEEP: return "ACORDOU DO DEEP SLEEP";
        case ESP_RST_BROWNOUT:  return "BROWNOUT (Queda de Tensao)";
        case ESP_RST_SDIO:      return "RESET VIA SDIO";
        default:                return "CODIGO NAO MAPEADO";
    }
}

void CommandProcessor::update()
{
  if (breathActive_) 
  {
    led_.breathingTask(0, 255, 255); // Roda a tarefa não bloqueante do LED
  }
  
  // 2. Manutencao do Blink do LED (Mantenha o seu codigo original abaixo)
  if (!blinkActive_ || millis() - lastBlinkChange_ < blinkInterval_)
  {
    return;
  }

  lastBlinkChange_ = millis();
  blinkOn_ = !blinkOn_;
  if (blinkOn_)
  {
    led_.white();
  }
  else
  {
    led_.off();
    if (!blinkContinuous_ && pulsesRemaining_ > 0 && --pulsesRemaining_ == 0)
    {
      blinkActive_ = false;
    }
  }
}

void CommandProcessor::executarHealth()
{
    int totalTestes = 0;
    int testesOK = 0;

    String resultado;

    resultado = "\n===== SYSTEM HEALTH =====\n\n";


    // =========================================================
    // WI-FI
    // =========================================================

    totalTestes++;

    bool wifiOK = (WiFi.status() == WL_CONNECTED);

    if (wifiOK)
    {
        testesOK++;

        int32_t rssi = WiFi.RSSI();

        resultado += "WiFi....... OK   ";
        resultado += String(rssi);
        resultado += " dBm\n";
    }
    else
    {
        resultado += "WiFi....... FAIL\n";
    }


    // =========================================================
    // NTP
    // =========================================================

    totalTestes++;

    bool ntpOK = ntp_.isSincronizado();

    if (ntpOK)
    {
        testesOK++;
        resultado += "NTP........ OK\n";
    }
    else
    {
        resultado += "NTP........ FAIL\n";
    }


    // =========================================================
    // MEMORIA RAM
    // =========================================================

    totalTestes++;

    uint32_t heapLivre = ESP.getFreeHeap();
    uint32_t heapMinimo = ESP.getMinFreeHeap();

    bool ramOK = heapLivre >= (40 * 1024);

    if (ramOK)
    {
        testesOK++;
        resultado += "RAM........ OK   ";
    }
    else
    {
        resultado += "RAM........ LOW  ";
    }

    resultado += String(heapLivre / 1024);
    resultado += " KB\n";


    // =========================================================
    // CLIMA
    // =========================================================

    totalTestes++;

    bool climaOK = false;

    if (ClimaManager::sincronizado)
    {
        uint32_t idadeClima =
            millis() - ClimaManager::ultimaAtualizacao;

        // Consideramos os dados validos por ate 30 minutos.
        climaOK = idadeClima <= 1800000UL;
    }

    if (climaOK)
    {
        testesOK++;

        resultado += "Clima...... OK   ";
        resultado += String(ClimaManager::temperatura, 1);
        resultado += " C\n";
    }
    else
    {
        resultado += "Clima...... STALE\n";
    }

    // =========================================================
    // CALCULO DO HEALTH
    // =========================================================

    int health = 0;

    if (totalTestes > 0)
    {
        health = (testesOK * 100) / totalTestes;
    }

    // =========================================================
    // STATUS GERAL
    // =========================================================

    String statusGeral;

    if (health == 100)
    {
        statusGeral = "EXCELENTE";
    }
    else if (health >= 80)
    {
        statusGeral = "BOM";
    }
    else if (health >= 60)
    {
        statusGeral = "ATENCAO";
    }
    else
    {
        statusGeral = "CRITICO";
    }

    // =========================================================
    // RESULTADO
    // =========================================================

    resultado += "\n-------------------------\n";

    resultado += "Health..... ";
    resultado += String(health);
    resultado += "%\n";

    resultado += "Status..... ";
    resultado += statusGeral;
    resultado += "\n";

    resultado += "Heap min... ";
    resultado += String(heapMinimo / 1024);
    resultado += " KB\n";

    resultado += "\n=========================\n";

    answerAll(resultado);
}

