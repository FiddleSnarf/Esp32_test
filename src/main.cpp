#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include "Http/HttpServer.h"
#include "WiFi/WifiController.h"
#include "Helpers/LedController.h"
#include "Servo_pwm/ServoControl.h"

static const char* MAIN_LOG_TAG = "MAIN";


extern "C" void app_main()
{
    ESP_LOGI(MAIN_LOG_TAG, "START TEST");

    LedController led(GPIO_NUM_2);

    WiFiManager wifi("MERCUSYS_300D", "19108490");
    wifi.connect();

    vTaskDelay(pdMS_TO_TICKS(5000));

    HttpServer server(led);
    server.start();

    ServoControl servo(15, LEDC_TIMER_0, LEDC_CHANNEL_0);

    while (true)
    {
        servo.setSpeed(100.f);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}