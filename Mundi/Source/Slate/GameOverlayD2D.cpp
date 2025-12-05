#include "pch.h"

#include <d2d1_1.h>
#include <dwrite.h>
#include <dxgi1_2.h>
#include <wincodec.h>
#include <cmath>
#include <Windows.h>

#include "GameOverlayD2D.h"
#include "UIManager.h"
#include "World.h"
#include "GameModeBase.h"
#include "GameState.h"
#include "GameEngine.h"
#include "D3D11RHI.h"
#include "FViewport.h"
#include "Source/Runtime/Core/Misc/PathUtils.h"

#ifdef _EDITOR
#include "USlateManager.h"
#include "Source/Slate/Windows/SViewportWindow.h"
#endif

#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")
#pragma comment(lib, "windowscodecs")

// Custom font settings
static const wchar_t* CUSTOM_FONT_NAME = L"Mantinia";

static inline void SafeRelease(IUnknown* p) { if (p) p->Release(); }

UGameOverlayD2D& UGameOverlayD2D::Get()
{
    static UGameOverlayD2D Instance;
    return Instance;
}

void UGameOverlayD2D::Initialize(ID3D11Device* InDevice, ID3D11DeviceContext* InContext, IDXGISwapChain* InSwapChain)
{
    OutputDebugStringA("[GameOverlayD2D] Initialize called\n");

    D3DDevice = InDevice;
    D3DContext = InContext;
    SwapChain = InSwapChain;
    bInitialized = (D3DDevice && D3DContext && SwapChain);

    if (!bInitialized)
    {
        return;
    }

    D2D1_FACTORY_OPTIONS FactoryOpts{};
#ifdef _DEBUG
    FactoryOpts.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &FactoryOpts, (void**)&D2DFactory)))
    {
        return;
    }

    IDXGIDevice* DxgiDevice = nullptr;
    if (FAILED(D3DDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&DxgiDevice)))
    {
        return;
    }

    if (FAILED(D2DFactory->CreateDevice(DxgiDevice, &D2DDevice)))
    {
        SafeRelease(DxgiDevice);
        return;
    }
    SafeRelease(DxgiDevice);

    if (FAILED(D2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &D2DContext)))
    {
        return;
    }

    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&DWriteFactory)))
    {
        return;
    }

    // Initialize WIC factory for image loading
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&WICFactory))))
    {
        // Continue without WIC - images won't work but text will
    }

    // Load custom font from file (build full path using GDataDir)
    // Note: Using 0 instead of FR_PRIVATE so DirectWrite can see the font
    FWideString FontPath = UTF8ToWide(GDataDir) + L"/UI/Fonts/Mantinia Regular.otf";
    int FontsAdded = AddFontResourceExW(FontPath.c_str(), 0, 0);
    bFontLoaded = (FontsAdded > 0);

    // Use custom font if loaded, fallback to Segoe UI
    const wchar_t* FontName = bFontLoaded ? CUSTOM_FONT_NAME : L"Segoe UI";

    if (DWriteFactory)
    {
        // Title format - large centered text for start menu
        DWriteFactory->CreateTextFormat(
            FontName,
            nullptr,
            DWRITE_FONT_WEIGHT_REGULAR,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            200.0f,
            L"en-us",
            &TitleFormat);

        if (TitleFormat)
        {
            TitleFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            TitleFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        // Subtitle format - smaller centered text
        DWriteFactory->CreateTextFormat(
            FontName,
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            35.0f,
            L"en-us",
            &SubtitleFormat);

        if (SubtitleFormat)
        {
            SubtitleFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            SubtitleFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        // Death/Victory text format - large, dramatic
        DWriteFactory->CreateTextFormat(
            FontName,
            nullptr,
            DWRITE_FONT_WEIGHT_REGULAR,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            96.0f,
            L"en-us",
            &DeathTextFormat);

        if (DeathTextFormat)
        {
            DeathTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            DeathTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        // Boss name text format - above health bar
        DWriteFactory->CreateTextFormat(
            FontName,
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            28.0f,
            L"en-us",
            &BossNameFormat);

        if (BossNameFormat)
        {
            BossNameFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            BossNameFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }

    // Load logo image
    if (WICFactory && D2DContext)
    {
        FWideString LogoPath = UTF8ToWide(GDataDir) + L"/UI/Icons/JeldenJingLogo.png";

        IWICBitmapDecoder* Decoder = nullptr;
        if (SUCCEEDED(WICFactory->CreateDecoderFromFilename(LogoPath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &Decoder)))
        {
            IWICBitmapFrameDecode* Frame = nullptr;
            if (SUCCEEDED(Decoder->GetFrame(0, &Frame)))
            {
                IWICFormatConverter* Converter = nullptr;
                if (SUCCEEDED(WICFactory->CreateFormatConverter(&Converter)))
                {
                    if (SUCCEEDED(Converter->Initialize(Frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeMedianCut)))
                    {
                        if (SUCCEEDED(D2DContext->CreateBitmapFromWicBitmap(Converter, nullptr, &LogoBitmap)))
                        {
                            D2D1_SIZE_F Size = LogoBitmap->GetSize();
                            LogoWidth = Size.width;
                            LogoHeight = Size.height;
                        }
                    }
                    SafeRelease(Converter);
                }
                SafeRelease(Frame);
            }
            SafeRelease(Decoder);
        }
    }

    // Load boss health bar frame image
    if (WICFactory && D2DContext)
    {
        FWideString FramePath = UTF8ToWide(GDataDir) + L"/UI/Icons/TX_Bar_Frame_Enemy.PNG";

        IWICBitmapDecoder* Decoder = nullptr;
        if (SUCCEEDED(WICFactory->CreateDecoderFromFilename(FramePath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &Decoder)))
        {
            IWICBitmapFrameDecode* Frame = nullptr;
            if (SUCCEEDED(Decoder->GetFrame(0, &Frame)))
            {
                IWICFormatConverter* Converter = nullptr;
                if (SUCCEEDED(WICFactory->CreateFormatConverter(&Converter)))
                {
                    if (SUCCEEDED(Converter->Initialize(Frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeMedianCut)))
                    {
                        if (SUCCEEDED(D2DContext->CreateBitmapFromWicBitmap(Converter, nullptr, &BossFrameBitmap)))
                        {
                            D2D1_SIZE_F Size = BossFrameBitmap->GetSize();
                            BossFrameWidth = Size.width;
                            BossFrameHeight = Size.height;
                        }
                    }
                    SafeRelease(Converter);
                }
                SafeRelease(Frame);
            }
            SafeRelease(Decoder);
        }
    }

    // Load boss health bar fill image
    if (WICFactory && D2DContext)
    {
        FWideString BarPath = UTF8ToWide(GDataDir) + L"/UI/Icons/TX_Gauge_EnemyHP_Bar.PNG";

        IWICBitmapDecoder* Decoder = nullptr;
        if (SUCCEEDED(WICFactory->CreateDecoderFromFilename(BarPath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &Decoder)))
        {
            IWICBitmapFrameDecode* Frame = nullptr;
            if (SUCCEEDED(Decoder->GetFrame(0, &Frame)))
            {
                IWICFormatConverter* Converter = nullptr;
                if (SUCCEEDED(WICFactory->CreateFormatConverter(&Converter)))
                {
                    if (SUCCEEDED(Converter->Initialize(Frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeMedianCut)))
                    {
                        if (SUCCEEDED(D2DContext->CreateBitmapFromWicBitmap(Converter, nullptr, &BossBarBitmap)))
                        {
                            D2D1_SIZE_F Size = BossBarBitmap->GetSize();
                            BossBarWidth = Size.width;
                            BossBarHeight = Size.height;
                        }
                    }
                    SafeRelease(Converter);
                }
                SafeRelease(Frame);
            }
            SafeRelease(Decoder);
        }
    }

    EnsureInitialized();
}

void UGameOverlayD2D::Shutdown()
{
    ReleaseD2DResources();

    // Unload custom font
    if (bFontLoaded)
    {
        FWideString FontPath = UTF8ToWide(GDataDir) + L"/UI/Fonts/Mantinia Regular.otf";
        RemoveFontResourceExW(FontPath.c_str(), 0, 0);
        bFontLoaded = false;
    }

    D3DDevice = nullptr;
    D3DContext = nullptr;
    SwapChain = nullptr;
    bInitialized = false;
}

void UGameOverlayD2D::EnsureInitialized()
{
    if (!D2DContext)
    {
        return;
    }

    SafeRelease(TextBrush);
    SafeRelease(SubtitleBrush);
    SafeRelease(DeathTextBrush);
    SafeRelease(VictoryTextBrush);

    // Title text color: RGB(207, 185, 144)
    D2DContext->CreateSolidColorBrush(D2D1::ColorF(207.0f/255.0f, 185.0f/255.0f, 144.0f/255.0f, 1.0f), &TextBrush);

    // Subtitle text color: White
    D2DContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &SubtitleBrush);

    // Blood red for "YOU DIED"
    D2DContext->CreateSolidColorBrush(D2D1::ColorF(0.6f, 0.0f, 0.0f, 1.0f), &DeathTextBrush);

    // Golden for "DEMIGOD FELLED"
    D2DContext->CreateSolidColorBrush(D2D1::ColorF(0.85f, 0.65f, 0.13f, 1.0f), &VictoryTextBrush);
}

void UGameOverlayD2D::ReleaseD2DResources()
{
    SafeRelease(LogoBitmap);
    SafeRelease(BossFrameBitmap);
    SafeRelease(BossBarBitmap);
    SafeRelease(TextBrush);
    SafeRelease(SubtitleBrush);
    SafeRelease(DeathTextBrush);
    SafeRelease(VictoryTextBrush);
    SafeRelease(BossNameFormat);
    SafeRelease(DeathTextFormat);
    SafeRelease(SubtitleFormat);
    SafeRelease(TitleFormat);
    SafeRelease(DWriteFactory);
    SafeRelease(WICFactory);
    SafeRelease(D2DContext);
    SafeRelease(D2DDevice);
    SafeRelease(D2DFactory);
}

// ============================================================================
// Gradient Brush Creation
// ============================================================================

ID2D1LinearGradientBrush* UGameOverlayD2D::CreateBannerGradientBrush(float ScreenW, float ScreenH, float Opacity)
{
    if (!D2DContext)
    {
        return nullptr;
    }

    // Banner height (about 15% of screen height)
    float BannerHeight = ScreenH * 0.15f;
    float BannerTop = (ScreenH - BannerHeight) * 0.5f;
    float BannerBottom = BannerTop + BannerHeight;

    // Gradient stops: transparent -> black -> black -> transparent (vertical gradient)
    D2D1_GRADIENT_STOP GradientStops[4];
    GradientStops[0].position = 0.0f;
    GradientStops[0].color = D2D1::ColorF(0, 0, 0, 0);  // Transparent at top edge
    GradientStops[1].position = 0.3f;
    GradientStops[1].color = D2D1::ColorF(0, 0, 0, 0.85f * Opacity);  // Fade to black
    GradientStops[2].position = 0.7f;
    GradientStops[2].color = D2D1::ColorF(0, 0, 0, 0.85f * Opacity);  // Hold black
    GradientStops[3].position = 1.0f;
    GradientStops[3].color = D2D1::ColorF(0, 0, 0, 0);  // Transparent at bottom edge

    ID2D1GradientStopCollection* GradientStopCollection = nullptr;
    HRESULT hr = D2DContext->CreateGradientStopCollection(
        GradientStops,
        4,
        D2D1_GAMMA_2_2,
        D2D1_EXTEND_MODE_CLAMP,
        &GradientStopCollection);

    if (FAILED(hr))
    {
        return nullptr;
    }

    ID2D1LinearGradientBrush* GradientBrush = nullptr;
    hr = D2DContext->CreateLinearGradientBrush(
        D2D1::LinearGradientBrushProperties(
            D2D1::Point2F(0, BannerTop),
            D2D1::Point2F(0, BannerBottom)),
        GradientStopCollection,
        &GradientBrush);

    SafeRelease(GradientStopCollection);

    return GradientBrush;
}

// ============================================================================
// Draw Functions
// ============================================================================

void UGameOverlayD2D::DrawStartMenu(float ScreenW, float ScreenH)
{
    // Draw logo in background (centered, match height of screen)
    if (LogoBitmap && LogoWidth > 0 && LogoHeight > 0)
    {
        // Target size: 75% of screen height, maintain aspect ratio
        float TargetH = ScreenH;
        float Scale = TargetH / LogoHeight;
        float ScaledW = LogoWidth * Scale;
        float ScaledH = TargetH;

        // Center the logo
        float DrawX = (ScreenW - ScaledW) * 0.5f;
        float DrawY = (ScreenH - ScaledH) * 0.5f;

        D2D1_RECT_F LogoRect = D2D1::RectF(DrawX, DrawY, DrawX + ScaledW, DrawY + ScaledH);
        D2DContext->DrawBitmap(LogoBitmap, LogoRect, 1.0f);
    }

    TextBrush->SetOpacity(1.0f);

    // Title text - centered, upper portion of screen
    const wchar_t* TitleText = L"Jelden Jing";
    D2D1_RECT_F TitleRect = D2D1::RectF(0, ScreenH * 0.35f, ScreenW, ScreenH * 0.50f);
    D2DContext->DrawTextW(
        TitleText,
        static_cast<UINT32>(wcslen(TitleText)),
        TitleFormat,
        TitleRect,
        TextBrush);

    // Subtitle text - centered, below title
    const wchar_t* SubtitleText = L"Press any key to start";
    D2D1_RECT_F SubtitleRect = D2D1::RectF(0, ScreenH * 0.85f, ScreenW, ScreenH * 0.65f);
    D2DContext->DrawTextW(
        SubtitleText,
        static_cast<UINT32>(wcslen(SubtitleText)),
        SubtitleFormat,
        SubtitleRect,
        SubtitleBrush);
}

void UGameOverlayD2D::DrawDeathScreen(float ScreenW, float ScreenH, const wchar_t* Text, bool bIsVictory)
{
    float DeltaTime = UUIManager::GetInstance().GetDeltaTime();
    DeathScreenTimer += DeltaTime;

    // Calculate fade animation
    float Opacity = 0.0f;
    float TotalDuration = DeathFadeInDuration + DeathHoldDuration + DeathFadeOutDuration;

    if (DeathScreenTimer < DeathFadeInDuration)
    {
        // Fade in phase
        Opacity = DeathScreenTimer / DeathFadeInDuration;
    }
    else if (DeathScreenTimer < DeathFadeInDuration + DeathHoldDuration)
    {
        // Hold phase
        Opacity = 1.0f;
    }
    else if (DeathScreenTimer < TotalDuration)
    {
        // Fade out phase
        float FadeOutProgress = (DeathScreenTimer - DeathFadeInDuration - DeathHoldDuration) / DeathFadeOutDuration;
        Opacity = 1.0f - FadeOutProgress;
    }
    else
    {
        // Animation complete - still show at 0 opacity (or could skip drawing)
        Opacity = 0.0f;
    }

    if (Opacity <= 0.0f)
    {
        return;
    }

    // Clamp opacity
    Opacity = (Opacity > 1.0f) ? 1.0f : ((Opacity < 0.0f) ? 0.0f : Opacity);

    // Draw the gradient banner
    ID2D1LinearGradientBrush* BannerBrush = CreateBannerGradientBrush(ScreenW, ScreenH, Opacity);
    if (BannerBrush)
    {
        // Draw full-width banner at center of screen
        float BannerHeight = ScreenH * 0.15f;
        float BannerTop = (ScreenH - BannerHeight) * 0.5f;
        D2D1_RECT_F BannerRect = D2D1::RectF(0, BannerTop, ScreenW, BannerTop + BannerHeight);
        D2DContext->FillRectangle(BannerRect, BannerBrush);
        SafeRelease(BannerBrush);
    }

    // Select the appropriate colored brush
    ID2D1SolidColorBrush* ColoredBrush = bIsVictory ? VictoryTextBrush : DeathTextBrush;
    if (ColoredBrush)
    {
        ColoredBrush->SetOpacity(Opacity);
    }

    // Draw the text on top of the banner
    D2D1_RECT_F TextRect = D2D1::RectF(0, ScreenH * 0.425f, ScreenW, ScreenH * 0.575f);
    D2DContext->DrawTextW(
        Text,
        static_cast<UINT32>(wcslen(Text)),
        DeathTextFormat,
        TextRect,
        ColoredBrush ? ColoredBrush : TextBrush);
}

void UGameOverlayD2D::DrawBossHealthBar(float ScreenW, float ScreenH, float DeltaTime)
{
    // Check if we have valid boss bar resources
    if (!BossFrameBitmap || !BossBarBitmap || !BossNameFormat)
    {
        return;
    }

    // Get boss health from GameState
    if (!GWorld)
    {
        return;
    }

    AGameModeBase* GM = GWorld->GetGameMode();
    if (!GM)
    {
        return;
    }

    AGameState* GS = Cast<AGameState>(GM->GetGameState());
    if (!GS)
    {
        return;
    }

    // Get target health - default to 100% if no boss registered
    float TargetHealth = GS->HasActiveBoss() ? GS->GetBossHealth().GetPercent() : 1.0f;

    // Smooth lerp animation
    if (DisplayedBossHealth != TargetHealth)
    {
        float LerpAmount = HealthLerpSpeed * DeltaTime;
        if (DisplayedBossHealth > TargetHealth)
        {
            DisplayedBossHealth = (DisplayedBossHealth - LerpAmount < TargetHealth)
                ? TargetHealth : DisplayedBossHealth - LerpAmount;
        }
        else
        {
            DisplayedBossHealth = (DisplayedBossHealth + LerpAmount > TargetHealth)
                ? TargetHealth : DisplayedBossHealth + LerpAmount;
        }
    }

    // Calculate bar position (bottom center, 50% of viewport width)
    float BarW = ScreenW * 0.5f;
    float BarScale = BarW / BossFrameWidth;  // Scale to fit 50% of screen width
    float BarH = BossFrameHeight * BarScale;
    float BarX = (ScreenW - BarW) * 0.5f;
    float BarY = (ScreenH - BarH) * 0.85f;

    // 1. Draw frame first (background)
    D2D1_RECT_F FrameRect = D2D1::RectF(BarX, BarY, BarX + BarW, BarY + BarH);
    D2DContext->DrawBitmap(BossFrameBitmap, FrameRect, 1.0f);

    // 2. Draw fill bar on top (scaled by health percentage)
    // Use additive blending to make it brighter
    if (DisplayedBossHealth > 0.0f)
    {
        float FillW = BossBarWidth * BarScale * DisplayedBossHealth;
        D2D1_RECT_F FillDestRect = D2D1::RectF(BarX, BarY, BarX + FillW, BarY + BarH);
        D2D1_RECT_F FillSrcRect = D2D1::RectF(0, 0, BossBarWidth * DisplayedBossHealth, BossBarHeight);

        // Switch to additive blending for brighter appearance
        D2DContext->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_ADD);

        D2DContext->DrawBitmap(BossBarBitmap, FillDestRect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, &FillSrcRect);

        // Restore normal blending
        D2DContext->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_SOURCE_OVER);
    }

    // 3. Draw boss name above bar (only if boss is registered)
    if (GS->HasActiveBoss())
    {
        FWideString BossName = UTF8ToWide(GS->GetBossName());
        D2D1_RECT_F NameRect = D2D1::RectF(BarX, BarY - 40.0f, BarX + BarW, BarY);
        D2DContext->DrawTextW(BossName.c_str(), static_cast<UINT32>(BossName.length()),
            BossNameFormat, NameRect, SubtitleBrush);  // Use white subtitle brush for boss name
    }
}

