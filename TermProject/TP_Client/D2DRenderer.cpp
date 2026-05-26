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

    for (int i = 0; i < 8 && !currentPath.empty(); ++i)
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
    hwnd_ = hwnd;
    startScreenPath_ = FindAssetPath(START_SCREEN_FILE_NAME);

    return CreateDeviceIndependentResources();
}

void D2DRenderer::Resize(UINT width, UINT height)
{
    if (renderTarget_)
        renderTarget_->Resize(D2D1::SizeU(width, height));
}

HRESULT D2DRenderer::Render(bool gameStarted, float totalTime)
{
    HRESULT hr = CreateDeviceResources();
    if (FAILED(hr))
        return hr;

    renderTarget_->BeginDraw();
    renderTarget_->SetTransform(D2D1::Matrix3x2F::Identity());
    renderTarget_->Clear(D2D1::ColorF(0x10151F));

    if (gameStarted)
        DrawScene(totalTime);
    else
        DrawStartScreen();

    hr = renderTarget_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET)
    {
        DiscardDeviceResources();
        hr = S_OK;
    }

    return hr;
}

HRESULT D2DRenderer::CreateDeviceIndependentResources()
{
    HRESULT hr = ::D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED, factory_.GetAddressOf());

    if (SUCCEEDED(hr))
    {
        hr = ::CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(wicFactory_.GetAddressOf()));
    }

    return hr;
}

HRESULT D2DRenderer::CreateDeviceResources()
{
    if (renderTarget_)
        return S_OK;

    RECT clientRect{};
    ::GetClientRect(hwnd_, &clientRect);

    const D2D1_SIZE_U size = D2D1::SizeU(
        static_cast<UINT>(clientRect.right - clientRect.left),
        static_cast<UINT>(clientRect.bottom - clientRect.top));

    HRESULT hr = factory_->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hwnd_, size),
        renderTarget_.GetAddressOf());

    if (SUCCEEDED(hr))
    {
        renderTarget_->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

        hr = renderTarget_->CreateSolidColorBrush(
            D2D1::ColorF(0x263244),
            gridBrush_.GetAddressOf());
    }

    if (SUCCEEDED(hr))
    {
        hr = renderTarget_->CreateSolidColorBrush(
            D2D1::ColorF(0xF2C14E),
            accentBrush_.GetAddressOf());
    }

    if (SUCCEEDED(hr))
    {
        hr = renderTarget_->CreateSolidColorBrush(
            D2D1::ColorF(0x3DDC97),
            playerBrush_.GetAddressOf());
    }

    return hr;
}

void D2DRenderer::DiscardDeviceResources()
{
    startScreenBitmap_.Reset();
    playerBrush_.Reset();
    accentBrush_.Reset();
    gridBrush_.Reset();
    renderTarget_.Reset();
}

HRESULT D2DRenderer::LoadBitmapFromFile(const std::wstring& filePath, ID2D1Bitmap** bitmap)
{
    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    HRESULT hr = wicFactory_->CreateDecoderFromFilename(
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
        hr = wicFactory_->CreateFormatConverter(converter.GetAddressOf());

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
        hr = renderTarget_->CreateBitmapFromWicBitmap(converter.Get(), nullptr, bitmap);

    return hr;
}

void D2DRenderer::DrawStartScreen()
{
    if (!startScreenBitmap_ && !startScreenLoadFailed_)
    {
        const HRESULT hr = LoadBitmapFromFile(startScreenPath_, startScreenBitmap_.GetAddressOf());
        startScreenLoadFailed_ = FAILED(hr);
    }

    if (!startScreenBitmap_)
        return;

    const D2D1_SIZE_F renderSize = renderTarget_->GetSize();
    const D2D1_SIZE_F bitmapSize = startScreenBitmap_->GetSize();
    const float scale = std::min(renderSize.width / bitmapSize.width, renderSize.height / bitmapSize.height);
    const float width = bitmapSize.width * scale;
    const float height = bitmapSize.height * scale;
    const float left = (renderSize.width - width) * 0.5f;
    const float top = (renderSize.height - height) * 0.5f;

    renderTarget_->DrawBitmap(
        startScreenBitmap_.Get(),
        D2D1::RectF(left, top, left + width, top + height),
        1.0f,
        D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
}

void D2DRenderer::DrawScene(float totalTime)
{
    const D2D1_SIZE_F size = renderTarget_->GetSize();
    constexpr float tileSize = 32.0f;

    for (float x = 0.0f; x < size.width; x += tileSize)
    {
        renderTarget_->DrawLine(
            D2D1::Point2F(x, 0.0f),
            D2D1::Point2F(x, size.height),
            gridBrush_.Get(),
            1.0f);
    }

    for (float y = 0.0f; y < size.height; y += tileSize)
    {
        renderTarget_->DrawLine(
            D2D1::Point2F(0.0f, y),
            D2D1::Point2F(size.width, y),
            gridBrush_.Get(),
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

    renderTarget_->DrawRectangle(arena, accentBrush_.Get(), 3.0f);

    renderTarget_->FillEllipse(
        D2D1::Ellipse(D2D1::Point2F(playerX, playerY), 18.0f, 18.0f),
        playerBrush_.Get());
}
