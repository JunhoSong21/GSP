#pragma once

#include <Windows.h>
#include <d2d1.h>
#include <string>
#include <wrl/client.h>
#include <wincodec.h>

class D2DRenderer
{
public:
    HRESULT Initialize(HWND hwnd);
    void Resize(UINT width, UINT height);
    HRESULT Render(bool gameStarted, float totalTime);

private:
    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceResources();
    HRESULT LoadBitmapFromFile(const std::wstring& filePath, ID2D1Bitmap** bitmap);
    void DiscardDeviceResources();
    void DrawStartScreen();
    void DrawScene(float totalTime);

    HWND _hwnd{};
    Microsoft::WRL::ComPtr<ID2D1Factory> _factory;
    Microsoft::WRL::ComPtr<IWICImagingFactory> _wicFactory;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> _renderTarget;
    Microsoft::WRL::ComPtr<ID2D1Bitmap> _startScreenBitmap;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> _gridBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> _accentBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> _playerBrush;
    std::wstring _startScreenPath;
    bool _startScreenLoadFailed{};
};
