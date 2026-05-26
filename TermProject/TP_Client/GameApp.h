#pragma once

#include "NetworkClient.h"
#include "D2DRenderer.h"
#include "GameTimer.h"

#include <Windows.h>

class GameApp
{
public:
    int Run(HINSTANCE instance, int commandShow);

private:
    enum class GameState
    {
        StartScreen,
        Playing
    };

    static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    HRESULT InitializeWindow(HINSTANCE instance, int commandShow);
    void RunMessageLoop();
    void Update(float deltaTime);
    void Render();
    void StartGame();

    HWND hwnd_{};
    NetworkClient networkClient_;
    D2DRenderer renderer_;
    GameTimer timer_;
    GameState gameState_{ GameState::StartScreen };
    float totalTime_{};
    bool minimized_{};
};
