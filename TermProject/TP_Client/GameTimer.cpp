#include "GameTimer.h"

GameTimer::GameTimer()
{
    ::QueryPerformanceFrequency(&_frequency);
    Reset();
}

void GameTimer::Reset()
{
    ::QueryPerformanceCounter(&_previousTime);
}

float GameTimer::Tick()
{
    LARGE_INTEGER currentTime{};
    ::QueryPerformanceCounter(&currentTime);

    const auto elapsedCounts = currentTime.QuadPart - _previousTime.QuadPart;
    _previousTime = currentTime;

    return static_cast<float>(elapsedCounts) / static_cast<float>(_frequency.QuadPart);
}
