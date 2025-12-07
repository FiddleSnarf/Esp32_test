#include "HttpServer.h"
#include <esp_log.h>
#include <string>

static const char* HTTP_LOG_TAG = "HTTP";

HttpServer::HttpServer(LedController& led):
    m_led(led),
    m_server(nullptr)
{
}

void HttpServer::start()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    if (httpd_start(&m_server, &config) == ESP_OK)
    {
        register_handlers();
        ESP_LOGI(HTTP_LOG_TAG, "HTTP-сервер запущен на порту %d", config.server_port);
    } else {
        ESP_LOGE(HTTP_LOG_TAG, "Ошибка запуска HTTP-сервера");
    }
}

void HttpServer::register_handlers()
{
    // Главная страница (/)
    httpd_uri_t root =
    {
        .uri = "/",
        .method = HTTP_GET,
        .handler = [](httpd_req_t* req) -> esp_err_t
        {
            auto* self = static_cast<HttpServer*>(req->user_ctx);
            std::string html = "<html><body><h1>ESP32 LED Control</h1>"
                               "<p>LED STATE: " + std::string(self->m_led.get() ? "ON" : "OFF") + "</p>"
                               "<a href='/led/on'>LED ON</a><br>"
                               "<a href='/led/off'>LED OFF</a></body></html>";
            httpd_resp_sendstr(req, html.c_str());
            return ESP_OK;
        },
        .user_ctx = this
    };
    httpd_register_uri_handler(m_server, &root);

    // Вкл LED (/led/on)
    httpd_uri_t led_on =
    {
        .uri = "/led/on",
        .method = HTTP_GET,
        .handler = [](httpd_req_t* req) -> esp_err_t {
            auto* self = static_cast<HttpServer*>(req->user_ctx);
            self->m_led.set(true);
            httpd_resp_set_status(req, "302 Found");
            httpd_resp_set_hdr(req, "Location", "/");
            httpd_resp_send(req, nullptr, 0);
            return ESP_OK;
        },
        .user_ctx = this
    };
    httpd_register_uri_handler(m_server, &led_on);

    // Выкл LED (/led/off)
    httpd_uri_t led_off =
    {
        .uri = "/led/off",
        .method = HTTP_GET,
        .handler = [](httpd_req_t* req) -> esp_err_t {
            auto* self = static_cast<HttpServer*>(req->user_ctx);
            self->m_led.set(false);
            httpd_resp_set_status(req, "302 Found");
            httpd_resp_set_hdr(req, "Location", "/");
            httpd_resp_send(req, nullptr, 0);
            return ESP_OK;
        },
        .user_ctx = this
    };
    httpd_register_uri_handler(m_server, &led_off);
}