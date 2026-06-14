#include "D2DRenderer.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <system_error>

static constexpr wchar_t START_SCREEN_FILE_NAME[] = L"StartScreen.png";
static constexpr wchar_t SIGN_UP_SCREEN_FILE_NAME[] = L"SignUpScreen.png";
static constexpr float START_SCREEN_WIDTH = 1672.0f;
static constexpr float START_SCREEN_HEIGHT = 941.0f;
static constexpr float LOGIN_ID_TEXT_LEFT = 694.0f;
static constexpr float LOGIN_ID_TEXT_TOP = 504.0f;
static constexpr float LOGIN_ID_TEXT_RIGHT = 1014.0f;
static constexpr float LOGIN_ID_TEXT_BOTTOM = 552.0f;
static constexpr float PASSWORD_TEXT_LEFT = 694.0f;
static constexpr float PASSWORD_TEXT_TOP = 576.0f;
static constexpr float PASSWORD_TEXT_RIGHT = 1014.0f;
static constexpr float PASSWORD_TEXT_BOTTOM = 624.0f;
static constexpr float LOGIN_ID_FIELD_LEFT = 631.0f;
static constexpr float LOGIN_ID_FIELD_TOP = 499.0f;
static constexpr float LOGIN_ID_FIELD_RIGHT = 1030.0f;
static constexpr float LOGIN_ID_FIELD_BOTTOM = 554.0f;
static constexpr float PASSWORD_FIELD_LEFT = 631.0f;
static constexpr float PASSWORD_FIELD_TOP = 570.0f;
static constexpr float PASSWORD_FIELD_RIGHT = 1030.0f;
static constexpr float PASSWORD_FIELD_BOTTOM = 626.0f;

static std::wstring FindAssetPath(const wchar_t* fileName)
{
    wchar_t modulePath[MAX_PATH]{};
    ::GetModuleFileNameW(nullptr, modulePath, MAX_PATH);

    std::filesystem::path currentPath(modulePath);
    currentPath = currentPath.parent_path();

    for (int i = 0; 8 > i && !currentPath.empty(); ++i)
    {
        const auto candidate = currentPath / L"Assets" / fileName;
        std::error_code errorCode;

        if (std::filesystem::exists(candidate, errorCode))
            return candidate.wstring();

        currentPath = currentPath.parent_path();
    }

    return (std::filesystem::current_path() / L".." / L"Assets" / fileName)
        .lexically_normal()
        .wstring();
}

HRESULT D2DRenderer::Initialize(HWND hwnd)
{
    _hwnd = hwnd;
    _startScreenPath = FindAssetPath(START_SCREEN_FILE_NAME);
    _signUpScreenPath = FindAssetPath(SIGN_UP_SCREEN_FILE_NAME);

    return CreateDeviceIndependentResources();
}

void D2DRenderer::Shutdown()
{
    DiscardDeviceResources();
    _inputTextFormat.Reset();
    _writeFactory.Reset();
    _wicFactory.Reset();
    _factory.Reset();
    _hwnd = nullptr;
}

void D2DRenderer::Resize(UINT width, UINT height)
{
    if (_renderTarget)
        _renderTarget->Resize(D2D1::SizeU(width, height));
}

HRESULT D2DRenderer::Render(
    bool gameStarted,
    bool showSignUpScreen,
    const std::wstring& loginIdText,
    const std::wstring& passwordText,
    bool loginIdFocused,
    bool passwordFocused,
    float totalTime)
{
    HRESULT hr = CreateDeviceResources();
    if (FAILED(hr))
        return hr;

    _renderTarget->BeginDraw();
    _renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
    _renderTarget->Clear(D2D1::ColorF(0x10151F));

    if (gameStarted)
        DrawScene(totalTime);
    else {
        DrawStartScreen();
        DrawStartScreenInputText(loginIdText, passwordText, loginIdFocused, passwordFocused);

        if (showSignUpScreen)
            DrawSignUpScreen();
    }

    hr = _renderTarget->EndDraw();
    if (D2DERR_RECREATE_TARGET == hr)
    {
        DiscardDeviceResources();
        hr = S_OK;
    }

    return hr;
}

HRESULT D2DRenderer::CreateDeviceIndependentResources()
{
    HRESULT hr = ::D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED, _factory.GetAddressOf());

    if (SUCCEEDED(hr))
    {
        hr = ::CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(_wicFactory.GetAddressOf()));
    }

    if (SUCCEEDED(hr))
    {
        hr = ::DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(_writeFactory.GetAddressOf()));
    }

    if (SUCCEEDED(hr))
    {
        hr = _writeFactory->CreateTextFormat(
            L"Malgun Gothic",
            nullptr,
            DWRITE_FONT_WEIGHT_BOLD,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            34.0f,
            L"ko-kr",
            _inputTextFormat.GetAddressOf());
    }

    if (SUCCEEDED(hr))
    {
        _inputTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        _inputTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    return hr;
}

