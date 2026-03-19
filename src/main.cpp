#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "driver/mcpwm_prelude.h"
#include <cmath>


const gpio_num_t GPIO_EN = GPIO_NUM_15;
const gpio_num_t GPIO_DIR = GPIO_NUM_2;
const gpio_num_t GPIO_STEP = GPIO_NUM_4;


const uint32_t timerResolution = 1'000'000; // 1МГц, 1/частота = 1 мкс точность

// --- Глобальные объекты драйвера ---
static mcpwm_timer_handle_t stepper_timer = nullptr;
static mcpwm_oper_handle_t stepper_oper = nullptr;
static mcpwm_cmpr_handle_t stepper_cmp = nullptr;
static mcpwm_gen_handle_t stepper_gen = nullptr;

void stepper_motor_init()
{
    const uint32_t defaultTimerPeriod = 1000;

    // 1. Настройка ENABLE (обычный GPIO)
    gpio_config_t io_conf_1 = {
        .pin_bit_mask = (1ULL << GPIO_EN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf_1);
    gpio_set_level(GPIO_DIR, 0);

    // 1. Настройка DIR (обычный GPIO)
    gpio_config_t io_conf_2 = {
        .pin_bit_mask = (1ULL << GPIO_DIR),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf_2);
    gpio_set_level(GPIO_DIR, 1); // Направление вперед

    // 2. Создание таймера (Сердце системы)
    mcpwm_timer_config_t timer_config = {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = timerResolution,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
        .period_ticks = defaultTimerPeriod,
        .intr_priority = 0,
        .flags = {},
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &stepper_timer));

    // 3. Создание оператора (привязка к таймеру)
    mcpwm_operator_config_t oper_config = {
        .group_id = 0,
        .intr_priority = 0,
        .flags = {},
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&oper_config, &stepper_oper));
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(stepper_oper, stepper_timer));

    // 4. Создание компаратора
    mcpwm_comparator_config_t cmp_config = {
        .intr_priority = 0,
        .flags = {
            .update_cmp_on_tez = true,   // Обновлять при счете = 0
            .update_cmp_on_tep = false,
            .update_cmp_on_sync = false,
        },
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(stepper_oper, &cmp_config, &stepper_cmp));

    // 5. Создание генератора (Выход сигнала)
    mcpwm_generator_config_t gen_config = {
        .gen_gpio_num = GPIO_STEP,
        .flags = {},
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(stepper_oper, &gen_config, &stepper_gen));

    // 6. Настройка логики генератора
    // Создаем импульс: низкий уровень в начале периода, высокий по достижении значения компаратора
    mcpwm_generator_set_action_on_timer_event(stepper_gen, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_LOW));
    mcpwm_generator_set_action_on_compare_event(stepper_gen, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, stepper_cmp, MCPWM_GEN_ACTION_HIGH));

    // 7. Установка скважности (50% от периода)
    mcpwm_comparator_set_compare_value(stepper_cmp, std::round(defaultTimerPeriod / 2.));

    // 8. Запуск таймера
    mcpwm_timer_enable(stepper_timer);
    //mcpwm_timer_start_stop(stepper_timer, MCPWM_TIMER_START_NO_STOP);
}

// --- Управление скоростью ---
void stepper_set_speed(float steps_per_sec)
{
    if (!stepper_timer)
        return;

    const uint32_t period_ticks = std::round(timerResolution / steps_per_sec);
    const uint32_t compareVal = std::round(period_ticks / 2.);

    // Меняем период
    mcpwm_timer_set_period(stepper_timer, period_ticks);

    // Меняем скважность
    mcpwm_comparator_set_compare_value(stepper_cmp, compareVal);
}

// --- Старт / Стоп ---
void stepper_start()
{
    mcpwm_timer_start_stop(stepper_timer, MCPWM_TIMER_START_NO_STOP);
}

void stepper_stop()
{
    mcpwm_timer_start_stop(stepper_timer, MCPWM_TIMER_STOP_FULL);
}

extern "C" void app_main()
{
    float speed = 100;
    stepper_motor_init();
    stepper_set_speed(speed);
    mcpwm_timer_start_stop(stepper_timer, MCPWM_TIMER_START_NO_STOP);

    for (int i = 0; i < 30; ++i)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
        speed += 100;
        stepper_set_speed(speed);
    }

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}