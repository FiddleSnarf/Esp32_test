#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include "WiFi/WifiController.h"
#include "Helpers/LedController.h"
#include "Servo_pwm/ServoControl.h"
#include "Telegram/TelegramBotController.h"

static const char* MAIN_LOG_TAG = "MAIN";


extern "C" void app_main()
{
    ESP_LOGI(MAIN_LOG_TAG, "START TEST");

    WiFiManager wifi("MERCUSYS_300D", "19108490");
    wifi.connect();
    vTaskDelay(pdMS_TO_TICKS(5000));

    LedController led(GPIO_NUM_2);
    //ServoControl servo(GPIO_NUM_15, LEDC_TIMER_0, LEDC_CHANNEL_0);

    TelegramBot telegramBot("8250618570:AAFysJ9jy-QQvtifjPC6s4otX-0YPi7-x_w");
    telegramBot.startPolling();

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}