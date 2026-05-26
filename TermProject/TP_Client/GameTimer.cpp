#include "GameTimer.h"

GameTimer::GameTimer()
{
    ::QueryPerformanceFrequency(&frequency_);
    Reset();
}

void GameTimer::Reset()
{
    ::QueryPerformanceCounter(&previousTime_);
}

float GameTimer::Tick()
{
    LARGE_INTEGER currentTime{};
    ::QueryPerformanceCounter(&currentTime);

    const auto elapsedCounts = currentTime.QuadPart - previousTime_.QuadPart;
    previousTime_ = currentTime;

    return static_cast<float>(elapsedCounts) / static_cast<float>(frequency_.QuadPart);
}
