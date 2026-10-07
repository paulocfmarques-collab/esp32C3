#pragma once
// ESP32-C3 LCD 1.44: esquema fornecido pelo usuario.
#define TFT_CS 2
#define TFT_DC 0
#define TFT_RST 5
#define TFT_SCK 3
#define TFT_MOSI 4
// Backlight ligado diretamente a 3.3V; sem GPIO.
#define TFT_WIDTH 128
#define TFT_HEIGHT 128
#define TFT_OFFSET_X 2
#define TFT_OFFSET_Y 3
// Ajuste offsets/inversao se a revisao do painel for diferente.
#define TFT_IPS false
#define BOARD_LED_PIN 11

// Deslocamento global de 90 graus para a esquerda.
#define DISPLAY_ROTATION_OFFSET 3
