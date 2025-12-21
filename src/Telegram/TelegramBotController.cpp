#include "TelegramBotController.h"
#include <esp_log.h>
#include <esp_http_client.h>
#include <esp_timer.h>
#include <esp_crt_bundle.h>
#include <cJSON.h>
#include <sstream>
#include <iomanip>
#include <vector>

const char* TG_LOG_TAG = "TELEGRAM";
const int QUEUE_SIZE = 10;
const std::string tgUrl = "https://api.telegram.org/bot";

// Статический метод для кодирования URL
std::string TelegramBot::urlEncode(const std::string& value)
{
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : value)
    {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            escaped << c;
            continue;
        }
        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char)c);
        escaped << std::nouppercase;
    }

    return escaped.str();
}

TelegramBot::TelegramBot(const std::string& tgToken):
    m_token(tgToken),
    m_running(false),
    m_last_update_id(0),
    m_polling_task(nullptr)
{
    ESP_LOGI(TG_LOG_TAG, "TelegramBot init by token: %s", tgToken.substr(0, 10).c_str());

    // Создаем очередь для сообщений
    m_message_queue = xQueueCreate(QUEUE_SIZE, sizeof(BotMessage*));

    // Регистрируем стандартные команды
    registerCommand("/start", [this](const std::string& chat_id,
                                     const std::string& text,
                                     const std::string& from)
    {
        handleStart(chat_id, from);
    });

    registerCommand("/help", [this](const std::string& chat_id,
                                    const std::string& text,
                                    const std::string& from)
    {
        handleHelp(chat_id);
    });

    registerCommand("/status", [this](const std::string& chat_id,
                                      const std::string& text,
                                      const std::string& from)
    {
        handleStatus(chat_id);
    });

    registerCommand("/servo", [this](const std::string& chat_id,
                                     const std::string& text,
                                     const std::string& from)
    {
        handleServo(chat_id, text);
    });

    // Обработчик по умолчанию
    setDefaultHandler([this](const std::string& chat_id,
                            const std::string& text,
                            const std::string& from)
    {
        const std::string response = "Unknown command: " + text + "\nUse /help for get command list";
        sendMessage(chat_id, response);
    });
}

TelegramBot::~TelegramBot()
{
    stopPolling();
    if (m_message_queue)
        vQueueDelete(m_message_queue);
}

void TelegramBot::startPolling()
{
    if (m_running)
    {
        ESP_LOGW(TG_LOG_TAG, "Polling has already started");
        return;
    }

    m_running = true;

    //TODO Разобраться
    xTaskCreate([](void* arg)
        {
            TelegramBot* bot = static_cast<TelegramBot*>(arg);
            bot->pollingTask();
        },
        "tg_polling", 8192, this, 5, &m_polling_task);

    ESP_LOGI(TG_LOG_TAG, "Polling started");
}

void TelegramBot::stopPolling()
{
    m_running = false;
    if (m_polling_task)
    {
        vTaskDelete(m_polling_task);
        m_polling_task = nullptr;
    }
    ESP_LOGI(TG_LOG_TAG, "Polling stopped");
}

