#pragma once
#include <d2d1_1.h>
#include <dwrite.h>
#include <wincodec.h>

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

    // Draw helpers
    void DrawStartMenu(float ScreenW, float ScreenH);
    void DrawDeathScreen(float ScreenW, float ScreenH, const wchar_t* Text, bool bIsVictory);

    // Create gradient brush for the banner (recreated per-frame due to screen size changes)
    ID2D1LinearGradientBrush* CreateBannerGradientBrush(float ScreenW, float ScreenH, float Opacity);

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
    IDWriteTextFormat* DeathTextFormat = nullptr; // "YOU DIED" / "DEMIGOD FELLED" format

    // Brushes
    ID2D1SolidColorBrush* TextBrush = nullptr;
    ID2D1SolidColorBrush* DeathTextBrush = nullptr;   // Blood red for "YOU DIED"
    ID2D1SolidColorBrush* VictoryTextBrush = nullptr; // Golden for "DEMIGOD FELLED"

    // Animation state
    float PulseTimer = 0.f;
    float PulseSpeed = 2.0f;  // Cycles per second

    // Death/Victory screen animation
    float DeathScreenTimer = 0.f;
    float DeathFadeInDuration = 1.5f;   // Time to fade in
    float DeathHoldDuration = 3.0f;     // Time to hold at full opacity
    float DeathFadeOutDuration = 1.0f;  // Time to fade out

    // Custom font
    bool bFontLoaded = false;
};
