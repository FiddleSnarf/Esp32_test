#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#include "StepMotor/StepMotorController.h"

namespace
{
    const char* LOG = "Main";

    // Пиновка
    const gpio_num_t GPIO_EN = GPIO_NUM_15;
    const gpio_num_t GPIO_DIR = GPIO_NUM_2;
    const gpio_num_t GPIO_STEP = GPIO_NUM_4;
}

extern "C" void app_main()
{
    ESP_LOGI(LOG, "Starting StepMotorController test");

    // Инициализация контроллера
    StepMotorController::InitParams params = {
        .enPin = GPIO_EN,
        .dirPin = GPIO_DIR,
        .stepPin = GPIO_STEP,
        .stepMode = StepMotorController::EnStepMode::en1_1,
        .enableInverse = false,
    };

    StepMotorController motor(params);
    motor.setEnabled(true);

    ESP_LOGI(LOG, "Min speed: %.2f grad/s", motor.getMinSpeed());
    ESP_LOGI(LOG, "Max speed: %.2f grad/s", motor.getMaxSpeed());

    // Тест: разгон до 100 град/с с ускорением 500 град/с²
    motor.setTargetSpeed(100.0, 500.0, 500.0);

    // Основной цикл - вызываем update() периодически
    while (1)
    {
        //motor.update();
        //vTaskDelay(pdMS_TO_TICKS(10)); // Вызов каждые 10 мс
    }
}