void TelegramBot::pollingTask()
{
    ESP_LOGI(TG_LOG_TAG, "Polling task started");

    while (m_running)
    {
        processUpdates();

        // Ждем 1 секунду между опросами
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    ESP_LOGI(TG_LOG_TAG, "Polling task completed");
    vTaskDelete(NULL);
}

void TelegramBot::processUpdates()
{
    std::string url = tgUrl + m_token + "/getUpdates?timeout=10";
    if (m_last_update_id > 0)
        url += "&offset=" + std::to_string(m_last_update_id + 1);

    std::string response = httpGet(url);

    if (!response.empty())
        parseUpdates(response);

    // Обработка сообщений из очереди
    BotMessage* msg_ptr = nullptr;
    while (xQueueReceive(m_message_queue, &msg_ptr, 0) == pdTRUE)
    {
        if (msg_ptr)
        {
            processMessage(*msg_ptr);
            delete msg_ptr;
        }
    }
}

bool TelegramBot::parseUpdates(const std::string& json_response)
{
    cJSON* root = cJSON_Parse(json_response.c_str());
    if (!root)
    {
        ESP_LOGE(TG_LOG_TAG, "Parse JSON error");
        return false;
    }

    cJSON* ok = cJSON_GetObjectItem(root, "ok");
    if (!cJSON_IsTrue(ok))
    {
        cJSON_Delete(root);
        return false;
    }

    cJSON* result = cJSON_GetObjectItem(root, "result");
    if (!cJSON_IsArray(result))
    {
        cJSON_Delete(root);
        return true;
    }

    int array_size = cJSON_GetArraySize(result);
    for (int i = 0; i < array_size; i++)
    {
        cJSON* update = cJSON_GetArrayItem(result, i);

        // Получаем update_id
        cJSON* update_id = cJSON_GetObjectItem(update, "update_id");
        if (cJSON_IsNumber(update_id)) {
            m_last_update_id = update_id->valueint;
        }

        // Получаем сообщение
        cJSON* message = cJSON_GetObjectItem(update, "message");
        if (message)
        {
            BotMessage* bot_msg = new BotMessage();

            // Chat ID
            cJSON* chat = cJSON_GetObjectItem(message, "chat");
            if (chat)
            {
                cJSON* chat_id = cJSON_GetObjectItem(chat, "id");
                if (cJSON_IsNumber(chat_id))
                    bot_msg->chat_id = std::to_string(chat_id->valueint);
            }

            // Текст сообщения
            cJSON* text = cJSON_GetObjectItem(message, "text");
            if (cJSON_IsString(text))
                bot_msg->text = text->valuestring;
            
            // Отправитель
            cJSON* from = cJSON_GetObjectItem(message, "from");
            if (from)
            {
                cJSON* first_name = cJSON_GetObjectItem(from, "first_name");
                if (cJSON_IsString(first_name))
                    bot_msg->from = first_name->valuestring;
            }

            bot_msg->update_id = m_last_update_id;

            // Добавляем в очередь
            if (xQueueSend(m_message_queue, &bot_msg, portMAX_DELAY) != pdTRUE)
            {
                delete bot_msg;
                ESP_LOGW(TG_LOG_TAG, "Queue is full");
            }
        }
    }

    cJSON_Delete(root);
    return true;
}

void TelegramBot::processMessage(const BotMessage& msg)
{
    if (msg.text.empty())
        return;

    ESP_LOGI(TG_LOG_TAG, "Message from %s: %s", msg.from.c_str(), msg.text.c_str());

    // Ищем обработчик команды
    auto it = m_command_handlers.find(msg.text);
    if (it != m_command_handlers.end())
        it->second(msg.chat_id, msg.text, msg.from);
    else
    {
        // Проверяем команды с параметрами (/servo 90)
        size_t space_pos = msg.text.find(' ');
        if (space_pos != std::string::npos)
        {
            std::string command = msg.text.substr(0, space_pos);
            auto cmd_it = m_command_handlers.find(command);
            if (cmd_it != m_command_handlers.end())
            {
                cmd_it->second(msg.chat_id, msg.text, msg.from);
                return;
            }
        }

        // Используем обработчик по умолчанию
        if (m_default_handler)
            m_default_handler(msg.chat_id, msg.text, msg.from);
    }
}

bool TelegramBot::sendMessage(const std::string& chat_id, const std::string& text)
{
    std::string url = tgUrl + m_token + "/sendMessage";
    std::string data = "chat_id=" + chat_id + "&text=" + urlEncode(text) + "&parse_mode=HTML";

    std::string response = httpPost(url, data);

    cJSON* root = cJSON_Parse(response.c_str());
    if (!root)
        return false;

    cJSON* ok = cJSON_GetObjectItem(root, "ok");
    bool success = cJSON_IsTrue(ok);

    cJSON_Delete(root);
    return success;
}

std::string TelegramBot::httpGet(const std::string& url)
{
    esp_http_client_config_t config;
    config.url = url.c_str();
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = 10000;
    config.crt_bundle_attach = esp_crt_bundle_attach;
    config.disable_auto_redirect = false;

    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Устанавливаем заголовки
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Accept", "application/json");

    esp_err_t err = esp_http_client_perform(client);

    std::string response;
    if (err == ESP_OK)
    {
        int content_len = esp_http_client_get_content_length(client);
        if (content_len > 0)
        {
            response.resize(content_len);
            esp_http_client_read(client, &response[0], content_len);
        }
    } else
        ESP_LOGE(TG_LOG_TAG, "HTTP GET ошибка: %s", esp_err_to_name(err));

    esp_http_client_cleanup(client);
    return response;
}

std::string TelegramBot::httpPost(const std::string& url, const std::string& data)
{
    esp_http_client_config_t config =
    {
        .url = url.c_str(),
        .method = HTTP_METHOD_POST,
        .timeout_ms = 10000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Устанавливаем заголовки
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
    esp_http_client_set_header(client, "Accept", "application/json");

    if (!data.empty())
        esp_http_client_set_post_field(client, data.c_str(), data.length());

    esp_err_t err = esp_http_client_perform(client);

    std::string response;
    if (err == ESP_OK)
    {
        int content_len = esp_http_client_get_content_length(client);
        if (content_len > 0)
        {
            response.resize(content_len);
            esp_http_client_read(client, &response[0], content_len);
        }
    } else
        ESP_LOGE(TG_LOG_TAG, "HTTP POST ошибка: %s", esp_err_to_name(err));

    esp_http_client_cleanup(client);
    return response;
}

void TelegramBot::handleStart(const std::string& chat_id, const std::string& from)
{
    std::string welcome = "👋 Привет, " + from + "!\n\n";
    welcome += "🤖 Я бот для управления ESP32 с сервоприводом\n\n";
    welcome += "📋 Доступные команды:\n";
    welcome += "• /help - Справка по командам\n";
    welcome += "• /status - Статус системы\n";
    welcome += "• /servo <угол> - Установить угол сервопривода (0-180)\n";
    welcome += "• /servo sweep - Плавное перемещение сервопривода\n";
    welcome += "• /servo center - Центрировать сервопривод\n";

    sendMessage(chat_id, welcome);
}

void TelegramBot::handleHelp(const std::string& chat_id)
{
    std::string help = "🛠️ Справка по командам:\n\n";
    help += "🎯 Управление сервоприводом:\n";
    help += "<code>/servo 90</code> - установить угол 90°\n";
    help += "<code>/servo sweep</code> - плавное движение\n";
    help += "<code>/servo center</code> - центрировать\n\n";
    help += "📊 Системные команды:\n";
    help += "<code>/status</code> - статус ESP32\n";
    help += "<code>/help</code> - эта справка";

    sendMessage(chat_id, help);
}

void TelegramBot::handleStatus(const std::string& chat_id)
{
    std::string status = "📊 Статус системы:\n\n";

    // Информация о памяти
    status += "💾 Память:\n";
    status += "• Свободно: " + std::to_string(esp_get_free_heap_size()) + " байт\n";
    status += "• Минимум: " + std::to_string(esp_get_minimum_free_heap_size()) + " байт\n\n";

    // Информация о WiFi (если доступна)
    status += "📡 Сеть:\n";

    // Информация о сервоприводе
    status += "⚙️ Сервопривод:\n";
    // Добавьте методы для получения статуса сервопривода из m_servo

    sendMessage(chat_id, status);
}

void TelegramBot::handleServo(const std::string& chat_id, const std::string& command)
{
    std::string response;
    size_t space_pos = command.find(' ');

    if (space_pos == std::string::npos)
    {
        response = "⚠️ Используйте: /servo <угол> или /servo <команда>\n";
        response += "Примеры:\n";
        response += "<code>/servo 90</code>\n";
        response += "<code>/servo sweep</code>\n";
        response += "<code>/servo center</code>";
        sendMessage(chat_id, response);
        return;
    }

    std::string param = command.substr(space_pos + 1);

    if (param == "sweep")
    {
        // Плавное движение сервопривода
        //m_servo.sweep();
        response = "🔁 Сервопривод выполняет плавное движение";
    } else if (param == "center")
    {
        // Центрирование
        //m_servo.setAngle(90);
        response = "🎯 Сервопривод центрирован (90°)";
    } else
    {
        // Попытка распарсить угол
        //try
        //{
        //    const int angle = std::stoi(param);
        //    if (angle >= 0 && angle <= 180)
        //    {
        //        //m_servo.setAngle(angle);
        //        response = "✅ Сервопривод установлен на " + param + "°";
        //    } else
        //    {
        //        response = "❌ Угол должен быть от 0 до 180 градусов";
        //    }
        //} catch (...)
        //{
        //    response = "❌ Неверный параметр: " + param;
        //}
    }

    sendMessage(chat_id, response);
}

void TelegramBot::registerCommand(const std::string& command, MessageHandler handler)
{
    m_command_handlers[command] = handler;
    ESP_LOGI(TG_LOG_TAG, "Command registered: %s", command.c_str());
}

void TelegramBot::setDefaultHandler(MessageHandler handler)
{
    m_default_handler = handler;
}
