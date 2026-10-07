# Comandos ESP32-C3

Envie os comandos por UDP para a porta 4210.

- `help`
- `info`
- `status`
- `uptime`
- `reason`
- `version`
- `build`
- `alive`
- `reboot`
- `temp`
- `cpu`
- `ram`
- `flash`
- `chip_info`
- `health`
- `net_info`
- `mac`
- `reset_wifi`
- `rssi`
- `ip`
- `ssid`
- `channel`
- `wifi_status`
- `time`
- `date`
- `ntp_status`
- `set_fuso:X`
- `dst_on`
- `dst_off`
- `led_on`
- `led_off`
- `led_breath`
- `led_pisca:P:I`
- `led_blink:I`
- `clima`
- `clima_age`
- `clima_sync`

`P`: numero de pulsos; `I`: intervalo em ms; `X`: fuso UTC.

Key2 (GPIO10) alterna relogio, estado e rede. Paginas web: `/info` e `/wifi`.

- `set_time:AAAA-MM-DD HH:MM:SS`: ajusta horario local manualmente; NTP pode substitui-lo ao sincronizar.

Key2: toque alterna cinco telas; segure 3 segundos e solte para reiniciar; segure 10 segundos e solte para apagar redes salvas e reiniciar.

- `desliga`: sono profundo sem despertar automatico. Voltar pelo RESET ou desligar/ligar a alimentacao. Mantem redes salvas. O backlight permanece alimentado pelo circuito da placa.
