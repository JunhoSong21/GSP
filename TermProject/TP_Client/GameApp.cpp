#include "GameApp.h"

#include <algorithm>
#include <windowsx.h>

constexpr wchar_t   WINDOW_CLASS_NAME[]     = L"TPClientWindowClass";
constexpr wchar_t   WINDOW_TITLE[]          = L"TP Client";
constexpr int       INITIAL_WIDTH           = 1280;
constexpr int       INITIAL_HEIGHT          = 720;
constexpr char      SERVER_ADDRESS[]        = "127.0.0.1";
constexpr char      DEFAULT_PLAYER_NAME[]   = "Player";
constexpr float     START_SCREEN_WIDTH      = 1672.0f;
constexpr float     START_SCREEN_HEIGHT     = 941.0f;
constexpr float     SIGN_UP_BUTTON_LEFT     = 660.0f;
constexpr float     SIGN_UP_BUTTON_TOP      = 735.0f;
constexpr float     SIGN_UP_BUTTON_RIGHT    = 1015.0f;
constexpr float     SIGN_UP_BUTTON_BOTTOM   = 807.0f;
constexpr float     LOGIN_ID_FIELD_LEFT     = 631.0f;
constexpr float     LOGIN_ID_FIELD_TOP      = 499.0f;
constexpr float     LOGIN_ID_FIELD_RIGHT    = 1030.0f;
constexpr float     LOGIN_ID_FIELD_BOTTOM   = 554.0f;
constexpr float     PASSWORD_FIELD_LEFT     = 631.0f;
constexpr float     PASSWORD_FIELD_TOP      = 570.0f;
constexpr float     PASSWORD_FIELD_RIGHT    = 1030.0f;
constexpr float     PASSWORD_FIELD_BOTTOM   = 626.0f;
constexpr size_t    MAX_LOGIN_ID_LENGTH     = 20;
constexpr size_t    MAX_PASSWORD_LENGTH     = 32;

static std::string ToUtf8(const std::wstring& text)
{
    if (text.empty())
        return {};

    const int requiredLength = ::WideCharToMultiByte(
        CP_UTF8,
        0,
        text.c_str(),
        static_cast<int>(text.length()),
        nullptr,
        0,
        nullptr,
        nullptr);

    if (0 >= requiredLength)
        return {};

    std::string result(requiredLength, '\0');
    ::WideCharToMultiByte(
        CP_UTF8,
        0,
        text.c_str(),
        static_cast<int>(text.length()),
        result.data(),
        requiredLength,
        nullptr,
        nullptr);

    return result;
}

int GameApp::Run(HINSTANCE instance, int commandShow)
{
    const HRESULT comResult = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool shouldUninitializeCom = SUCCEEDED(comResult);

    if (FAILED(comResult) && RPC_E_CHANGED_MODE != comResult)
        return -1;

    if (FAILED(InitializeWindow(instance, commandShow)))
    {
        Shutdown();

        if (shouldUninitializeCom)
            ::CoUninitialize();

        return -1;
    }

    RunMessageLoop();
    Shutdown();

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

    _hwnd = ::CreateWindowEx(0, WINDOW_CLASS_NAME, WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
        nullptr, nullptr, instance, this);

    if (!_hwnd)
        return HRESULT_FROM_WIN32(::GetLastError());

    HRESULT hr = _renderer.Initialize(_hwnd);
    if (FAILED(hr))
        return hr;

    ::ShowWindow(_hwnd, commandShow);
    ::UpdateWindow(_hwnd);
    _timer.Reset();

    return S_OK;
}

void GameApp::Shutdown()
{
    _networkClient.Disconnect(false);
    _renderer.Shutdown();
}

void GameApp::RunMessageLoop()
{
    MSG message{};

    while (WM_QUIT != message.message)
    {
        if (::PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) {
            ::TranslateMessage(&message);
            ::DispatchMessage(&message);
            continue;
        }

        if (_minimized)
        {
            ::WaitMessage();
            _timer.Reset();
            continue;
        }

        const float deltaTime = _timer.Tick();
        Update(deltaTime);
        Render();
    }
}