void UGameOverlayD2D::Draw()
{
    if (!bInitialized || !SwapChain || !D2DContext || !TitleFormat || !SubtitleFormat || !TextBrush)
    {
        return;
    }

    if (!GWorld)
    {
        return;
    }

    AGameModeBase* GM = GWorld->GetGameMode();
    if (!GM)
    {
        return;
    }

    AGameState* GS = Cast<AGameState>(GM->GetGameState());
    if (!GS)
    {
        return;
    }

    EGameFlowState FlowState = GS->GetGameFlowState();

    // Only draw for specific states
    bool bShouldDraw = (FlowState == EGameFlowState::StartMenu ||
                        FlowState == EGameFlowState::Fighting ||
                        FlowState == EGameFlowState::Victory ||
                        FlowState == EGameFlowState::Defeat);

    if (!bShouldDraw)
    {
        // Reset death screen timer when not in death/victory state
        DeathScreenTimer = 0.0f;
        return;
    }

    // Get viewport dimensions
    // In editor mode, use MainViewport from USlateManager (PIE viewport)
    // In game mode, use GEngine.GameViewport
    FViewport* Viewport = nullptr;

#ifdef _EDITOR
    SViewportWindow* MainViewportWindow = USlateManager::GetInstance().GetMainViewport();
    if (MainViewportWindow)
    {
        Viewport = MainViewportWindow->GetViewport();
    }
#else
    Viewport = GEngine.GameViewport.get();
#endif

    if (!Viewport)
    {
        return;
    }

    float ViewportX = static_cast<float>(Viewport->GetStartX());
    float ViewportY = static_cast<float>(Viewport->GetStartY());
    float ScreenW = static_cast<float>(Viewport->GetSizeX());
    float ScreenH = static_cast<float>(Viewport->GetSizeY());

    // Create D2D bitmap from backbuffer
    IDXGISurface* Surface = nullptr;
    if (FAILED(SwapChain->GetBuffer(0, __uuidof(IDXGISurface), (void**)&Surface)))
    {
        return;
    }

    D2D1_BITMAP_PROPERTIES1 BmpProps = {};
    BmpProps.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    BmpProps.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    BmpProps.dpiX = 96.0f;
    BmpProps.dpiY = 96.0f;
    BmpProps.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

    ID2D1Bitmap1* TargetBmp = nullptr;
    if (FAILED(D2DContext->CreateBitmapFromDxgiSurface(Surface, &BmpProps, &TargetBmp)))
    {
        SafeRelease(Surface);
        return;
    }

    D2DContext->SetTarget(TargetBmp);
    D2DContext->BeginDraw();

    // Apply transform to offset drawing to viewport region
    D2DContext->SetTransform(D2D1::Matrix3x2F::Translation(ViewportX, ViewportY));

    // Set clip rect to viewport bounds
    D2DContext->PushAxisAlignedClip(
        D2D1::RectF(0, 0, ScreenW, ScreenH),
        D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    // Get delta time for animations
    float DeltaTime = UUIManager::GetInstance().GetDeltaTime();

    // Draw based on current state
    switch (FlowState)
    {
    case EGameFlowState::StartMenu:
        DrawStartMenu(ScreenW, ScreenH);
        break;

    case EGameFlowState::Fighting:
        DrawBossHealthBar(ScreenW, ScreenH, DeltaTime);
        break;

    case EGameFlowState::Defeat:
        DrawDeathScreen(ScreenW, ScreenH, L"YOU DIED", false);
        break;

    case EGameFlowState::Victory:
        DrawDeathScreen(ScreenW, ScreenH, L"DEMIGOD FELLED", true);
        break;

    default:
        break;
    }

    // Pop clip and reset transform
    D2DContext->PopAxisAlignedClip();
    D2DContext->SetTransform(D2D1::Matrix3x2F::Identity());

    D2DContext->EndDraw();
    D2DContext->SetTarget(nullptr);

    SafeRelease(TargetBmp);
    SafeRelease(Surface);
}
