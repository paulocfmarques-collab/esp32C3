# Gateway ESP32-C3 com LCD 1.44

Abra `ESP32C3.ino` mantendo todos os arquivos desta pasta juntos. Os arquivos originais do C6 foram preservados.

## Arduino IDE

- Instale o pacote **esp32 by Espressif Systems**, versão 3.x (o código usa NetworkClientSecure).
- Selecione **ESP32C3 Dev Module** e a porta USB correspondente.
- Ative **USB CDC On Boot** para o monitor serial pela USB nativa. Monitor: 115200 baud.
- Selecione tamanho de flash conforme a placa; o esquema identifica W25Q128 (16 MB), mas confirme a revisão física.
- Use um esquema de partições com suporte a OTA. Não use uma opção que desabilite OTA.
- Bibliotecas necessárias: **GFX Library for Arduino (Arduino_GFX)**. As outras bibliotecas vêm com o pacote ESP32.
- Não é necessário Adafruit_NeoPixel nesta versão.

## Pinos conferidos no esquema fornecido

| Sinal | GPIO |
|---|---:|
| LCD CS | 2 |
| LCD DC | 0 |
| LCD RESET | 5 |
| LCD MOSI | 4 |
| LCD SCK | 3 |
| LED simples | 11 |

O backlight está ligado diretamente à alimentação; não há pino de controle no esquema. Os botões GPIO8 e GPIO10 e a USB GPIO18/19 ficam livres de alterações. O LCD usa SPI por software nos pinos do esquema.

O painel foi configurado como ST7735 de 128 × 128, com offsets 2/3 usuais para 1.44. O esquema identifica o controlador, mas não informa os offsets da área visível. Se houver deslocamento ou cores invertidas, ajuste `TFT_OFFSET_X`, `TFT_OFFSET_Y` e `TFT_IPS` em `BoardConfig.h` após teste físico.

## Funções

WiFi, portal de configuração, comandos UDP, NTP, clima e OTA foram mantidos. O relógio, console e progresso OTA foram ajustados para a tela menor. O nome do ponto de acesso passa a ser `ESP32_C3_CONFIG`; conecte e abra `http://192.168.4.1`. OTA: `ESP32-C3-Gateway`.

Os comandos do LED disponíveis são `led_on`, `led_off`, `led_breath`, `led_pisca:P:I` e `led_blink:I`. P indica o número de pulsos e I o intervalo em milissegundos. A classe mantém internamente o nome RGBLed, mas opera o LED simples da placa.

Removidos comandos e suporte a SD/logs, `net_scan` que dependia de gravação em SD, `psram`, `led_cyan` e `led_color`. Removida a dependência SdFat. A ajuda mostra os comandos mantidos. Foram eliminados ramos duplicados e corrigido o despacho de ip/ssid/channel/wifi_status, antes parcialmente aninhado no comando rssi.

Foi corrigida também a URL de clima, que apontava para a página inicial em vez da API. Mantida a localização de Porto Alegre indicada pelo código original; altere latitude/longitude em ClimaManager.h se necessário.

## Validação e limites

Foi feita revisão estática dos pinos, referências ao controlador e dimensões do painel e OTA. Não foi possível compilar nesta sessão porque a instalação Arduino local não estava acessível, nem testar na placa física. Portanto este pacote é código-fonte adaptado, sem firmware binário validado.

Depois de carregar, confira: imagem sem deslocamento, conexão WiFi, resposta ao comando `help` por UDP, sincronização do relógio, LED e OTA.

## Atualizacao

Incorporados ate cinco redes WiFi, paginas web /info e /wifi e navegacao por Key2 (GPIO10) entre relogio, estado e rede, com retorno apos 30 segundos. Pagina SD excluida.

Nomes dos comandos mantidos conforme referencia. Removidos redundantes: init (use reason), heap/heap_min (use ram), flash_info (use flash), selftest (use health). Consulte help. Atualize todos os arquivos juntos; cabecalhos e implementacoes estao alinhados. Compilacao e teste fisico pendentes.

## Novos updates

Incluidos set_time para ajuste local manual, tentativa de clima a cada 60 segundos enquanto nao sincronizado, telas de sistema e redes salvas, botao com toque/pressao longa. GPIO10: toque troca telas; 3 segundos e soltar reinicia; 10 segundos e soltar apaga redes e reinicia. As mensagens de confirmacao permanecem visiveis enquanto o botao esta pressionado. Sem dependencia de SD ou LED RGB. Verificacao estatica concluida; compilacao e teste na placa pendentes.

Display girado 90 graus para a esquerda em todas as telas, inclusive console e OTA. Ajuste global DISPLAY_ROTATION_OFFSET em BoardConfig.h.

## Botoes de pagina

Key1 (GPIO8) e BOOT (GPIO9) agora tambem trocam paginas ao pressionar, com captura por interrupcao e bloqueio de repeticao ate soltar. Key2 (GPIO10) mantem troca ao soltar e funcoes de pressao longa. O monitor serial mostra [BOTAO] Pagina para Key1/BOOT. Nao segure BOOT ao ligar ou reiniciar, pois ele seleciona o modo de gravacao da placa.

## Economia de bateria

Envie `desliga` por UDP. Desliga radio WiFi, LED e controlador LCD e entra em deep sleep sem temporizador de retorno. Redes salvas permanecem. Para ligar novamente, pressione RESET (botao R) ou desligue/ligue a alimentacao. Key1, Key2 e BOOT nao despertam deste modo: seus GPIOs estao fora do conjunto 0-5 que permite despertar o ESP32-C3 de deep sleep. O backlight ligado a 3.3V no esquema continua consumindo; apagar a iluminacao exige alteracao eletrica. Consumo final e comportamento precisam ser medidos/testados na placa.

Barrinhas WiFi: escala adaptativa das ultimas 50 amostras, com amplitude minima de 8 dBm, para mostrar variacoes pequenas. Indicam oscilacao relativa recente; nao sao uma escala absoluta de qualidade. Sem variacao real de RSSI, permanecem estaveis; sem conexao ficam cinzas.

## Lista circular de SSIDs

Cadastro em /wifi: cinco slots persistentes com SSID e senha. Redes novas seguem 1,2,3,4,5,1; a sexta substitui a primeira. Repetir um SSID atualiza sua senha sem avancar o cursor. A pagina mostra todos os slots, inclusive vazios, a rede conectada e o proximo slot. O cursor e as credenciais sobrevivem a reinicio e ao comando desliga. Mantida a validacao corrigida de putString e leitura de confirmacao.