void GameApp::Update(float deltaTime)
{
    _networkClient.Pump();

    if (GameState::Playing != _gameState)
        return;

    _totalTime += deltaTime;
}

void GameApp::Render()
{
    _renderer.Render(
        GameState::Playing == _gameState,
        _showSignUpScreen,
        _loginIdText,
        _passwordText,
        StartInputField::LoginId == _focusedStartInput,
        StartInputField::Password == _focusedStartInput,
        _totalTime);
}

void GameApp::StartGame()
{
    _gameState = GameState::Playing;
    _showSignUpScreen = false;
    _focusedStartInput = StartInputField::None;
    _totalTime = 0.0f;
    _timer.Reset();

    const std::string playerName = _loginIdText.empty() ? DEFAULT_PLAYER_NAME : ToUtf8(_loginIdText);
    _networkClient.Connect(SERVER_ADDRESS, PORT, playerName);
    ::InvalidateRect(_hwnd, nullptr, FALSE);
}

void GameApp::HandleStartScreenCharacter(wchar_t character)
{
    if (GameState::StartScreen != _gameState || _showSignUpScreen)
        return;

    if (StartInputField::LoginId == _focusedStartInput)
    {
        if (MAX_LOGIN_ID_LENGTH > _loginIdText.length())
            _loginIdText.push_back(character);

        ::InvalidateRect(_hwnd, nullptr, FALSE);
        return;
    }

    if (StartInputField::Password == _focusedStartInput)
    {
        if (MAX_PASSWORD_LENGTH > _passwordText.length())
            _passwordText.push_back(character);

        ::InvalidateRect(_hwnd, nullptr, FALSE);
        return;
    }
}

void GameApp::HandleStartScreenBackspace()
{
    if (GameState::StartScreen != _gameState || _showSignUpScreen)
        return;

    if (StartInputField::LoginId == _focusedStartInput && false == _loginIdText.empty())
    {
        _loginIdText.pop_back();
        ::InvalidateRect(_hwnd, nullptr, FALSE);
        return;
    }

    if (StartInputField::Password == _focusedStartInput && false == _passwordText.empty())
    {
        _passwordText.pop_back();
        ::InvalidateRect(_hwnd, nullptr, FALSE);
    }
}

void GameApp::FocusNextStartInputField()
{
    if (StartInputField::LoginId == _focusedStartInput)
        _focusedStartInput = StartInputField::Password;
    else
        _focusedStartInput = StartInputField::LoginId;

    ::InvalidateRect(_hwnd, nullptr, FALSE);
}

bool GameApp::TryGetStartScreenImagePoint(int mouseX, int mouseY, float& imageX, float& imageY) const
{
    RECT clientRect{};
    ::GetClientRect(_hwnd, &clientRect);

    const float clientWidth = static_cast<float>(clientRect.right - clientRect.left);
    const float clientHeight = static_cast<float>(clientRect.bottom - clientRect.top);
    const float scale = std::min(clientWidth / START_SCREEN_WIDTH, clientHeight / START_SCREEN_HEIGHT);
    const float imageWidth = START_SCREEN_WIDTH * scale;
    const float imageHeight = START_SCREEN_HEIGHT * scale;
    const float imageLeft = (clientWidth - imageWidth) * 0.5f;
    const float imageTop = (clientHeight - imageHeight) * 0.5f;
    imageX = (static_cast<float>(mouseX) - imageLeft) / scale;
    imageY = (static_cast<float>(mouseY) - imageTop) / scale;

    return 0.0f <= imageX
        && START_SCREEN_WIDTH >= imageX
        && 0.0f <= imageY
        && START_SCREEN_HEIGHT >= imageY;
}

