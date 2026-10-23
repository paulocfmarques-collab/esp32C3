# Changelog

## 2026-10-21 — Formato parseável do help

- Padronizada a resposta como `COMANDOS` seguido de um comando por linha.
- Removidas descrições e exemplos da lista; detalhes permanecem neste guia.

## Olhos ciberneticos do protetor

- Olhos abertos redesenhados para o TFT 128 x 128: iris azul-ciano, pupila
  escura, aneis concentricos, marcas radiais, circuitos e reflexos claros,
  com moldura metalica desenhada por primitivas, sem imagens externas.
- Mantidas as 40 expressoes, bocas, piscadas e movimentos; olhos especiais
  como coracoes e estrelas continuam estilizados. O olhar lateral desloca
  a iris e as pupilas variam com a expressao e com o tempo.
- Mantidos o inicio apos 15 minutos e o intervalo minimo de 80 ms entre
  quadros, sem delays adicionais ou novos buffers de imagem.
- Compilacao e teste na placa ficam a cargo do usuario.

## 2026-10-31 — Reset dos contadores NVS

- Adicionado `reset_counters` para zerar boot/watchdog no namespace `telemetry`
  sem apagar as configurações Prometheus.
- Atualizados o menu `help`, o guia de comandos e o catálogo consolidado.

Registro consolidado das alterações do firmware ESP32-C3.

## 2026-10-18 — Protetor e revisão documental

- Documentado `name`, que retorna `ESP32C3` para identificação da placa.
- Revisado o desenho do protetor com olhos e bocas sem carcaça, dez expressões
  de referência e expressões adicionais; início após 15 minutos sem interação.
  Alteração de fonte registrada em `5ee4231`.
- Guia de comandos atualizado para distinguir navegação por botões e handlers
  realmente disponíveis; não há comandos de brilho/páginas herdados do C6.
- Mantido `crash_info` por USB/UDP: apaga após leitura bem-sucedida, sem SD,
  conforme evolução registrada em `c9172af`.
- Referência comum: [histórico ESP32](../../ESP32_Documents/changelog.md).
  A revisão documental não confirma teste físico do firmware.

## [Unreleased]

### Added

- Prometheus em `/metrics`, na porta configurada e salva (padrao 9120),
  mantendo o painel HTTP na porta 80 em STA e AP.
- Configuracao web em `/prometheus`, endereco de referencia do servidor,
  intervalo sugerido e download `/prometheus.yml`; persistencia no namespace
  `telemetry`, com validacao e erros HTTP 400/503.
- Metricas reais de identificacao, disponibilidade, Wi-Fi, CPU, memoria,
  tarefas, flash, botoes, LED e HTTP; contadores persistentes de boot/watchdog.
- `esp_cpu_temperature_celsius` para a temperatura interna do chip e
  `esp_weather_data_age_seconds` para a idade do ultimo cache de clima valido,
  com timer monotono de 64 bits e sem resetar a idade em consultas com falha.
- Catalogo e roteiro de verificacao em `PROMETHEUS.md`, incluindo as series
  omitidas por ausencia de sensores, reles, ADC ou cliente MQTT.

- Renderização em buffer de 32 KB e páginas de relógio, estado, rede, sistema e
  redes salvas; rotas HTTP `/info` e `/wifi`.
- Navegação por interrupções dos botões Key1/BOOT e ações curtas/longas em Key2.
- Barras adaptativas de RSSI e traçado de um pixel com o histórico do sinal.
- Monitor de rede com ping ICMP assíncrono ao gateway, gráfico de 24 amostras,
  contagem de quedas de Wi-Fi e tempo acumulado offline.
- Página de previsão diária de Porto Alegre, com mínimas, máximas, chance de
  chuva, data e idade do cache.
- Cinco perfis persistentes de Wi-Fi, busca sequencial de redes e AP de
  configuração como fallback.
- Páginas de relógio, estado, rede, sistema e redes salvas, com navegação por
  botões; `desliga` coloca a placa em deep sleep.
- Ajuste manual do relógio pelo comando `set_time:AAAA-MM-DD HH:MM:SS`.

### Changed

- Firmware portado do ESP32-C6/ST7789 para ESP32-C3/ST7735, com tela 128 × 128,
  pinos específicos da placa e vistas redimensionadas.
- LED adaptado ao indicador monocromático integrado; comandos e referência
  padronizados conforme a ajuda do firmware.
- Barras Wi-Fi usam verde a partir de -67 dBm, amarelo de -80 a menos de
  -67 dBm, vermelho abaixo de -80 dBm e cinza sem conexão.
- Consulta meteorológica usa o endpoint atual e tenta novamente após um minuto
  enquanto ainda não houver dados sincronizados.
- Requisições de clima preservam dados válidos anteriores em caso de falha e
  usam timeouts de conexão e leitura de três segundos.
- A tela usa renderização em buffer; o console de comandos tem orientação
  ajustada para melhorar a leitura.
- Atualizada a referência geral de comandos para corresponder aos handlers do
  firmware.

### Fixed

- Verificação e migração de credenciais Wi-Fi confirmam os dados persistidos,
  inclusive senha vazia.
- Corrigida a validação de credenciais legadas, os argumentos de piscada finita
  e a desativação de DST.
- Corrigidos o despacho de comandos de rede e problemas de estado nos efeitos de
  LED e no DST.
- Separados o relatório de saúde e o despacho de comandos; alinhadas declarações
  e implementações após a remoção do SD.
- Corrigido o rótulo TFT malformado no diagrama Mermaid.

### Removed

- Armazenamento em SD e comandos dependentes dele.
- Comandos RGB e PSRAM dependentes da placa; `init`, `heap`, `heap_min`,
  `flash_info` e `selftest` foram removidos em favor de `reason`, `ram`, `flash`
  e `health`.

### Known limitations

- O backlight permanece alimentado durante deep sleep.
- Não há troca de SSID em tempo de execução nem descoberta de redes ocultas.
- AP, HTTP, UDP e OTA não têm autenticação configurada; a validação de
  certificado HTTPS do serviço de clima está desativada.
- O pacote atual não tem registro de compilação completa ou regressão em
  hardware.

## [2.0.0] - 2026-10-07

### Added

- README principal com visão geral do firmware, badges e instruções de
  compilação.
- Documentação GitHub, diagramas de arquitetura e instruções de setup no Arduino
  IDE.
- Versão premium do README e organização da documentação por funcionalidades,
  hardware, compilação e fluxo de operação.

### Changed

- Documentação alinhada aos módulos `ESP32Gateway`, `CommandProcessor`,
  `DisplayUtil`, `NTPUtil` e `RGBLed`, com descrições de pinagem e botões.

### Fixed

- Descrições de `desliga`, perfis Wi-Fi circulares, `set_time`, `health` e
  telemetria de RSSI alinhadas ao firmware.

## [1.0.0] - Initial documentation baseline

### Added

- README inicial, guia de uso em português, documentação de comandos e notas de
  hardware.
