#pragma once

#include <string>
#include <functional>
#include <map>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
//#include "ServoControl.h"

class TelegramBot
{
public:
    using MessageHandler = std::function<void(const std::string& chat_id, const std::string& text, const std::string& from)>;

    TelegramBot(const std::string& tgToken);
    ~TelegramBot();

    void startPolling();
    void stopPolling();

    bool sendMessage(const std::string& chat_id, const std::string& text);
    bool sendPhoto(const std::string& chat_id, const uint8_t* data, size_t len);

    void registerCommand(const std::string& command, MessageHandler handler);
    void setDefaultHandler(MessageHandler handler);

    // Статические помощники для общих задач
    static std::string urlEncode(const std::string& value);
    static std::string formatMessage(const std::string& template_str, ...);

private:
    struct BotMessage
    {
        std::string chat_id;
        std::string text;
        std::string from;
        int64_t update_id;
    };

    // Основные методы
    void pollingTask();
    void processUpdates();
    bool parseUpdates(const std::string& json_response);
    void processMessage(const BotMessage& msg);

    // HTTP методы
    std::string httpGet(const std::string& url);
    std::string httpPost(const std::string& url, const std::string& data = "");
    std::string httpPostMultipart(const std::string& url, const std::map<std::string, std::string>& fields);

    // Обработчики команд
    void handleStart(const std::string& chat_id, const std::string& from);
    void handleHelp(const std::string& chat_id);
    void handleStatus(const std::string& chat_id);
    void handleServo(const std::string& chat_id, const std::string& command);

private:
    std::string m_token;
    bool m_running = false;
    int64_t m_last_update_id = 0;
    TaskHandle_t m_polling_task;

    std::map<std::string, MessageHandler> m_command_handlers;
    MessageHandler m_default_handler;

    QueueHandle_t m_message_queue;
};
