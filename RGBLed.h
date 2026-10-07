#ifndef RGBLED_H
#define RGBLED_H

#include <Arduino.h>
#include "BoardConfig.h"

#define RED         0
#define GREEN       1
#define BLUE        2
#define YELLOW      3
#define CYAN        4
#define MAGENTA     5
#define WHITE       6

#define RGB_LED_PIN BOARD_LED_PIN

class RGBLed
{
public:
    void begin();

    void off();

    void red();
    void green();
    void blue();

    void yellow();
    void cyan();
    void magenta();
    void white();

    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void setBrightness(uint8_t brightness);
    void breathing(uint8_t r, uint8_t g, uint8_t b, uint16_t stepDelay = 5);
    void breathingTask(uint8_t r, uint8_t g, uint8_t b);
    void blink(uint8_t color, uint16_t interval, uint16_t timeBlink);

private:
    uint8_t currentBrightness = 50;
    void apply(uint8_t level);
    void setColorByEnum(uint8_t color);
};

#endif