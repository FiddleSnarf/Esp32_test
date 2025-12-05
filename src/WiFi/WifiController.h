#pragma once

// Класс для управления Wi-Fi
class WiFiManager
{
public:
    WiFiManager(const char* ssid, const char* password);

    void connect();

private:
    const char* m_ssid;
    const char* m_password;
};