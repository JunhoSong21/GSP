#include "D2DRenderer.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <system_error>

static constexpr wchar_t START_SCREEN_FILE_NAME[] = L"StartScreen.png";

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

    return CreateDeviceIndependentResources();
}

void D2DRenderer::Resize(UINT width, UINT height)
{
    if (_renderTarget)
        _renderTarget->Resize(D2D1::SizeU(width, height));
}

HRESULT D2DRenderer::Render(bool gameStarted, float totalTime)
{
    HRESULT hr = CreateDeviceResources();
    if (FAILED(hr))
        return hr;

    _renderTarget->BeginDraw();
    _renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
    _renderTarget->Clear(D2D1::ColorF(0x10151F));

    if (gameStarted)
        DrawScene(totalTime);
    else
        DrawStartScreen();

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

    return hr;
}

void D2DRenderer::DiscardDeviceResources()
{
    _startScreenBitmap.Reset();
    _playerBrush.Reset();
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
