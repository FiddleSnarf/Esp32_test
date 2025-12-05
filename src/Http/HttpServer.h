#pragma once

#include <esp_http_server.h>
#include "./Helpers/LedController.h"

class HttpServer
{
public:
    HttpServer(LedController& led);

    void start();

private:
    void register_handlers();

private:
    LedController& m_led;
    httpd_handle_t m_server;
};