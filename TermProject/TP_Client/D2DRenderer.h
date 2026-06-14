#pragma once

#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <string>
#include <wrl/client.h>
#include <wincodec.h>

class D2DRenderer
{
public:
    HRESULT Initialize(HWND hwnd);
    void Shutdown();
    void Resize(UINT width, UINT height);
    HRESULT Render(
        bool gameStarted,
        bool showSignUpScreen,
        const std::wstring& loginIdText,
        const std::wstring& passwordText,
        bool loginIdFocused,
        bool passwordFocused,
        float totalTime);

private:
    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceResources();
    HRESULT LoadBitmapFromFile(const std::wstring& filePath, ID2D1Bitmap** bitmap);
    void DiscardDeviceResources();
    void DrawStartScreen();
    void DrawStartScreenInputText(
        const std::wstring& loginIdText,
        const std::wstring& passwordText,
        bool loginIdFocused,
        bool passwordFocused);
    void DrawSignUpScreen();
    void DrawScene(float totalTime);

    HWND _hwnd{};
    Microsoft::WRL::ComPtr<ID2D1Factory> _factory;
    Microsoft::WRL::ComPtr<IWICImagingFactory> _wicFactory;
    Microsoft::WRL::ComPtr<IDWriteFactory> _writeFactory;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> _inputTextFormat;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> _renderTarget;
    Microsoft::WRL::ComPtr<ID2D1Bitmap> _startScreenBitmap;
    Microsoft::WRL::ComPtr<ID2D1Bitmap> _signUpScreenBitmap;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> _gridBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> _accentBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> _playerBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> _inputTextBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> _inputCoverBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> _inputFocusBrush;
    std::wstring _startScreenPath;
    std::wstring _signUpScreenPath;
    bool _startScreenLoadFailed{};
    bool _signUpScreenLoadFailed{};
};
