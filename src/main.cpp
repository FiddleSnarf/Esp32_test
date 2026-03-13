#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "driver/rmt_tx.h"
#include "StepMotor/StepGenerator.h"

#include "Helpers/LedController.h"

#include "led_strip.h"

///////////////////////////////Change the following configurations according to your board//////////////////////////////
const gpio_num_t STEP_MOTOR_GPIO_EN = GPIO_NUM_15;
const gpio_num_t STEP_MOTOR_GPIO_DIR = GPIO_NUM_2;
const gpio_num_t STEP_MOTOR_GPIO_STEP = GPIO_NUM_4;
#define STEP_MOTOR_ENABLE_LEVEL  0 // DRV8825 is enabled on low level
#define STEP_MOTOR_SPIN_DIR_CLOCKWISE 0
#define STEP_MOTOR_SPIN_DIR_COUNTERCLOCKWISE !STEP_MOTOR_SPIN_DIR_CLOCKWISE

#define STEP_MOTOR_RESOLUTION_HZ 1000000 // 1MHz resolution

static const char *TAG = "example";



extern "C" void app_main()
{
    /*gpio_set_direction(STEP_MOTOR_GPIO_EN, GPIO_MODE_OUTPUT);
    gpio_set_direction(STEP_MOTOR_GPIO_DIR, GPIO_MODE_OUTPUT);
    gpio_set_direction(STEP_MOTOR_GPIO_STEP, GPIO_MODE_OUTPUT);

    gpio_set_level(STEP_MOTOR_GPIO_EN, 0);
    gpio_set_level(STEP_MOTOR_GPIO_DIR, 0);
    gpio_set_level(STEP_MOTOR_GPIO_STEP, 0);

    while (1)
    {
        gpio_set_level(STEP_MOTOR_GPIO_STEP, 1);
        esp_rom_delay_us(20);
        gpio_set_level(STEP_MOTOR_GPIO_STEP, 0);

        vTaskDelay(pdMS_TO_TICKS(10));
    }*/

    ESP_LOGI(TAG, "Initialize EN + DIR GPIO");
    //const uint64_t bitMask = 0;
    gpio_config_t en_dir_gpio_config = {
        .pin_bit_mask = 1ULL << STEP_MOTOR_GPIO_DIR | 1ULL << STEP_MOTOR_GPIO_EN,
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&en_dir_gpio_config));

    ESP_LOGI(TAG, "Create RMT TX channel");
    rmt_channel_handle_t motor_chan = NULL;
    rmt_tx_channel_config_t tx_chan_config = {
        .gpio_num = STEP_MOTOR_GPIO_STEP,
        .clk_src = RMT_CLK_SRC_DEFAULT, // select clock source
        .resolution_hz = STEP_MOTOR_RESOLUTION_HZ,
        .mem_block_symbols = 64,
        .trans_queue_depth = 10, // set the number of transactions that can be pending in the background
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &motor_chan));

    ESP_LOGI(TAG, "Set spin direction");
    gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_CLOCKWISE);
    ESP_LOGI(TAG, "Enable step motor");
    gpio_set_level(STEP_MOTOR_GPIO_EN, STEP_MOTOR_ENABLE_LEVEL);

    ESP_LOGI(TAG, "Create motor encoders");
    stepper_motor_curve_encoder_config_t accel_encoder_config = {
        .resolution = STEP_MOTOR_RESOLUTION_HZ,
        .sample_points = 500,
        .start_freq_hz = 500,
        .end_freq_hz = 1500,
    };
    rmt_encoder_handle_t accel_motor_encoder = NULL;
    ESP_ERROR_CHECK(rmt_new_stepper_motor_curve_encoder(&accel_encoder_config, &accel_motor_encoder));

    stepper_motor_uniform_encoder_config_t uniform_encoder_config = {
        .resolution = STEP_MOTOR_RESOLUTION_HZ,
    };
    rmt_encoder_handle_t uniform_motor_encoder = NULL;
    ESP_ERROR_CHECK(rmt_new_stepper_motor_uniform_encoder(&uniform_encoder_config, &uniform_motor_encoder));

    stepper_motor_curve_encoder_config_t decel_encoder_config = {
        .resolution = STEP_MOTOR_RESOLUTION_HZ,
        .sample_points = 500,
        .start_freq_hz = 1500,
        .end_freq_hz = 500,
    };
    rmt_encoder_handle_t decel_motor_encoder = NULL;
    ESP_ERROR_CHECK(rmt_new_stepper_motor_curve_encoder(&decel_encoder_config, &decel_motor_encoder));

    ESP_LOGI(TAG, "Enable RMT channel");
    ESP_ERROR_CHECK(rmt_enable(motor_chan));

    ESP_LOGI(TAG, "Spin motor for 6000 steps: 500 accel + 5000 uniform + 500 decel");
    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
    };

    const static uint32_t accel_samples = 1000;
    const static uint32_t uniform_speed_hz = 1500;
    const static uint32_t decel_samples = 1000;



    // Светодиод

    // Handle светодиодной ленты
    led_strip_handle_t led_strip;

    /* Конфигурация ленты (общая) */
    led_strip_config_t strip_config = {
        .strip_gpio_num = 48,
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812
    };

    /* Конфигурация RMT (аппаратный контроллер) */
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10000000, // 10MHz
    };

    /* Создание устройства */
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

    /* Очистка ленты перед началом (выключение) */
    ESP_ERROR_CHECK(led_strip_clear(led_strip));


    uint16_t h = 0;
    uint8_t s = 255;
    uint8_t v = 10;

    while (1)
    {
        /*// acceleration phase
        tx_config.loop_count = 0;
        ESP_ERROR_CHECK(rmt_transmit(motor_chan, accel_motor_encoder, &accel_samples, sizeof(accel_samples), &tx_config));

        // uniform phase
        tx_config.loop_count = 10;
        ESP_ERROR_CHECK(rmt_transmit(motor_chan, uniform_motor_encoder, &uniform_speed_hz, sizeof(uniform_speed_hz), &tx_config));

        // deceleration phase
        tx_config.loop_count = 0;
        ESP_ERROR_CHECK(rmt_transmit(motor_chan, decel_motor_encoder, &decel_samples, sizeof(decel_samples), &tx_config));
        // wait all transactions finished
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(motor_chan, -1));*/


        /*led_strip_set_pixel_hsv(led_strip, 0, h, s, v);
        led_strip_refresh(led_strip);
        vTaskDelay(pdMS_TO_TICKS(10));

        ++h;
        if (h > 360)
            h = 0;*/
    }
}