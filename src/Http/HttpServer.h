#pragma once

#include <esp_http_server.h>
#include "./Helpers/RgbLedController.h"

class HttpServer
{
public:
    HttpServer(RgbLedControllerPtr led);

    void start();

private:
    void register_handlers();

private:
    RgbLedControllerPtr m_led;
    httpd_handle_t m_server;
};