#pragma once

#include "NetworkClient.h"
#include "D2DRenderer.h"
#include "GameTimer.h"

#include <Windows.h>
#include <string>

class GameApp {
public:
    int Run(HINSTANCE instance, int commandShow);

private:
    enum class GameState
    {
        StartScreen,
        Playing
    };

    enum class StartInputField
    {
        None,
        LoginId,
        Password
    };

    static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    HRESULT InitializeWindow(HINSTANCE instance, int commandShow);
    void Shutdown();
    void RunMessageLoop();
    void Update(float deltaTime);
    void Render();
    void StartGame();
    void HandleStartScreenCharacter(wchar_t character);
    void HandleStartScreenBackspace();
    void FocusNextStartInputField();
    bool TryGetStartScreenImagePoint(int mouseX, int mouseY, float& imageX, float& imageY) const;
    bool IsLoginIdFieldHit(int mouseX, int mouseY) const;
    bool IsPasswordFieldHit(int mouseX, int mouseY) const;
    bool IsSignUpButtonHit(int mouseX, int mouseY) const;

private:
    HWND                _hwnd{};
    NetworkClient       _networkClient;
    D2DRenderer         _renderer;
    GameTimer           _timer;
    GameState           _gameState{ GameState::StartScreen };
    StartInputField     _focusedStartInput{ StartInputField::None };
    std::wstring        _loginIdText;
    std::wstring        _passwordText;

    float               _totalTime{};
    bool                _minimized{};
    bool                _showSignUpScreen{};

};
