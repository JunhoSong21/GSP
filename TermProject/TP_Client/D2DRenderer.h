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

    HWND hwnd_{};
    Microsoft::WRL::ComPtr<ID2D1Factory> factory_;
    Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory_;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> renderTarget_;
    Microsoft::WRL::ComPtr<ID2D1Bitmap> startScreenBitmap_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> gridBrush_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> accentBrush_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> playerBrush_;
    std::wstring startScreenPath_;
    bool startScreenLoadFailed_{};
};
