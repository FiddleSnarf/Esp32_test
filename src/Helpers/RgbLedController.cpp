#include "RgbLedController.h"

RgbLedController::RgbLedController()
{
    // Т.к. это для управления RGB светодиодом на плате, а управляем как лентой
    // то используем только один светодиод

    /* Конфигурация ленты (общая) */
    led_strip_config_t strip_config = {
        .strip_gpio_num = 48, // Для управления RGB светодиодом на плате используется 48 пин
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {},
    };

    /* Конфигурация RMT (аппаратный контроллер) */
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10000000, // 10MHz
        .mem_block_symbols = 64,
        .flags = {},
    };

    /* Создание устройства */
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &m_ledStrip));

    ledOff();
}

RgbLedController::~RgbLedController()
{
    led_strip_del(m_ledStrip);
}

void RgbLedController::ledOnHSV(uint16_t hue, uint16_t saturation, uint16_t value)
{
    ESP_ERROR_CHECK(led_strip_set_pixel_hsv_16(m_ledStrip, 0, hue, saturation, value));
    ESP_ERROR_CHECK(led_strip_refresh(m_ledStrip));
    m_ledState = true;
}

void RgbLedController::ledOnRGB(uint32_t red, uint32_t green, uint32_t blue)
{
    ESP_ERROR_CHECK(led_strip_set_pixel(m_ledStrip, 0, red, green, blue));
    ESP_ERROR_CHECK(led_strip_refresh(m_ledStrip));
    m_ledState = true;
}

void RgbLedController::ledOff()
{
    ESP_ERROR_CHECK(led_strip_clear(m_ledStrip));
    m_ledState = false;
}

bool RgbLedController::getLedState() const
{
    return m_ledState;
}