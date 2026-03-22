#include "StepMotorController.h"

namespace
{
    const char* LOG = "StepMotorController";    // Канал лога
    const double ANGLE_ON_STEP = 1.8;           // Обычно для ШД 1.8 градусов на шаг
}

StepMotorController::StepMotorController(const InitParams& params):
    m_initParams(params),
    m_stepGen(params.stepPin),
    m_clbK(ANGLE_ON_STEP / static_cast<double>(params.stepMode))
{
    // TODO Инициализация выходов
}

StepMotorController::~StepMotorController()
{
}

void StepMotorController::setEnabled(bool state)
{
    // TODO
}

void StepMotorController::setTargetSpeed(double targetSpeed, double acc, double dec)
{
    // TODO
}

void StepMotorController::setTargetPosition(double targetPos, double maxSpeed, double acc, double dec)
{
    // TODO
}

void StepMotorController::softStop(double dec)
{
    // TODO
}

void StepMotorController::hardStop()
{
    m_stepGen.stop();
}

void StepMotorController::resetCurrentPosition(double newPosition)
{
    // TODO
}

double StepMotorController::getMinSpeed() const
{
    return m_stepGen.getMinFreq() * m_clbK;
}

double StepMotorController::getMaxSpeed() const
{
    return m_stepGen.getMaxFreq() * m_clbK;
}

double StepMotorController::getCurrentPosition() const
{
    // TODO
    return 0.;
}

double StepMotorController::getCurrentSpeed() const
{
    // TODO
    return 0.;
}