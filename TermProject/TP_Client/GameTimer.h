#pragma once

#include <Windows.h>

class GameTimer
{
public:
    GameTimer();

    void Reset();
    float Tick();

private:
    LARGE_INTEGER frequency_{};
    LARGE_INTEGER previousTime_{};
};
