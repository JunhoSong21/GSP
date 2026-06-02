#pragma once

#include "NetworkClient.h"
#include "D2DRenderer.h"
#include "GameTimer.h"

#include <Windows.h>

class GameApp {
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

private:
    HWND                _hwnd{};
    NetworkClient       _networkClient;
    D2DRenderer         _renderer;
    GameTimer           _timer;
    GameState           _gameState{ GameState::StartScreen };

    float               _totalTime{};
    bool                _minimized{};

};
