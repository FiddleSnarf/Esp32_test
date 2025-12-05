#pragma once

#include <driver/gpio.h>

class LedController
{
public:
    LedController(gpio_num_t pin):
        m_led_pin(pin)
    {
        gpio_reset_pin(m_led_pin);
        gpio_set_direction(m_led_pin, GPIO_MODE_OUTPUT);
    }

    void toggleLed() 
    {
        m_ledState = !m_ledState;
        updateState();
    }

    void set(bool state)
    {
        m_ledState = state;
        updateState();
    }

    bool get() const
    {
        return m_ledState;
    }

private:
    void updateState()
    {
        gpio_set_level(m_led_pin, m_ledState);
    }

private:
    gpio_num_t m_led_pin;
    bool m_ledState = false;
};