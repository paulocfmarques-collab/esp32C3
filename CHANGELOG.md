# Changelog

Todos os dados importantes deste projeto devem ser registrados aqui.

## [2.0.0] - 2026-10-07

### Added

- README principal reescrito com foco em GitHub e documentação de visitação rápida.
- Criada a versão premium `README.premium.md` com apresentação mais visual.
- Criado `CHANGELOG.md` estruturado e preparado para manutenção contínua.
- Adicionados badges de status, linguagem, plataforma, licença e apresentação.
- Adicionados diagramas Mermaid para arquitetura, fluxo e estrutura do projeto.
- Organização da documentação por tópicos:
  - visão geral
  - funcionalidades
  - comandos
  - hardware
  - compilação
  - fluxo de operação

### Changed

- O README passou a refletir com mais precisão os módulos detectados no código:
  - `ESP32Gateway`
  - `CommandProcessor`
  - `DisplayUtil`
  - `NTPUtil`
  - `RGBLed`
- A documentação deixou de depender de descrições genéricas e passou a espelhar recursos concretos do firmware.
- A seção de comandos foi alinhada ao conjunto real exposto por `CommandProcessor.cpp`.
- A pinagem e os comportamentos de botões foram descritos com base no código atual.
- A estrutura de documentação foi separada entre README principal e versão premium.

### Fixed

- Corrigida a documentação para destacar a existência de páginas de status, rede, sistema e redes salvas.
- Corrigida a narrativa do projeto para incluir:
  - `desliga` com deep sleep
  - lista circular de 5 redes Wi‑Fi
  - comando `set_time`
  - `health`
  - telemetria de RSSI

### Documentation

- Incluído um mapa visual do projeto com Mermaid.
- Incluído um diagrama de classe simplificado dos módulos principais.
- Incluído um diagrama de sequência para ilustrar a operação do fluxo usuário → rede → firmware.
- Adicionada orientação de setup no Arduino IDE.

## [1.0.0] - Initial documentation baseline

### Added

- README inicial do projeto.
- LEIA-ME em português com instruções básicas.
- Documentação de comandos e notas de hardware.

---

## Convenção para próximas versões

Use os seguintes tipos de entrada:

- `Added` para recursos novos
- `Changed` para comportamento alterado
- `Fixed` para correções
- `Removed` para itens removidos
- `Security` para melhorias de segurança
- `Documentation` para mudanças de documentação

## Modelo recomendado para futuras entradas

```md
## [X.Y.Z] - YYYY-MM-DD

### Added
- ...

### Changed
- ...

### Fixed
- ...

### Removed
- ...

### Security
- ...

### Documentation
- ...
```
