#pragma once

#include "Source/Slate/Windows/SWindow.h"
#include "Source/Runtime/Engine/SkeletalViewer/ViewerState.h"

class UBlendSpace2D;
class USkeletalMesh;
class UWorld;
class FViewport;
class FViewportClient;
struct ID3D11Device;

/**
 * @brief Blend Space 2D 프리뷰 창
 * - 상단: 스켈레탈 메시 3D 프리뷰
 * - 하단: 2D 그리드 + 파라미터 조작
 * - Ctrl+마우스로 블렌드 파라미터 실시간 조절
 */
class BlendSpacePreviewWindow : public SWindow
{
public:
    BlendSpacePreviewWindow();
    virtual ~BlendSpacePreviewWindow();

    /**
     * @brief 창 초기화
     * @param InBlendSpace 프리뷰할 BlendSpace2D
     * @param InWorld 월드 (뷰포트용)
     * @param InDevice D3D 디바이스
     */
    bool Initialize(UBlendSpace2D* InBlendSpace, UWorld* InWorld, ID3D11Device* InDevice);

    // SWindow overrides
    virtual void OnRender() override;
    virtual void OnUpdate(float DeltaSeconds) override;

    /** @brief 뷰포트 렌더링 전 호출 - viewport region 설정 */
    void OnRenderViewport();

    // 창 상태
    bool IsOpen() const { return bIsOpen; }
    void Close() { bIsOpen = false; }

    // BlendSpace 설정
    void SetBlendSpace(UBlendSpace2D* InBlendSpace);
    UBlendSpace2D* GetBlendSpace() const { return BlendSpace; }

    // 프리뷰 메시 설정
    void SetPreviewMesh(USkeletalMesh* InMesh);

    // 마우스 이벤트 (카메라 조작용)
    void OnMouseDown(FVector2D MousePos, uint32 Button);
    void OnMouseUp(FVector2D MousePos, uint32 Button);
    void OnMouseMove(FVector2D MousePos);

    // 뷰포트 영역 반환
    const FRect& GetViewportRect() const { return ViewportRect; }

private:
    // 렌더링 함수들
    void RenderViewport(float InViewportHeight);
    void RenderBlendSpaceGrid();
    void UpdatePreviewPose(float DeltaSeconds);

    // 뷰어 상태 생성/파괴
    void CreateViewerState();
    void DestroyViewerState();

    // 애니메이션 경로에서 스켈레탈 메시 경로 찾기
    FString FindSkeletalMeshFromAnimPath(const FString& AnimPath);

private:
    // 창 상태
    bool bIsOpen = true;
    bool bInitialPlacementDone = false;

    // BlendSpace 데이터
    UBlendSpace2D* BlendSpace = nullptr;

    // 현재 파라미터 위치 (Ctrl+마우스로 조작)
    float CurrentX = 0.0f;
    float CurrentY = 0.0f;
    bool bIsDraggingParameter = false;

    // 뷰어 상태 (3D 프리뷰용)
    ViewerState* PreviewState = nullptr;
    UWorld* World = nullptr;
    ID3D11Device* Device = nullptr;

    // 레이아웃
    float ViewportRatio = 0.6f;  // 상단 뷰포트 비율
    FRect ViewportRect;
    FRect GridRect;

    // 마우스 입력 상태 (뷰포트 카메라 조작용)
    bool bViewportRightMouseDown = false;
    int32 LastMouseX = 0;
    int32 LastMouseY = 0;
};
