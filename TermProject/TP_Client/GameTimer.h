#pragma once

#include <Windows.h>

class GameTimer
{
public:
    GameTimer();

    void Reset();
    float Tick();

private:
    LARGE_INTEGER _frequency{};
    LARGE_INTEGER _previousTime{};
};
