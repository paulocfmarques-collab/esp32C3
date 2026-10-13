# ESP32-C3 Gateway

Firmware para ESP32-C3 com display TFT, configuração de redes Wi-Fi, comandos por UDP e serial, relógio NTP, informações meteorológicas e atualização OTA.

[Início rápido](#início-rápido) · [Wi-Fi](#perfis-wi-fi) · [Comandos](#comandos-udp) · [Hardware](#hardware-e-pinagem) · [Changelog](CHANGELOG.md)

## Visão geral

O projeto usa um display ST7735 de 128 × 128 pixels e oferece sete páginas: relógio, estado, rede, sistema, redes salvas, monitor de rede e previsão diária. A previsão do tempo usa dados do Open-Meteo para Porto Alegre (RS). O monitor de rede verifica por ICMP o gateway Wi-Fi.

| Recurso | Implementação |
|---|---|
| Conectividade | Até cinco perfis Wi-Fi persistidos; portal de configuração como fallback |
| Interface web | Informações em `/info` e configuração de redes em `/wifi` |
| Comandos | UDP na porta 4210 e console serial a 115200 baud |
| Relógio | NTP, fuso horário configurável, horário de verão e ajuste manual |
| Display | Sete páginas, histórico de RSSI e protetor de tela |
| Monitor de rede | Ping ICMP ao gateway, histórico de 24 resultados e eventos recentes |
| Clima | Condições atuais e previsão diária via Open-Meteo |
| Atualização | ArduinoOTA após conexão Wi-Fi |
| Energia | Deep sleep pelo comando `desliga` |

O LED conectado ao GPIO 11 é monocromático. Embora o módulo e a classe se chamem `RGBLed`, a placa tem um único canal PWM; as cores da API não correspondem a cores físicas.

## Início rápido

1. Mantenha `ESP32C3.ino` e os arquivos `.h` e `.cpp` do projeto juntos em uma pasta chamada `ESP32C3`.
2. No Arduino IDE, instale **esp32 by Espressif Systems 3.x** e **GFX Library for Arduino (Arduino_GFX)**.
3. Selecione **ESP32C3 Dev Module**, a porta serial correta e, se usar a USB nativa para o monitor serial, habilite **USB CDC On Boot**.
4. Escolha um tamanho de flash e um esquema de partições compatíveis com sua placa e com OTA.
5. Compile e faça o primeiro upload por USB. O monitor serial usa **115200 baud**.

As bibliotecas de Wi-Fi, HTTP, Preferences, NTP e OTA fazem parte do pacote ESP32. O projeto não depende de SdFat nem de Adafruit_NeoPixel. Não há configuração de build do PlatformIO no repositório.

### Primeira conexão

Se nenhum perfil salvo conectar, o dispositivo inicia o ponto de acesso aberto `ESP32_C3_CONFIG`. Conecte-se a ele e acesse **http://192.168.4.1** para configurar uma rede. Após salvar, o dispositivo reinicia e tenta conectar-se aos perfis salvos.

Quando conectado à rede, use o endereço IP exibido no dispositivo:

- `http://IP_DO_DISPOSITIVO/info` — informações do dispositivo e da rede.
- `http://IP_DO_DISPOSITIVO/wifi` — gerenciamento de perfis Wi-Fi.

## Perfis Wi-Fi

Até cinco redes e suas senhas são armazenadas na flash. Um novo perfil ocupa o próximo slot em ordem circular (1 → 2 → 3 → 4 → 5 → 1); quando a lista está cheia, o perfil do slot substituído deixa de ser usado. Cadastrar novamente um SSID existente atualiza a senha sem avançar o próximo slot. Redes disponíveis são verificadas no início; mudanças nos perfis entram em vigor na próxima reinicialização. Redes ocultas não são descobertas pela busca atual.

Também é possível cadastrar um perfil por UDP ou serial com `wifi_add:SSID|SENHA`. Para uma rede aberta, deixe a senha vazia após `|`. O comando `wifi_list` mostra os SSIDs salvos, sem revelar senhas.

## Display e controles

As páginas auxiliares retornam ao relógio após 30 segundos. Respostas de comandos aparecem no console por 10 segundos e o protetor de tela começa após 15 minutos de inatividade.

| Controle | Ação |
|---|---|
| Key1 · GPIO 8 | Avança uma página |
| BOOT · GPIO 9 | Mantenha pressionado por 5 segundos para apagar as configurações Wi-Fi e NTP e reiniciar |
| Key2 · GPIO 10 | Volta uma página ao soltar |
| RESET | Reinicia o dispositivo; também o desperta do deep sleep |

Não mantenha BOOT pressionado ao ligar ou reiniciar a placa: esse botão também seleciona o modo de gravação do firmware.

As barras e o histórico de RSSI usam leituras reais do sinal. O histórico visual contém 50 amostras. O monitor de rede executa um ping assíncrono ao gateway aproximadamente a cada cinco segundos, com timeout de um segundo, e mantém 24 resultados e os três eventos Wi-Fi mais recentes. Esses históricos são mantidos em RAM e são apagados ao reiniciar. Um gateway que bloqueia ICMP pode não responder mesmo quando a conexão Wi-Fi está ativa; o teste não verifica acesso à Internet.

## Comandos UDP

Envie o texto do comando para o IP do dispositivo na porta **UDP 4210**. A resposta é enviada ao endereço e à porta de origem. Os mesmos comandos também podem ser executados pelo console serial.

| Categoria | Comandos |
|---|---|
| Sistema | `help`, `info`, `status`, `uptime`, `reason`, `version`, `build`, `alive`, `reboot`, `desliga` |
| Hardware | `temp`, `cpu`, `ram`, `flash`, `chip_info`, `health` |
| Rede | `wifi_list`, `wifi_add:SSID|SENHA`, `reset_wifi`, `net_info`, `mac`, `rssi`, `ip`, `ssid`, `channel`, `wifi_status` |
| Horário | `time`, `date`, `ntp_status`, `set_fuso:X`, `set_time:AAAA-MM-DD HH:MM:SS`, `dst_on`, `dst_off` |
| LED | `led_on`, `led_off`, `led_breath`, `led_pisca:P:I`, `led_blink:I` |
| Clima | `clima`, `clima_age`, `clima_sync` |

- `set_fuso:X`: define o deslocamento UTC entre −12 e +14 horas; o padrão é UTC−3.
- `set_time:2026-10-07 14:30:00`: ajusta o horário manualmente; uma sincronização NTP posterior pode substituí-lo.
- `led_pisca:P:I`: executa de 1 a 100 pulsos, com intervalo de 1 a 5000 ms.
- `led_blink:I`: pisca continuamente, com intervalo de 50 a 60000 ms.
- `desliga`: envia a resposta e entra em deep sleep; o retorno é por RESET ou ciclo de alimentação.

Exemplo de cliente UDP em Python:

```python
import socket

with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as client:
    client.settimeout(5)
    client.sendto(b"status", ("192.168.1.100", 4210))
    try:
        while True:
            data, _ = client.recvfrom(4096)
            print(data.decode("utf-8", errors="replace"))
    except socket.timeout:
        pass
```

Substitua o IP de exemplo pelo endereço do dispositivo. Alguns comandos, como a atualização do clima, podem levar mais que cinco segundos.

## Clima, OTA e deep sleep

O clima e a previsão diária são consultados para Porto Alegre, com coordenadas definidas em `ClimaManager.h`. Após uma consulta bem-sucedida, o intervalo automático é de 15 minutos; antes da primeira sincronização ou após falha, há nova tentativa após um minuto. Dados válidos anteriores são preservados em caso de falha. `clima_age` informa o tempo desde a última tentativa, que pode não ser a última consulta bem-sucedida.

O OTA usa o hostname `ESP32-C3-Gateway` e fica disponível após a conexão em modo estação. Faça o primeiro upload por USB, use partições compatíveis com OTA e garanta que o computador alcance o dispositivo pela rede.

O comando `desliga` interrompe Wi-Fi e atividade do LED, desliga o controlador do display e inicia deep sleep sem temporizador de despertar. Os perfis salvos são preservados. Key1, BOOT e Key2 não despertam o dispositivo. O backlight é ligado diretamente a 3,3 V e continua alimentado durante o deep sleep; o consumo total depende da placa e deve ser medido no hardware real.

## Hardware e pinagem

| Sinal | GPIO |
|---|---:|
| LCD CS | 2 |
| LCD DC | 0 |
| LCD reset | 5 |
| LCD MOSI | 4 |
| LCD clock | 3 |
| LED monocromático | 11 |
| Key1 | 8 |
| BOOT | 9 |
| Key2 | 10 |

O display de 128 × 128 usa SPI por software. `BoardConfig.h` define pinos, dimensões, offsets e rotação; os offsets atuais são 2 e 3. Revisões diferentes do painel podem exigir ajuste. O backlight não tem controle por GPIO.

## Estrutura do projeto

| Arquivo | Responsabilidade |
|---|---|
| `ESP32C3.ino` | Inicialização, navegação, botões e ciclo principal |
| `BoardConfig.h` | Pinos e configuração do display |
| `ESP32Gateway.h/.cpp` | Perfis Wi-Fi, portal HTTP e UDP |
| `CommandProcessor.h/.cpp` | Despacho de comandos e diagnósticos |
| `DisplayUtil.h/.cpp` | Display, páginas e visualização do RSSI |
| `NTPUtil.h/.cpp` | Relógio, fuso e ajuste manual |
| `ClimaManager.h` | Consultas ao Open-Meteo |
| `NetworkMonitor.h/.cpp` | Monitoramento ICMP e histórico de rede |
| `OTAManager.h` | Atualização ArduinoOTA |
| `RGBLed.h/.cpp` | Controle PWM do LED monocromático |

## Segurança e limitações

O ponto de acesso de configuração é aberto. As páginas HTTP, comandos UDP e atualizações OTA não têm autenticação configurada. O cliente HTTPS do serviço meteorológico usa `setInsecure()` e não valida o certificado do servidor. Use o dispositivo somente em uma rede confiável e considere essas limitações antes de expô-lo a outras redes. Comandos `wifi_add` são ocultados nos logs do firmware, mas isso não substitui a proteção da rede.

O repositório não registra uma compilação completa nem testes de regressão no hardware para a versão atual. Ao atualizar o firmware, substitua o conjunto completo dos arquivos-fonte para evitar misturar cabeçalhos e implementações incompatíveis.

## Contribuição e histórico

Ao relatar problemas, inclua a revisão da placa, a versão do pacote ESP32, os passos para reproduzir e a saída completa do erro. Remova senhas e informações privadas dos logs.

Consulte [CHANGELOG.md](CHANGELOG.md) para o histórico de alterações. O repositório também contém guias em português para comandos (`COMANDOS.md`) e uso (`LEIA-ME.md`).
