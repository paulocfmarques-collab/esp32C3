#ifndef CLIMA_MANAGER_H
#define CLIMA_MANAGER_H

#include <WiFi.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
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
    inline static uint32_t ultimaTentativa = 0;
    inline static bool tentouAtualizar = false;
    inline static String ultimoErro = "";

    static bool falhar(const String& motivo) {
        ultimoErro = motivo;
        Serial.println("[CLIMA ERRO] " + motivo);
        return false;
    }

    // Extrai somente o objeto solicitado, sem confundir dados com current_units.
    static bool objeto(const String& json, const char* chave, String& bloco) {
        int pos = json.indexOf(String("\"") + chave + "\"");
        if (pos < 0) return false;
        int colon = json.indexOf(':', pos);
        if (colon < 0) return false;
        int start = colon + 1;
        while (start < (int)json.length() && isspace((unsigned char)json[start])) ++start;
        if (start >= (int)json.length() || json[start] != '{') return false;
        int end = json.indexOf('}', start);
        if (end < 0) return false;
        bloco = json.substring(start + 1, end);
        return true;
    }

    static bool numero(const String& bloco, const char* chave, float& valor, bool array = false) {
        int pos = bloco.indexOf(String("\"") + chave + "\"");
        if (pos < 0) return false;
        int colon = bloco.indexOf(':', pos);
        if (colon < 0) return false;
        int start = colon + 1;
        while (start < (int)bloco.length() && isspace((unsigned char)bloco[start])) ++start;
        if (array) {
            if (start >= (int)bloco.length() || bloco[start] != '[') return false;
            ++start;
        }
        String token = bloco.substring(start);
        token.trim();
        if (token.length() == 0 || !(isdigit((unsigned char)token[0]) || token[0] == '-')) return false;
        char* end = nullptr;
        float parsed = strtof(token.c_str(), &end);
        if (end == token.c_str() || !isfinite(parsed)) return false;
        while (isspace((unsigned char)*end)) ++end;
        if (array ? (*end != ']') : (*end != ',' && *end != '\0')) return false;
        valor = parsed;
        return true;
    }

    static bool numeroDiario(const String& bloco, const char* chave, float& valor) {
        return numero(bloco, chave, valor, true);
    }

    static bool interpretar(const String& payload) {
        String current, daily;
        float temp, code, low, high, rain;
        if (!objeto(payload, "current", current) ||
            !numero(current, "temperature_2m", temp) ||
            !numero(current, "weather_code", code) ||
            code < 0 || code > 99 || floorf(code) != code) {
            return falhar("Resposta sem dados atuais validos (temperature_2m/weather_code).");
        }
        if (!objeto(payload, "daily", daily) ||
            !numeroDiario(daily, "temperature_2m_min", low) ||
            !numeroDiario(daily, "temperature_2m_max", high) ||
            !numeroDiario(daily, "precipitation_probability_max", rain) ||
            low > high || rain < 0 || rain > 100) {
            return falhar("Resposta sem previsao diaria valida (minima/maxima/chuva).");
        }
        int timePos = daily.indexOf("\"time\"");
        int colon = timePos < 0 ? -1 : daily.indexOf(':', timePos);
        int start = colon < 0 ? -1 : daily.indexOf('[', colon);
        int quote = start < 0 ? -1 : daily.indexOf('"', start);
        if (quote < 0 || quote + 11 >= (int)daily.length() || daily[quote + 11] != '"')
            return falhar("Previsao sem data valida.");
        String date = daily.substring(quote + 1, quote + 11);
        for (int i = 0; i < 10; ++i) {
            if (i == 4 || i == 7) { if (date[i] != '-') return falhar("Data da previsao invalida."); }
            else if (!isdigit((unsigned char)date[i])) return falhar("Data da previsao invalida.");
        }
        // Publica tudo junto somente depois de validar a resposta completa.
        temperatura = temp; codigoCondicao = (int)code;
        minima = low; maxima = high; chuva = (int)rain; dataPrevisao = date;
        ultimaAtualizacao = millis(); previsaoAtualizada = ultimaAtualizacao;
        sincronizado = true; previsaoValida = true; ultimoErro = "";
        return true;
    }

    static bool atualizar(bool forcar = false) {
        const uint32_t now = millis();
        const uint32_t intervalo = ultimoErro.length() == 0 && sincronizado ? 900000UL : 60000UL;
        if (!forcar && tentouAtualizar && now - ultimaTentativa < intervalo) return false;
        tentouAtualizar = true; ultimaTentativa = now;
        if (WiFi.status() != WL_CONNECTED) return falhar("Wi-Fi desconectado.");

        NetworkClientSecure client;
        client.setInsecure(); // Mantem o comportamento TLS existente.
        client.setHandshakeTimeout(10);
        HTTPClient http;
        http.setConnectTimeout(10000);
        http.setTimeout(10000);
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        // Porto Alegre - RS, conforme a localidade exibida no painel.
        const String url = "https://api.open-meteo.com/v1/forecast"
            "?latitude=-30.0346&longitude=-51.2177"
            "&current=temperature_2m,weather_code"
            "&daily=temperature_2m_min,temperature_2m_max,precipitation_probability_max"
            "&timezone=America%2FSao_Paulo&forecast_days=1";
        Serial.println("[CLIMA] Consultando API Open-Meteo (Porto Alegre)...");
        if (!http.begin(client, url)) {
            http.end();
            return falhar("Nao foi possivel iniciar a consulta HTTPS.");
        }
        const int code = http.GET();
        Serial.printf("[CLIMA] HTTP: %d\n", code);
        if (code != HTTP_CODE_OK) {
            String motivo = "HTTP " + String(code);
            if (code < 0) motivo += " (" + HTTPClient::errorToString(code) + ")";
            else {
                String body = http.getString();
                if (body.length() > 0) motivo += ": " + body.substring(0, 200);
            }
            http.end();
            return falhar(motivo);
        }
        String payload = http.getString();
        http.end();
        Serial.printf("[CLIMA] Resposta: %u bytes\n", (unsigned)payload.length());
        if (!interpretar(payload)) return false;
        Serial.printf("[CLIMA] Sincronizado -> Temp: %.1f C | Cod WMO: %d\n", temperatura, codigoCondicao);
        return true;
    }

    static String obterTextoCondicao() {
        if (!sincronizado) return "Sem dados";
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
