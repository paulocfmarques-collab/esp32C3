# ESP32-C3 Gateway — Quick Start

See [README.md](README.md) for installation, hardware, commands, controls, and limitations, and [CHANGELOG.md](CHANGELOG.md) for the recorded changes.

1. Open `ESP32C3.ino` with all source files in the same folder.
2. Install the ESP32 3.x board package and GFX Library for Arduino.
3. Select ESP32C3 Dev Module and an OTA-compatible partition layout; upload over USB.
4. Join `ESP32_C3_CONFIG` and open `http://192.168.4.1` to save Wi-Fi credentials.
5. Send `help` to the device IP on UDP port 4210.

Five Wi-Fi profiles are stored persistently in a circular list. At startup, visible saved networks are tried before the AP fallback.

Key1/BOOT advance pages on press. Key2 advances on release; holding it for 3 seconds and releasing restarts, while 10 seconds and releasing clears profiles and restarts.

`desliga` enters deep sleep. Use RESET to restart. The backlight remains powered by the board circuit.

## Navegacao e limpeza de configuracoes

K1 avanca as telas; K2 retorna ao soltar. BOOT e reservado para limpar configuracoes.

Durante o funcionamento, segure BOOT por 5 segundos. Soltar antes cancela. Ao aparecer CONFIG APAGADA, solte BOOT: a placa reinicia e abre o AP. Limpa somente redes salvas e configuracoes do relogio, preservando firmware. EN continua sendo apenas reset. Nao mantenha BOOT pressionado ao ligar ou resetar, pois isso entra no modo de gravacao.

## Novas páginas: monitor de rede e previsão

K1 avança e K2 retorna entre sete páginas: relógio, estado, rede, sistema,
redes salvas, monitor de rede e previsão. As páginas auxiliares voltam ao
relógio após 30 segundos sem navegação.

O monitor envia um ping ICMP ao gateway a cada aproximadamente 5 segundos,
com espera máxima de 1 segundo em uma tarefa separada. Mostra resposta em ms,
percentual de pings sem resposta, número de quedas do Wi-Fi, tempo acumulado
sem conexão e os três eventos de conexão mais recentes (minutos desde o boot).
O gráfico contém os últimos 24 resultados: verde = resposta, vermelho = timeout.
Falha de ping não significa necessariamente queda do Wi-Fi nem mede a Internet;
o roteador pode bloquear ICMP. Estatísticas e histórico ficam na RAM e reiniciam
quando a placa reinicia. Em AP sem conexão STA, aparece “sem rede”.

A previsão de hoje usa Porto Alegre/RS, já configurada no projeto, e mostra data,
mínima, máxima e probabilidade máxima de precipitação do dia. Atualiza com o clima
a cada 15 minutos; sem dados iniciais tenta novamente após 1 minuto.
Dados válidos são mantidos em RAM nas falhas de atualização, com idade visível.
Dados ausentes ou nulos não viram zero. A consulta de clima continua HTTP síncrona,
com timeouts de conexão e leitura de 3 segundos; os pings são assíncronos.

Não foram acrescentados comandos UDP: a navegação usa K1/K2 e `clima_sync`
continua disponível para solicitar a atualização do clima.
