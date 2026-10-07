# ESP32-C3 Gateway Premium

![Version](https://img.shields.io/badge/version-2.0.0-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--C3%20%7C%20Arduino-ff6f00.svg)
![Language](https://img.shields.io/badge/language-C%2B%2B-00599C.svg)
![GitHub](https://img.shields.io/badge/presentation-premium-181717.svg?logo=github)

> Uma versão premium, mais enxuta e impactante, do README para destaque no GitHub.

## Resumo

O **ESP32-C3 Gateway** é um firmware embarcado para ESP32-C3 com display TFT, portal Wi‑Fi, console UDP, NTP, clima, OTA e feedback por LED RGB.

## Principais pontos

- Interface visual com páginas de relógio, rede, sistema e status
- Portal de configuração Wi‑Fi com múltiplas redes
- Comandos UDP para operação remota
- Sincronização NTP com fuso e DST persistidos
- Clima atualizado via rede
- OTA quando a placa está conectada
- Botões físicos com navegação e ações longas
- Modo deep sleep por comando remoto

## Arquitetura visual

```mermaid
flowchart LR
    U[Usuário] --> W[Wi-Fi]
    U --> UDP[UDP Console]
    U --> BTN[Botões]
    W --> ESP[ESP32-C3]
    UDP --> CMD[CommandProcessor]
    BTN --> UI[DisplayUtil]
    ESP --> LED[RGBLed]
    ESP --> NTP[NTPUtil]
    ESP --> OTA[OTAManager]
    ESP --> WEATHER[ClimaManager]
```

## O que este firmware entrega

- Experiência visual pronta para tela pequena
- Operação remota por rede local
- Diagnóstico rápido do sistema
- Gestão de Wi‑Fi em slots persistentes
- Interface elegante para demonstração em GitHub

## Destaque GitHub

Este README premium foi pensado para:

- visitantes do repositório
- documentação de portfólio
- apresentação de projeto embarcado
- leitura rápida com apelo visual

## Sugestão de uso

Se quiser, esta versão pode ser usada como:

- `README.md` principal
- `README.premium.md` como vitrine
- base para página de apresentação no GitHub
