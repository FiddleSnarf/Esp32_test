#pragma once

#include "StepGenerator.h"

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
     * @param acc: Ускорение, град/с^2
     * @param dec: Замедление, град/с^2
     */
    void setTargetSpeed(double targetSpeed, double acc, double dec);

    /**
     * @brief Метод установки целевого положения (Управление по положению)
     * @param targetPos: Целевое положение, град
     * @param targetPos: Максимальная скорость, град/с
     * @param acc: Ускорение, град/с^2
     * @param dec: Замедление, град/с^2
     */
    void setTargetPosition(double targetPos, double maxSpeed, double acc, double dec);

    /**
     * @brief Метод для "плавного" останова
     * @param dec: Замедление, град/с^2
     */
    void softStop(double dec);

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
    double getMinSpeed() const;

    /**
     * @brief Метод для получения максимальной возможной скорости
     * @return Максимальная скорость, град/с
     */
    double getMaxSpeed() const;

    /**
     * @brief Метод для получения текущего положения
     * @return Текущее положение, град
     */
    double getCurrentPosition() const;

    /**
     * @brief Метод для получения текущей скорости
     * @return Текущая скорость, град/с
     */
    double getCurrentSpeed() const;

private:
    InitParams m_initParams;    // Параметры инициализации
    StepGenerator m_stepGen;    // Генератор импульсов step

    double m_clbK = 1.;         // Калибровочный коэффициент для перевода град в шаги и обратно
};