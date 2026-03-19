#pragma once

#include "led_strip.h"
#include <memory>

class RgbLedController
{
public:
    RgbLedController();
    ~RgbLedController();

    /**
     * @brief Установка цвета светодиода (модель цвета HSV)
     * @param hue: hue (0 - 360)
     * @param saturation: saturation (0 - 65535)
     * @param value: value (0 - 65535)
     */
    void ledOnHSV(uint16_t hue, uint16_t saturation, uint16_t value);

    /**
     * @brief Установка цвета светодиода (модель цвета RGB)
     */
    void ledOnRGB(uint32_t red, uint32_t green, uint32_t blue);

    void ledOff();
    bool getLedState() const;

private:
    led_strip_handle_t m_ledStrip;
    bool m_ledState = false;
};
using RgbLedControllerPtr = std::shared_ptr<RgbLedController>;