bool GameApp::IsLoginIdFieldHit(int mouseX, int mouseY) const
{
    float imageX = 0.0f;
    float imageY = 0.0f;

    if (!TryGetStartScreenImagePoint(mouseX, mouseY, imageX, imageY))
        return false;

    return LOGIN_ID_FIELD_LEFT <= imageX
        && LOGIN_ID_FIELD_RIGHT >= imageX
        && LOGIN_ID_FIELD_TOP <= imageY
        && LOGIN_ID_FIELD_BOTTOM >= imageY;
}

bool GameApp::IsPasswordFieldHit(int mouseX, int mouseY) const
{
    float imageX = 0.0f;
    float imageY = 0.0f;

    if (!TryGetStartScreenImagePoint(mouseX, mouseY, imageX, imageY))
        return false;

    return PASSWORD_FIELD_LEFT <= imageX
        && PASSWORD_FIELD_RIGHT >= imageX
        && PASSWORD_FIELD_TOP <= imageY
        && PASSWORD_FIELD_BOTTOM >= imageY;
}

bool GameApp::IsSignUpButtonHit(int mouseX, int mouseY) const
{
    float imageX = 0.0f;
    float imageY = 0.0f;

    if (!TryGetStartScreenImagePoint(mouseX, mouseY, imageX, imageY))
        return false;

    return SIGN_UP_BUTTON_LEFT <= imageX
        && SIGN_UP_BUTTON_RIGHT >= imageX
        && SIGN_UP_BUTTON_TOP <= imageY
        && SIGN_UP_BUTTON_BOTTOM >= imageY;
}

LRESULT CALLBACK GameApp::StaticWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    GameApp* app = nullptr;

    if (WM_NCCREATE == message)
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
        _minimized = (SIZE_MINIMIZED == wParam);

        if (!_minimized)
            _renderer.Resize(width, height);

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
        if (VK_BACK == wParam)
        {
            HandleStartScreenBackspace();
            return 0;
        }

        if (VK_TAB == wParam && GameState::StartScreen == _gameState && !_showSignUpScreen)
        {
            FocusNextStartInputField();
            return 0;
        }

        if (VK_ESCAPE == wParam && GameState::StartScreen == _gameState && _showSignUpScreen)
        {
            _showSignUpScreen = false;
            ::InvalidateRect(hwnd, nullptr, FALSE);

            return 0;
        }

        if (VK_RETURN == wParam && GameState::StartScreen == _gameState && !_showSignUpScreen)
        {
            StartGame();

            return 0;
        }

        break;
    case WM_LBUTTONDOWN:
        if (GameState::StartScreen == _gameState && !_showSignUpScreen)
        {
            const int mouseX = GET_X_LPARAM(lParam);
            const int mouseY = GET_Y_LPARAM(lParam);

            if (IsLoginIdFieldHit(mouseX, mouseY))
            {
                _focusedStartInput = StartInputField::LoginId;
                ::InvalidateRect(hwnd, nullptr, FALSE);

                return 0;
            }

            if (IsPasswordFieldHit(mouseX, mouseY))
            {
                _focusedStartInput = StartInputField::Password;
                ::InvalidateRect(hwnd, nullptr, FALSE);

                return 0;
            }

            if (IsSignUpButtonHit(mouseX, mouseY))
            {
                _focusedStartInput = StartInputField::None;
                _showSignUpScreen = true;
                ::InvalidateRect(hwnd, nullptr, FALSE);

                return 0;
            }

            _focusedStartInput = StartInputField::None;
            ::InvalidateRect(hwnd, nullptr, FALSE);

            return 0;
        }

        break;
    case WM_CHAR:
    {
        const wchar_t character = static_cast<wchar_t>(wParam);

        if (0x20 <= character && 0x7F != character)
        {
            HandleStartScreenCharacter(character);
            return 0;
        }

        break;
    }
    case WM_DESTROY:
        _networkClient.Disconnect();
        ::PostQuitMessage(0);

        return 0;
    default:
        break;
    }

    return ::DefWindowProc(hwnd, message, wParam, lParam);
}