HRESULT D2DRenderer::CreateDeviceResources()
{
    if (_renderTarget)
        return S_OK;

    RECT clientRect{};
    ::GetClientRect(_hwnd, &clientRect);

    const D2D1_SIZE_U size = D2D1::SizeU(
        static_cast<UINT>(clientRect.right - clientRect.left),
        static_cast<UINT>(clientRect.bottom - clientRect.top));

    HRESULT hr = _factory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(_hwnd, size),
        _renderTarget.GetAddressOf());

    if (SUCCEEDED(hr))
    {
        _renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

        hr = _renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(0x263244),
            _gridBrush.GetAddressOf());
    }

    if (SUCCEEDED(hr))
    {
        hr = _renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(0xF2C14E),
            _accentBrush.GetAddressOf());
    }

    if (SUCCEEDED(hr))
    {
        hr = _renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(0x3DDC97),
            _playerBrush.GetAddressOf());
    }

    if (SUCCEEDED(hr))
    {
        hr = _renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(0xE8D2A2),
            _inputTextBrush.GetAddressOf());
    }

    if (SUCCEEDED(hr))
    {
        hr = _renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(0x101010, 0.90f),
            _inputCoverBrush.GetAddressOf());
    }

    if (SUCCEEDED(hr))
    {
        hr = _renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(0xF2C14E, 0.85f),
            _inputFocusBrush.GetAddressOf());
    }

    return hr;
}

void D2DRenderer::DiscardDeviceResources()
{
    _startScreenBitmap.Reset();
    _signUpScreenBitmap.Reset();
    _playerBrush.Reset();
    _inputFocusBrush.Reset();
    _inputCoverBrush.Reset();
    _inputTextBrush.Reset();
    _accentBrush.Reset();
    _gridBrush.Reset();
    _renderTarget.Reset();
}

HRESULT D2DRenderer::LoadBitmapFromFile(const std::wstring& filePath, ID2D1Bitmap** bitmap)
{
    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    HRESULT hr = _wicFactory->CreateDecoderFromFilename(
        filePath.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        decoder.GetAddressOf());

    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    if (SUCCEEDED(hr))
        hr = decoder->GetFrame(0, frame.GetAddressOf());

    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    if (SUCCEEDED(hr))
        hr = _wicFactory->CreateFormatConverter(converter.GetAddressOf());

    if (SUCCEEDED(hr))
    {
        hr = converter->Initialize(
            frame.Get(),
            GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0f,
            WICBitmapPaletteTypeMedianCut);
    }

    if (SUCCEEDED(hr))
        hr = _renderTarget->CreateBitmapFromWicBitmap(converter.Get(), nullptr, bitmap);

    return hr;
}

