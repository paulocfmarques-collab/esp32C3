# Comandos ESP32-C3

Envie o texto por UDP ao IP da placa, porta **4210**. A resposta retorna ao remetente.

## SISTEMA

| Comando | Descrição |
|---|---|
| `help` | Mostra a lista de comandos. |
| `info` | Resumo do dispositivo. |
| `status` | Estado do Wi-Fi, relógio e memória. |
| `uptime` | Tempo ligado. |
| `reason` | Motivo do último reset. |
| `version` | Versão gerada na compilação. |
| `build` | Data e hora de compilação. |
| `alive` | Responde com o IP. |
| `reboot` | Reinicia o dispositivo. |
| `desliga` | Sono profundo: desativa Wi-Fi, LED e LCD; retorna por RESET. O backlight permanece alimentado. |
| `temp` | Temperatura interna do chip. |
| `cpu` | Modelo, revisão e frequência da CPU. |
| `ram` | Uso e disponibilidade de RAM. |
| `flash` | Informações da memória flash. |
| `chip_info` | Chip e SDK. |
| `health` | Diagnóstico de Wi-Fi, relógio, RAM e clima. |

## REDE

| Comando | Descrição |
|---|---|
| `wifi_list` | Retorna somente os SSIDs cadastrados, um por linha; lista vazia retorna uma linha em branco. |
| `net_info` | IP, gateway, DNS e sinal. |
| `mac` | Endereço MAC. |
| `reset_wifi` | Apaga redes salvas e reinicia. |
| `wifi_add:SSID\|SENHA` | Cadastra rede e senha na lista circular de cinco slots; SSID existente atualiza a senha. Use `reboot` para buscar as redes após salvar. |
| `rssi` | Intensidade do sinal em dBm. |
| `ip` | IP atual. |
| `ssid` | Rede conectada. |
| `channel` | Canal Wi-Fi. |
| `wifi_status` | Estado da conexão. |

## HORÁRIO

| Comando | Descrição |
|---|---|
| `time` | Hora local. |
| `date` | Data local. |
| `ntp_status` | Estado do relógio. |
| `set_fuso:X` | Define fuso UTC de -12 a +14. |
| `set_time:AAAA-MM-DD HH:MM:SS` | Ajusta a data e hora local manualmente. |
| `dst_on` | Ativa ajuste de horário de verão. |
| `dst_off` | Desativa ajuste de horário de verão. |

## LED

| Comando | Descrição |
|---|---|
| `led_on` | Liga o LED simples. |
| `led_off` | Desliga o LED e interrompe efeitos. |
| `led_breath` | Ativa pulsação do LED. |
| `led_pisca:P:I` | P=1–100 pulsos; I=1–5000 ms. |
| `led_blink:I` | Pisca continuamente; I=50–60000 ms. |

## CLIMA

| Comando | Descrição |
|---|---|
| `clima` | Temperatura e condição do clima. |
| `clima_age` | Tempo desde a tentativa de atualização. |
| `clima_sync` | Força consulta do clima. |

## Exemplos de horário e LED

```text
set_fuso:-3
set_time:2026-10-07 14:30:00
led_pisca:5:250
led_blink:500
led_off
```

## Display e energia

Console girado 90° à direita em relação à versão anterior; demais páginas mantêm a orientação. Renderização em memória com envio de cada quadro pronto para reduzir piscadas. Atualização mantida em 1 segundo; buffer de 32 KB, com fallback caso não haja RAM.

`desliga` usa deep sleep, preserva redes e não tem despertar automático. Use RESET ou um ciclo de alimentação. Key1/Key2/BOOT não despertam desse modo. Backlight ligado diretamente continua consumindo.

## Cadastro de Wi-Fi e recuperação

`wifi_add:SSID|SENHA` cadastra SSID e senha na lista circular de cinco redes; um SSID repetido atualiza a senha. Exemplos:

```text
wifi_add:MinhaRede|MinhaSenha
wifi_add:RedeAberta|
reboot
```

 SSID nao pode conter `|`; a senha pode. Nao use espacos no final da senha, pois o processador remove espacos finais do comando. A senha nao aparece nas respostas nem no console. O UDP atual nao e criptografado: use rede de confianca. Depois envie `reboot` para iniciar nova busca.

O monitor de conectividade reinicia apos 30 minutos continuos sem WL_CONNECTED, inclusive no modo AP. Conectar um cliente ao AP nao cancela esse prazo. Uma conexao STA valida zera a contagem. O SSID de cada tentativa aparece na tela; redes nao detectadas sao puladas. Este monitor depende do loop em funcionamento e nao substitui watchdog de travamento da CPU.

## Navegacao e limpeza de configuracoes

K1 avanca as telas; K2 retorna ao soltar. BOOT e reservado para limpar configuracoes.

Durante o funcionamento, segure BOOT por 5 segundos. Soltar antes cancela. Ao aparecer CONFIG APAGADA, solte BOOT: a placa reinicia e abre o AP. Limpa somente redes salvas e configuracoes do relogio, preservando firmware. EN continua sendo apenas reset. Nao mantenha BOOT pressionado ao ligar ou resetar, pois isso entra no modo de gravacao.

## Cores das barrinhas Wi-Fi

Uma barra acesa: vermelho; duas: amarelo; tres ou quatro: verde. Cor e quantidade usam a mesma escala adaptativa do RSSI recente. Sem conexao: cinza. Sinal constante mantem a indicacao estavel; esta escala mostra variacoes relativas, nao qualidade absoluta.

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
