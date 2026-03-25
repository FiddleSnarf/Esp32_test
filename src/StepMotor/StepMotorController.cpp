#include "StepMotorController.h"
#include "esp_log.h"
#include <algorithm>
#include <cmath>

namespace
{
    const char* LOG = "StepMotorController";    // Канал лога
    const float ANGLE_ON_STEP = 1.8;            // Обычно для ШД 1.8 градусов на шаг
}

StepMotorController::StepMotorController(const InitParams& params):
    m_initParams(params),
    m_stepGen(params.stepPin),
    m_clbK(ANGLE_ON_STEP / static_cast<float>(params.stepMode)),
    m_lastUpdateTick(esp_timer_get_time() / 1000)
{
    // Инициализация ENABLE
    if (params.enPin != GPIO_NUM_NC)
    {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << params.enPin),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&io_conf);
        setEnabled(false);
    }

    // Инициализация DIR
    if (params.dirPin != GPIO_NUM_NC)
    {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << params.dirPin),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&io_conf);
    }
}

StepMotorController::~StepMotorController()
{
    hardStop();
}

void StepMotorController::setEnabled(bool state)
{
    if (m_initParams.enPin != GPIO_NUM_NC)
    {
        const bool actualState = m_initParams.enableInverse ? !state : state;
        gpio_set_level(m_initParams.enPin, actualState ? 1 : 0);
        ESP_LOGD(LOG, "Set enabled = %d", state);
    }
}

void StepMotorController::setTargetSpeed(float targetSpeed, float acc, float dec)
{
    // Знак скорости определяет направление

    m_moveProfile = {
        .controlMode = EnControlMode::enSpeedControl,
        .targetPos = 0.,
        .targetSpeed = targetSpeed,
        .acceleration = acc,
        .deceleration = dec
    };

    // TODO
}

void StepMotorController::setTargetPosition(double targetPos, float targetSpeed, float acc, float dec)
{
    targetSpeed = std::abs(targetSpeed);

    // TODO Тут пока ничего не надо
}

void StepMotorController::softStop()
{
    // TODO Тут пока ничего не надо
}

void StepMotorController::hardStop()
{
    m_stepGen.stop();
    m_state = State::Idle;
    m_currentSpeed = 0;
}

void StepMotorController::resetCurrentPosition(double newPosition)
{
    // TODO Тут пока ничего не надо
}

double StepMotorController::getMinSpeed() const
{
    return m_stepGen.getMinFreq() * m_clbK;
}

double StepMotorController::getMaxSpeed() const
{
    return m_stepGen.getMaxFreq() * m_clbK;
}

double StepMotorController::getCurrentPosition() const
{
    // TODO Тут пока ничего не надо
    return 0.;
}

float StepMotorController::getCurrentSpeed() const
{
    return m_currentSpeed * m_clbK;
}

uint32_t StepMotorController::angleToSteps(float angle) const
{
    return static_cast<uint32_t>(std::round(angle / m_clbK));
}

uint64_t StepMotorController::angleToSteps(double angle) const
{
    return static_cast<uint64_t>(std::round(angle / m_clbK));
}

float StepMotorController::stepsToAngle(uint32_t angle) const
{
    return angle * m_clbK;
}

double StepMotorController::stepsToAngle(uint64_t angle) const
{
    return angle * m_clbK;
}

void StepMotorController::setDirection(bool dirState)
{
    if (m_initParams.dirPin != GPIO_NUM_NC)
    {
        // Направление движения должно меняться в моменты нулевой скорости.
        // Например если мотор вращался в одну сторону, а потом резко понадобилось изменить направление
        // То необходимо замедлиться до остановки, только потом вызвать этот метод для смены направления, а затем разогнаться до требуемой скорости

        const bool actualDirState = m_initParams.directionInverse ? !dirState : dirState;
        gpio_set_level(m_initParams.dirPin, actualDirState ? 1 : 0);
        ESP_LOGD(LOG, "Set direction = %d", dirState);
    }
}

void StepMotorController::updateMotion()
{
    uint64_t currentTick = esp_timer_get_time() / 1000;
    double dt = static_cast<double>(currentTick - m_lastUpdateTick) / 1000.0; // секунды
    m_lastUpdateTick = currentTick;

    // Защита от некорректных значений dt
    if (dt <= 0 || dt > 1.0)
        dt = 0.001;

    switch (m_state)
    {
    case State::Accelerating:
    {
        if (m_profile.targetSpeed <= 0)
        {
            m_state = State::Idle;
            m_currentSpeed = 0;
            m_stepGen.stop();
            return;
        }

        // Разгон
        m_currentSpeed += m_profile.acceleration * dt;

        if (m_currentSpeed >= m_profile.targetSpeed)
        {
            m_currentSpeed = m_profile.targetSpeed;
            m_state = State::ConstantSpeed;
            ESP_LOGD(LOG, "Reached target speed=%.2f", m_currentSpeed);
        }

        uint32_t freq = speedToFreq(m_currentSpeed);
        m_stepGen.setFreq(freq);
        break;
    }

    case State::ConstantSpeed:
    {
        // Проверка: если нужно тормозить (скорость = 0 или команда на остановку)
        if (m_profile.targetSpeed == 0)
        {
            m_state = State::Decelerating;
        }
        break;
    }

    case State::Decelerating:
    {
        // Торможение
        m_currentSpeed -= m_profile.deceleration * dt;

        if (m_currentSpeed <= 0)
        {
            m_currentSpeed = 0;
            m_state = State::Idle;
            m_stepGen.stop();
            ESP_LOGD(LOG, "Stopped");
            return;
        }

        uint32_t freq = speedToFreq(m_currentSpeed);
        m_stepGen.setFreq(freq);
        break;
    }

    case State::Idle:
        // Ничего не делаем
        break;
    }
}
