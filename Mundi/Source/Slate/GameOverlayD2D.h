#pragma once
#include <d2d1_1.h>
#include <dwrite.h>

class UGameOverlayD2D
{
public:
    static UGameOverlayD2D& Get();

    void Initialize(ID3D11Device* Device, ID3D11DeviceContext* Context, IDXGISwapChain* InSwapChain);
    void Shutdown();
    void Draw();

private:
    UGameOverlayD2D() = default;
    ~UGameOverlayD2D() = default;
    UGameOverlayD2D(const UGameOverlayD2D&) = delete;
    UGameOverlayD2D& operator=(const UGameOverlayD2D&) = delete;

    void EnsureInitialized();
    void ReleaseD2DResources();

private:
    bool bInitialized = false;

    // D3D references
    ID3D11Device* D3DDevice = nullptr;
    ID3D11DeviceContext* D3DContext = nullptr;
    IDXGISwapChain* SwapChain = nullptr;

    // D2D resources
    ID2D1Factory1* D2DFactory = nullptr;
    ID2D1Device* D2DDevice = nullptr;
    ID2D1DeviceContext* D2DContext = nullptr;
    IDWriteFactory* DWriteFactory = nullptr;

    // Text formats
    IDWriteTextFormat* TitleFormat = nullptr;     // Large font for game title
    IDWriteTextFormat* SubtitleFormat = nullptr;  // Smaller font for "Press any key"

    // Brushes
    ID2D1SolidColorBrush* TextBrush = nullptr;

    // Animation state
    float PulseTimer = 0.f;
    float PulseSpeed = 2.0f;  // Cycles per second

    // Custom font
    bool bFontLoaded = false;
};
