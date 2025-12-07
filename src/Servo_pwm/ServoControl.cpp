#include "ServoControl.h"
#include <algorithm>

// Конфигурация ШИМ
constexpr auto LEDC_MODE = LEDC_LOW_SPEED_MODE;
constexpr uint32_t LEDC_DUTY_RES = 1 << LEDC_TIMER_13_BIT;
constexpr uint32_t LEDC_FREQUENCY = 50;                             // 50 Гц
constexpr uint32_t LEDC_PERIOD = 1. / LEDC_FREQUENCY * 1000000.;    // Период, мкс

// Значения импульсов, мкс
constexpr uint32_t SERVO_PULSE_STOP = 1500;     // Стоп
constexpr uint32_t SERVO_PULSE_DELTA = 1000;    // Дапазон в одну из сторон

ServoControl::ServoControl(uint16_t pulsePin, ledc_timer_t ledcTimer, ledc_channel_t ledcChannel):
    m_pulsePin(pulsePin),
    m_ledcTimer(ledcTimer),
    m_ledcChannel(ledcChannel)
{
    // Конфигурация таймера LEDC
    ledc_timer_config_t ledc_timer =
    {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num = ledcTimer,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Конфигурация канала LEDC
    ledc_channel_config_t ledc_ch =
    {
        .gpio_num = pulsePin,
        .speed_mode = LEDC_MODE,
        .channel = ledcChannel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = ledcTimer,
        .duty = 0,
        .hpoint = 0,
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_ch));
}

void ServoControl::setSpeed(float speedPercent)
{
    const float speed_k = std::max(std::min(speedPercent / 100.f, 1.f), -1.f); // От -1. до 1.
    const uint32_t pulseWidth = SERVO_PULSE_STOP + static_cast<uint32_t>(SERVO_PULSE_DELTA * speed_k);
    const uint32_t duty = pulseWidth * LEDC_DUTY_RES / LEDC_PERIOD;

    ledc_set_duty(LEDC_MODE, m_ledcChannel, duty);
    ledc_update_duty(LEDC_MODE, m_ledcChannel);
}
