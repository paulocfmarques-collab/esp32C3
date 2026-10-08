#ifndef CLIMA_MANAGER_H
#define CLIMA_MANAGER_H

#include <WiFi.h>
#include <stdlib.h>
#include <math.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h> // Biblioteca nativa e leve para o ESP32-C3

class ClimaManager {
public:
    inline static float temperatura = 0.0;
    inline static int codigoCondicao = 0;
    inline static uint32_t ultimaAtualizacao = 0;
    inline static bool sincronizado = false;
    inline static bool previsaoValida = false;
    inline static float minima = 0, maxima = 0;
    inline static int chuva = 0;
    inline static String dataPrevisao = "";
    inline static uint32_t previsaoAtualizada = 0;
    static bool numeroDiario(const String& bloco, const char* chave, float& valor) {
        int pos = bloco.indexOf(String("\"") + chave + "\":");
        if (pos < 0) return false;
        int inicio = bloco.indexOf('[', pos), fim = bloco.indexOf(']', inicio);
        if (inicio < 0 || fim < inicio) return false;
        String token = bloco.substring(inicio + 1, fim); token.trim();
        char* end = nullptr; float parsed = strtof(token.c_str(), &end);
        if (end == token.c_str() || *end != '\0' || !isfinite(parsed)) return false;
        valor = parsed; return true;
    }

    static void atualizar() {
        uint32_t intervalo = sincronizado ? 900000UL : 60000UL;
        if (millis() - ultimaAtualizacao < intervalo && ultimaAtualizacao != 0) return;
        if (WiFi.status() != WL_CONNECTED) return;

        ultimaAtualizacao = millis();
        
        NetworkClientSecure client;
        client.setInsecure(); // Desativa a checagem de chaves RSA pesadas para salvar memória RAM

        HTTPClient http;
        
        // Ativa o redirecionamento automático de HTTP (301) para HTTPS (443) de forma transparente
        http.setConnectTimeout(3000);
        http.setTimeout(3000);
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        
        // URL da API configurada com os parâmetros corretos para Porto Alegre - RS
        String url = "https://api.open-meteo.com/v1/forecast?latitude=-30.03&longitude=-51.23&current=temperature_2m,weather_code&daily=temperature_2m_min,temperature_2m_max,precipitation_probability_max&forecast_days=1&timezone=America%2FSao_Paulo";
        
        http.begin(client, url);
        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            int dailyPos = payload.indexOf("\"daily\":");
            if (dailyPos >= 0) {
                String daily = payload.substring(dailyPos);
                float low, high, rain;
                int timePos = daily.indexOf("\"time\":");
                int dateStart = daily.indexOf('[', timePos);
                int quote = daily.indexOf('"', dateStart);
                if (timePos >= 0 && dateStart >= 0 && quote >= 0 &&
                    numeroDiario(daily, "temperature_2m_min", low) &&
                    numeroDiario(daily, "temperature_2m_max", high) &&
                    numeroDiario(daily, "precipitation_probability_max", rain) &&
                    low <= high && rain >= 0 && rain <= 100) {
                    minima = low; maxima = high; chuva = int(rain);
                    dataPrevisao = daily.substring(quote + 1, quote + 11);
                    previsaoAtualizada = millis(); previsaoValida = true;
                }
            }
            
            // Busca linear direta no texto recebido da API para evitar usar ArduinoJson
            int currentBlockPos = payload.indexOf("\"current\":");
            if (currentBlockPos >= 0) {
                String currentPayload = payload.substring(currentBlockPos);

                // Captura a temperatura real dentro do bloco numérico do "current"
                int tempPos = currentPayload.indexOf("\"temperature_2m\":");
                if (tempPos >= 0) {
                    int startPos = tempPos + 17;
                    int endPos = currentPayload.indexOf(",", startPos);
                    if (endPos == -1 || endPos > currentPayload.indexOf("}", startPos)) {
                        endPos = currentPayload.indexOf("}", startPos);
                    }
                    
                    if (endPos > startPos) {
                        temperatura = currentPayload.substring(startPos, endPos).toFloat();
                    }
                }

                // Captura o código de condição meteorológica correspondente do "current"
                int codePos = currentPayload.indexOf("\"weather_code\":");
                if (codePos >= 0) {
                    int startPos = codePos + 15;
                    int endPos = currentPayload.indexOf(",", startPos);
                    if (endPos == -1 || endPos > currentPayload.indexOf("}", startPos)) {
                        endPos = currentPayload.indexOf("}", startPos);
                    }

                    if (endPos > startPos) {
                        codigoCondicao = currentPayload.substring(startPos, endPos).toInt();
                    }
                }
                sincronizado = true;
                Serial.printf("[CLIMA] Sincronizado -> Temp: %.1f C | Cod WMO: %d\n", temperatura, codigoCondicao);
            }
        } 
        else {
            Serial.printf("[CLIMA ERRO] Falha no transporte. Codigo HTTP: %d\n", httpCode);
        }
        http.end();
    }

    static String obterTextoCondicao() {
        if (codigoCondicao == 0) return "Ceu Limpo";
        if (codigoCondicao >= 1 && codigoCondicao <= 3) return "Parc. Nublado";
        if (codigoCondicao >= 45 && codigoCondicao <= 48) return "Nevoeiro";
        if (codigoCondicao >= 51 && codigoCondicao <= 65) return "Chuva/Garoa";
        if (codigoCondicao >= 71 && codigoCondicao <= 77) return "Neve";
        if (codigoCondicao >= 80 && codigoCondicao <= 82) return "Pancadas Chuva";
        if (codigoCondicao >= 95 && codigoCondicao <= 99) return "Tempestade";
        return "Nublado";
    }
};

#endif
