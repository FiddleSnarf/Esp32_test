#pragma once

#include "driver/ledc.h"

class ServoControl
{
public:
    ServoControl(uint16_t pulsePin, ledc_timer_t ledcTimer, ledc_channel_t ledcChannel);

    void setSpeed(float speedPercent);

private:
    const uint16_t m_pulsePin;
    const ledc_timer_t m_ledcTimer;
    const ledc_channel_t m_ledcChannel;
};