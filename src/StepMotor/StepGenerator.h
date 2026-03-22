#pragma once

#include "driver/gpio.h"
#include "driver/mcpwm_prelude.h"

class StepGenerator
{
    static const uint32_t MAX_TIMER_RESOLUTION = 1'000'000;

public:
    /**
     * @brief Конструктор
     * @param stepPin: номер пина STEP
     * @param timerResolutionHz: разрешение таймера, Гц
     */
    StepGenerator(gpio_num_t stepPin, uint32_t timerResolutionHz = MAX_TIMER_RESOLUTION);
    ~StepGenerator();

    /**
     * @brief Метод для установки частоты ипульсов (может быть применен "на лету")
     * @param pulsesFreq: Частота, Гц
     */
    bool setFreq(uint32_t pulsesFreq);

    /**
     * @brief Метод для остановки выдачи импульсов
     */
    void stop();

    /**
     * @brief Метод для получения минимальной допустимой частоты
     * @return Минимальная частота, Гц
     */
    uint32_t getMinFreq() const;

    /**
     * @brief Метод для получения максимальной допустимой частоты
     * @return Максимальная частота, Гц
     */
    uint32_t getMaxFreq() const;

    /**
     * @brief Метод для получения состояния выдачи импульсов
     * @return Состояние
     */
    bool isStarted() const;

private:
    /* Метод для расчета периода в тиках таймера */
    uint32_t calcPeriodTick(uint32_t pulsesFreq) const;

private:
    mcpwm_timer_handle_t m_stepperTimer = nullptr;
    mcpwm_oper_handle_t m_stepperOper = nullptr;
    mcpwm_cmpr_handle_t m_stepperCmp = nullptr;
    mcpwm_gen_handle_t m_stepperGen = nullptr;

    const uint32_t m_timerResolutionHz = 0;             // Разрешение таймера, Гц
    const uint32_t m_pulseWidthTick = 0;                // Продолжительность импульса, тик
    const uint32_t m_minFreq = 20;                      // Минимальная частота, Гц
    const uint32_t m_maxFreq = 100'000;                 // Максимальная частота, Гц
    bool m_isStarted = false;                           // Состояние выдачи импульсов
};