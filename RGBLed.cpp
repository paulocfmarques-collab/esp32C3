#include "RGBLed.h"

void RGBLed::begin()
{
    pinMode(RGB_LED_PIN, OUTPUT);

    apply(0);

}

void RGBLed::setBrightness(uint8_t brightness)
{
    currentBrightness = brightness;
    apply(brightness);

}

void RGBLed::off()
{
    apply(0);

}

void RGBLed::setColor(uint8_t r, uint8_t g, uint8_t b)
{
    apply((r || g || b) ? currentBrightness : 0);

}

void RGBLed::red()
{
    setColor(255, 0, 0);
}

void RGBLed::green()
{
    setColor(0, 255, 0);
}

void RGBLed::blue()
{
    setColor(0, 0, 255);
}

void RGBLed::yellow()
{
    setColor(255, 255, 0);
}

void RGBLed::cyan()
{
    setColor(0, 255, 255);
}

void RGBLed::magenta()
{
    setColor(255, 0, 255);
}

void RGBLed::white()
{
    setColor(255, 255, 255);
}

void RGBLed::breathing(uint8_t r, uint8_t g, uint8_t b, uint16_t stepDelay)
{
    apply((r || g || b) ? currentBrightness : 0);

    for (int brightness = 0; brightness <= 255; brightness++)
    {
        apply(brightness);
    
        delay(stepDelay);
    }

    for (int brightness = 255; brightness >= 0; brightness--)
    {
        apply(brightness);
    
        delay(stepDelay);
    }
}

void RGBLed::breathingTask(uint8_t r, uint8_t g, uint8_t b)
{
    static int brightness = 0;
    static int direction = 1;
    static unsigned long lastUpdate = 0;

    if (millis() - lastUpdate < 10)
        return;

    lastUpdate = millis();

    apply((r || g || b) ? currentBrightness : 0);
    apply(brightness);


    brightness += direction;

    if (brightness >= 255)
    {
        brightness = 255;
        direction = -1;
    }

    if (brightness <= 0)
    {
        brightness = 0;
        direction = 1;
    }
}

void RGBLed::blink(uint8_t color, uint16_t interval, uint16_t timeBlink)
{
    unsigned long startTime = millis();
    while (millis() - startTime < timeBlink)
    {
        setColorByEnum(color);
        delay(interval);
        off();
        delay(interval);
    }
}

void RGBLed::setColorByEnum(uint8_t color)
{
    switch(color)
    {
        case RED:
            red();
            break;
        case GREEN:
            green();
            break;
        case BLUE:
            blue();
            break;
        case YELLOW:
            yellow();
            break;
        case CYAN:
            cyan();
            break;
        case MAGENTA:
            magenta();
            break;
        case WHITE:
            white();
            break;
    }
}
void RGBLed::apply(uint8_t level) { analogWrite(RGB_LED_PIN, level); }
