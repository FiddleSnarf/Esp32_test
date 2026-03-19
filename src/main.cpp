#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "StepMotor/StepGenerator.h"


const gpio_num_t GPIO_EN = GPIO_NUM_15;
const gpio_num_t GPIO_DIR = GPIO_NUM_2;
const gpio_num_t GPIO_STEP = GPIO_NUM_4;


void init()
{
    // Настройка ENABLE (обычный GPIO)
    gpio_config_t io_conf_1 = {
        .pin_bit_mask = (1ULL << GPIO_EN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf_1);
    gpio_set_level(GPIO_EN, 0);

    // Настройка DIR (обычный GPIO)
    gpio_config_t io_conf_2 = {
        .pin_bit_mask = (1ULL << GPIO_DIR),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf_2);
    gpio_set_level(GPIO_DIR, 1); // Направление вперед
}

extern "C" void app_main()
{
    init();
    StepGenerator gen(GPIO_STEP, 50);

    while (1)
    {
        float speed = 0;
        for (int i = 0; i < 300; ++i)
        {
            speed += 20;
            gen.setFreq(speed);

            vTaskDelay(pdMS_TO_TICKS(5));
        }

        for (int i = 0; i < 250; ++i)
        {
            speed -= 20;
            gen.setFreq(speed);

            vTaskDelay(pdMS_TO_TICKS(5));
        }

        gen.stop();

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}