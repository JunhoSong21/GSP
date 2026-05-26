#include "GameApp.h"

#include <string>

constexpr wchar_t WINDOW_CLASS_NAME[] = L"TPClientWindowClass";
constexpr wchar_t WINDOW_TITLE[] = L"TP Client";
constexpr int INITIAL_WIDTH = 1280;
constexpr int INITIAL_HEIGHT = 720;
constexpr char SERVER_ADDRESS[] = "127.0.0.1";
constexpr char DEFAULT_PLAYER_NAME[] = "Player";

int GameApp::Run(HINSTANCE instance, int commandShow)
{
    const HRESULT comResult = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool shouldUninitializeCom = SUCCEEDED(comResult);

    if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE)
        return -1;

    if (FAILED(InitializeWindow(instance, commandShow)))
    {
        if (shouldUninitializeCom)
            ::CoUninitialize();

        return -1;
    }

    RunMessageLoop();

    if (shouldUninitializeCom)
        ::CoUninitialize();

    return 0;
}

HRESULT GameApp::InitializeWindow(HINSTANCE instance, int commandShow)
{
    WNDCLASSEX windowClass{};
    windowClass.cbSize          = sizeof(WNDCLASSEX);
    windowClass.style           = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc     = StaticWindowProc;
    windowClass.hInstance       = instance;
    windowClass.hCursor         = ::LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground   = nullptr;
    windowClass.lpszClassName   = WINDOW_CLASS_NAME;

    if (!::RegisterClassEx(&windowClass))
        return HRESULT_FROM_WIN32(::GetLastError());

    RECT windowRect{ 0, 0, INITIAL_WIDTH, INITIAL_HEIGHT };
    ::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    hwnd_ = ::CreateWindowEx(0, WINDOW_CLASS_NAME, WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
        nullptr, nullptr, instance, this);

    if (!hwnd_)
        return HRESULT_FROM_WIN32(::GetLastError());

    HRESULT hr = renderer_.Initialize(hwnd_);
    if (FAILED(hr))
        return hr;

    ::ShowWindow(hwnd_, commandShow);
    ::UpdateWindow(hwnd_);
    timer_.Reset();

    return S_OK;
}

void GameApp::RunMessageLoop()
{
    MSG message{};

    while (message.message != WM_QUIT)
    {
        if (::PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&message);
            ::DispatchMessage(&message);
            continue;
        }

        if (minimized_)
        {
            ::WaitMessage();
            timer_.Reset();
            continue;
        }

        const float deltaTime = timer_.Tick();
        Update(deltaTime);
        Render();
    }
}

void GameApp::Update(float deltaTime)
{
    networkClient_.Pump();

    if (gameState_ != GameState::Playing)
        return;

    totalTime_ += deltaTime;
}

void GameApp::Render()
{
    renderer_.Render(gameState_ == GameState::Playing, totalTime_);
}

void GameApp::StartGame()
{
    gameState_ = GameState::Playing;
    totalTime_ = 0.0f;
    timer_.Reset();
    networkClient_.Connect(SERVER_ADDRESS, PORT, DEFAULT_PLAYER_NAME);
    ::InvalidateRect(hwnd_, nullptr, FALSE);
}

LRESULT CALLBACK GameApp::StaticWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    GameApp* app = nullptr;

    if (message == WM_NCCREATE)
    {
        const auto createStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
        app = static_cast<GameApp*>(createStruct->lpCreateParams);
        ::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    else
        app = reinterpret_cast<GameApp*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if (app)
        return app->WindowProc(hwnd, message, wParam, lParam);

    return ::DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT GameApp::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
    {
        const UINT width = LOWORD(lParam);
        const UINT height = HIWORD(lParam);
        minimized_ = (wParam == SIZE_MINIMIZED);

        if (!minimized_)
            renderer_.Resize(width, height);

        return 0;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT paintStruct{};
        ::BeginPaint(hwnd, &paintStruct);
        ::EndPaint(hwnd, &paintStruct);
        Render();

        return 0;
    }
    case WM_DISPLAYCHANGE:
        ::InvalidateRect(hwnd, nullptr, FALSE);

        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_RETURN && gameState_ == GameState::StartScreen)
        {
            StartGame();

            return 0;
        }

        break;
    case WM_DESTROY:
        networkClient_.Disconnect();
        ::PostQuitMessage(0);

        return 0;
    default:
        break;
    }

    return ::DefWindowProc(hwnd, message, wParam, lParam);
}
