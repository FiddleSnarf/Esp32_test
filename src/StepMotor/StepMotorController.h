#pragma once

#include "StepGenerator.h"
#include "esp_timer.h"

class StepMotorController
{
public:
    enum class EnStepMode : int
    {
        en1_1 = 1,   // Полный шаг
        en1_2 = 2,   // Микрошаг 1 к 2
        en1_4 = 4,   // Микрошаг 1 к 4
        en1_8 = 8,   // Микрошаг 1 к 8
        en1_16 = 16, // Микрошаг 1 к 16
        en1_32 = 32, // Микрошаг 1 к 32
    };

    struct InitParams
    {
        gpio_num_t enPin = GPIO_NUM_NC;             // Номер пина ENABLE
        gpio_num_t dirPin = GPIO_NUM_NC;            // Номер пина DIR
        gpio_num_t stepPin = GPIO_NUM_NC;           // Номер пина STEP
        EnStepMode stepMode = EnStepMode::en1_1;    // Режим микрошага

        bool enableInverse = false;                 // Инверсия ENABLE (Если задана то низкий уровень включает драйвер мотора)
        bool directionInverse = false;              // Инверсия DIR
    };

    /**
     * @brief Конструктор
     * @param params: Параметры инициализации
     */
    StepMotorController(const InitParams& params);
    ~StepMotorController();

    /**
     * @brief Метод для вкл/выкл драйвера
     * @param state: Состояние
     */
    void setEnabled(bool state);

    /**
     * @brief Метод установки целевой скорости (Управление по скорости)
     * @param targetSpeed: Целевая скорость, град/с
     * @param acc: Ускорение, град/с²
     * @param dec: Замедление, град/с²
     */
    void setTargetSpeed(float targetSpeed, float acc, float dec);

    /**
     * @brief Метод установки целевого положения (Управление по положению)
     * @param targetPos: Целевое положение, град
     * @param targetSpeed: Целевая скорость, град/с
     * @param acc: Ускорение, град/с²
     * @param dec: Замедление, град/с²
     */
    void setTargetPosition(double targetPos, float targetSpeed, float acc, float dec);

    /**
     * @brief Метод для "плавного" останова
     */
    void softStop();

    /**
     * @brief Метод для "мгновенного" останова
     */
    void hardStop();

    /**
     * @brief Метод для сброса текущего положения
     * @param newPosition: Новое текущее положение, град
     */
    void resetCurrentPosition(double newPosition = 0.);

    /**
     * @brief Метод для получения минимальной возможной скорости
     * @return Минимальная скорость, град/с
     */
    float getMinSpeed() const;

    /**
     * @brief Метод для получения максимальной возможной скорости
     * @return Максимальная скорость, град/с
     */
    float getMaxSpeed() const;

    /**
     * @brief Метод для получения текущего положения
     * @return Текущее положение, град
     */
    double getCurrentPosition() const;

    /**
     * @brief Метод для получения текущей скорости
     * @return Текущая скорость, град/с
     */
    float getCurrentSpeed() const;

private:
    enum class EnControlMode
    {
        enNone,             // Ничего
        enSpeedControl,     // Управление по скорости
        enPositionControl   // Управление по положению
    };

    enum class EnState
    {
        enIdle,           // Ожидание
        enAccelerating,   // Разгон
        enConstantSpeed,  // Постоянная скорость
        enDecelerating    // Торможение
    };

    struct MotionProfile
    {
        EnControlMode controlMode = EnControlMode::enNone;  // Режим управления
        bool moveDirection = false;                         // Направление вращения
        uint64_t targetPos = 0;                             // шаг (Для режима управления по положению, в режиме управления по скорости игнорируется)
        uint32_t targetSpeed = 0;                           // шаг/с
        uint32_t acceleration = 0;                          // шаг/с²
        uint32_t deceleration = 0;                          // шаг/с²
    };

    /* Перевод из град в шаги */
    uint32_t angleToSteps(float angle) const;
    uint64_t angleToSteps(double angle) const;

    /* Перевод из шагов в град */
    float stepsToAngle(uint32_t angle) const;
    double stepsToAngle(uint64_t angle) const;

    /* Установка направления вращения */
    void setDirection(bool dirState);

    /* Обновление профиля движения */
    void updateMotion();

private:
    InitParams m_initParams;                                    // Параметры инициализации
    StepGenerator m_stepGen;                                    // Генератор импульсов step
    const float m_clbK = 1.;                                    // Калибровочный коэффициент для перевода град в шаги и обратно

    EnState m_state = EnState::enIdle;                          // Текущее состояние
    MotionProfile m_moveProfile;                                // Текущий профиль движения
    uint32_t m_currentSpeed = 0;                                // Текущая скорость, шаг/с

    uint64_t m_lastUpdateTick = 0;                              // Метка времени для расчета состояния системы, мс
};