void D2DRenderer::DrawStartScreen()
{
    if (!_startScreenBitmap && !_startScreenLoadFailed)
    {
        const HRESULT hr = LoadBitmapFromFile(_startScreenPath, _startScreenBitmap.GetAddressOf());
        _startScreenLoadFailed = FAILED(hr);
    }

    if (!_startScreenBitmap)
        return;

    const D2D1_SIZE_F renderSize = _renderTarget->GetSize();
    const D2D1_SIZE_F bitmapSize = _startScreenBitmap->GetSize();
    const float scale = std::min(renderSize.width / bitmapSize.width, renderSize.height / bitmapSize.height);
    const float width = bitmapSize.width * scale;
    const float height = bitmapSize.height * scale;
    const float left = (renderSize.width - width) * 0.5f;
    const float top = (renderSize.height - height) * 0.5f;

    _renderTarget->DrawBitmap(
        _startScreenBitmap.Get(),
        D2D1::RectF(left, top, left + width, top + height),
        1.0f,
        D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
}

void D2DRenderer::DrawStartScreenInputText(
    const std::wstring& loginIdText,
    const std::wstring& passwordText,
    bool loginIdFocused,
    bool passwordFocused)
{
    const D2D1_SIZE_F renderSize = _renderTarget->GetSize();
    const float scale = std::min(renderSize.width / START_SCREEN_WIDTH, renderSize.height / START_SCREEN_HEIGHT);
    const float imageWidth = START_SCREEN_WIDTH * scale;
    const float imageHeight = START_SCREEN_HEIGHT * scale;
    const float imageLeft = (renderSize.width - imageWidth) * 0.5f;
    const float imageTop = (renderSize.height - imageHeight) * 0.5f;
    const D2D1_MATRIX_3X2_F startScreenTransform =
        D2D1::Matrix3x2F::Scale(scale, scale) *
        D2D1::Matrix3x2F::Translation(imageLeft, imageTop);

    _renderTarget->SetTransform(startScreenTransform);

    const D2D1_RECT_F loginCoverRect = D2D1::RectF(
        LOGIN_ID_TEXT_LEFT,
        LOGIN_ID_TEXT_TOP,
        LOGIN_ID_TEXT_RIGHT,
        LOGIN_ID_TEXT_BOTTOM);
    const D2D1_RECT_F passwordCoverRect = D2D1::RectF(
        PASSWORD_TEXT_LEFT,
        PASSWORD_TEXT_TOP,
        PASSWORD_TEXT_RIGHT,
        PASSWORD_TEXT_BOTTOM);
    const D2D1_RECT_F loginFocusRect = D2D1::RectF(
        LOGIN_ID_FIELD_LEFT,
        LOGIN_ID_FIELD_TOP,
        LOGIN_ID_FIELD_RIGHT,
        LOGIN_ID_FIELD_BOTTOM);
    const D2D1_RECT_F passwordFocusRect = D2D1::RectF(
        PASSWORD_FIELD_LEFT,
        PASSWORD_FIELD_TOP,
        PASSWORD_FIELD_RIGHT,
        PASSWORD_FIELD_BOTTOM);

    if (false == loginIdText.empty())
        _renderTarget->FillRectangle(loginCoverRect, _inputCoverBrush.Get());

    if (false == passwordText.empty())
        _renderTarget->FillRectangle(passwordCoverRect, _inputCoverBrush.Get());

    if (loginIdFocused)
        _renderTarget->DrawRectangle(loginFocusRect, _inputFocusBrush.Get(), 3.0f);

    if (passwordFocused)
        _renderTarget->DrawRectangle(passwordFocusRect, _inputFocusBrush.Get(), 3.0f);

    if (false == loginIdText.empty())
    {
        _renderTarget->DrawTextW(
            loginIdText.c_str(),
            static_cast<UINT32>(loginIdText.length()),
            _inputTextFormat.Get(),
            loginCoverRect,
            _inputTextBrush.Get());
    }

    if (false == passwordText.empty())
    {
        const std::wstring maskedPassword(passwordText.length(), L'*');
        _renderTarget->DrawTextW(
            maskedPassword.c_str(),
            static_cast<UINT32>(maskedPassword.length()),
            _inputTextFormat.Get(),
            passwordCoverRect,
            _inputTextBrush.Get());
    }

    _renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
}

void D2DRenderer::DrawSignUpScreen()
{
    if (!_signUpScreenBitmap && !_signUpScreenLoadFailed)
    {
        const HRESULT hr = LoadBitmapFromFile(_signUpScreenPath, _signUpScreenBitmap.GetAddressOf());
        _signUpScreenLoadFailed = FAILED(hr);
    }

    if (!_signUpScreenBitmap)
        return;

    const D2D1_SIZE_F renderSize = _renderTarget->GetSize();
    const D2D1_SIZE_F bitmapSize = _signUpScreenBitmap->GetSize();
    const float scale = std::min(renderSize.width / bitmapSize.width, renderSize.height / bitmapSize.height);
    const float width = bitmapSize.width * scale;
    const float height = bitmapSize.height * scale;
    const float left = (renderSize.width - width) * 0.5f;
    const float top = (renderSize.height - height) * 0.5f;

    _renderTarget->DrawBitmap(
        _signUpScreenBitmap.Get(),
        D2D1::RectF(left, top, left + width, top + height),
        1.0f,
        D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
}

void D2DRenderer::DrawScene(float totalTime)
{
    const D2D1_SIZE_F size = _renderTarget->GetSize();
    constexpr float tileSize = 32.0f;

    for (float x = 0.0f; size.width > x; x += tileSize)
    {
        _renderTarget->DrawLine(
            D2D1::Point2F(x, 0.0f),
            D2D1::Point2F(x, size.height),
            _gridBrush.Get(),
            1.0f);
    }

    for (float y = 0.0f; size.height > y; y += tileSize)
    {
        _renderTarget->DrawLine(
            D2D1::Point2F(0.0f, y),
            D2D1::Point2F(size.width, y),
            _gridBrush.Get(),
            1.0f);
    }

    const float centerX = size.width * 0.5f;
    const float centerY = size.height * 0.5f;
    const float orbitRadius = std::min(size.width, size.height) * 0.22f;
    const float playerX = centerX + std::cos(totalTime) * orbitRadius;
    const float playerY = centerY + std::sin(totalTime) * orbitRadius;

    const D2D1_RECT_F arena = D2D1::RectF(
        centerX - 220.0f,
        centerY - 140.0f,
        centerX + 220.0f,
        centerY + 140.0f);

    _renderTarget->DrawRectangle(arena, _accentBrush.Get(), 3.0f);

    _renderTarget->FillEllipse(
        D2D1::Ellipse(D2D1::Point2F(playerX, playerY), 18.0f, 18.0f),
        _playerBrush.Get());
}
