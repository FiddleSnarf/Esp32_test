#include "StepGenerator.h"
#include <esp_log.h>
#include <cmath>
#include <algorithm>

namespace
{
    const char* LOG = "StepGenerator";                                  // Канал лога
    const uint32_t PULSE_WIDTH_Us = 4u;                                 // Длительность импульса фиксированная = 4 мкс
    const uint32_t MIN_TIMER_RESOLUTION = 1'000'000 / PULSE_WIDTH_Us;   // Минимальное требуемое разрешение таймера для принятой ширины импульса, Гц
}

StepGenerator::StepGenerator(gpio_num_t stepPin, uint32_t timerResolutionHz):
    m_timerResolutionHz(std::clamp(timerResolutionHz, MIN_TIMER_RESOLUTION, MAX_TIMER_RESOLUTION)),
    m_pulseWidthTick(std::round(static_cast<double>(PULSE_WIDTH_Us) * m_timerResolutionHz / 1'000'000)),
    m_minFreq(m_timerResolutionHz / 65535),                 // т.к. таймер использует 16ти битный счетчик
    m_maxFreq(m_timerResolutionHz / (m_pulseWidthTick + 1)) // Максимальную частоту рассчитываем так чтобы были возможны импульсы продолжительностью PULSE_WIDTH_Us
{
    if (m_timerResolutionHz != timerResolutionHz)
        ESP_LOGW(LOG, "Timer resolution to be changed = %d", m_timerResolutionHz);

    // Создание таймера
    const uint32_t defaultTimerPeriodTick = calcPeriodTick(m_minFreq);
    mcpwm_timer_config_t timer_config = {
        .group_id = 0,  // TODO ИИ говорит что можно создать 3 тамера на группу, иначе ESP_ERR_NOT_FOUND
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = m_timerResolutionHz,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
        .period_ticks = defaultTimerPeriodTick,
        .intr_priority = 0,
        .flags = {},
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &m_stepperTimer));

    // Создание оператора
    mcpwm_operator_config_t oper_config = {
        .group_id = 0,  // TODO ИИ говорит что можно создать 3 оператора на группу, иначе ESP_ERR_NOT_FOUND
        .intr_priority = 0,
        .flags = {},
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&oper_config, &m_stepperOper));
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(m_stepperOper, m_stepperTimer));

    // Создание компаратора
    mcpwm_comparator_config_t cmp_config = {
        .intr_priority = 0,
        .flags = {
            .update_cmp_on_tez = true,   // Обновлять при счете = 0
            .update_cmp_on_tep = false,
            .update_cmp_on_sync = false,
        },
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(m_stepperOper, &cmp_config, &m_stepperCmp));

    // Создание генератора
    mcpwm_generator_config_t gen_config = {
        .gen_gpio_num = stepPin,
        .flags = {},
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(m_stepperOper, &gen_config, &m_stepperGen));

    // Настройка логики генератора
    // Создаем импульс: низкий уровень в начале периода, высокий по достижении значения компаратора
    mcpwm_generator_set_action_on_timer_event(m_stepperGen, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_LOW));
    mcpwm_generator_set_action_on_compare_event(m_stepperGen, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, m_stepperCmp, MCPWM_GEN_ACTION_HIGH));

    // Установка скважности
    mcpwm_comparator_set_compare_value(m_stepperCmp, m_pulseWidthTick);

    // Запуск таймера
    mcpwm_timer_enable(m_stepperTimer);
}

StepGenerator::~StepGenerator()
{
    stop();

    // Сначала выключаем таймер
    if (m_stepperTimer)
        mcpwm_timer_disable(m_stepperTimer);

    // Удаляем в порядке обратном созданию
    if (m_stepperGen)
        mcpwm_del_generator(m_stepperGen);

    if (m_stepperCmp)
        mcpwm_del_comparator(m_stepperCmp);

    if (m_stepperOper)
        mcpwm_del_operator(m_stepperOper);

    if (m_stepperTimer)
        mcpwm_del_timer(m_stepperTimer);
}

bool StepGenerator::setFreq(uint32_t pulsesFreq)
{
    if (pulsesFreq < m_minFreq || pulsesFreq > m_maxFreq)
    {
        ESP_LOGW(LOG, "FREQ OUT OF RANGE = %d", pulsesFreq);
        return false;
    }

    if (pulsesFreq == 0)
        stop();
    else
    {
        const uint32_t periodTicks = calcPeriodTick(pulsesFreq);

        //ESP_LOGI(LOG, "FREQ = %d, PERIOD = %d", pulsesFreq, periodTicks);

        // Меняем период
        mcpwm_timer_set_period(m_stepperTimer, periodTicks);

        if (!m_isStarted)
        {
            ESP_ERROR_CHECK(mcpwm_timer_start_stop(m_stepperTimer, MCPWM_TIMER_START_NO_STOP));
            m_isStarted = true;
            //ESP_LOGI(LOG, "Pulses start");
        }
    }

    return true;
}

void StepGenerator::stop()
{
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(m_stepperTimer, MCPWM_TIMER_STOP_FULL));
    m_isStarted = false;
    //ESP_LOGI(TAG, "Pulses stop");
}

uint32_t StepGenerator::getMinFreq() const
{
    return m_minFreq;
}

uint32_t StepGenerator::getMaxFreq() const
{
    return m_maxFreq;
}

bool StepGenerator::isStarted() const
{
    return m_isStarted;
}

uint32_t StepGenerator::calcPeriodTick(uint32_t pulsesFreq) const
{
    return std::round(static_cast<double>(m_timerResolutionHz) / pulsesFreq);
}
