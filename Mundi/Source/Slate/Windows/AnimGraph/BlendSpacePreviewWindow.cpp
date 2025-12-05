#include "pch.h"
#include "BlendSpacePreviewWindow.h"
#include "Source/Editor/FBX/BlendSpace/BlendSpace2D.h"
#include "Source/Runtime/Engine/SkeletalViewer/SkeletalViewerBootstrap.h"
#include "Source/Runtime/Engine/GameFramework/SkeletalMeshActor.h"
#include "Source/Runtime/Engine/GameFramework/CameraActor.h"
#include "Source/Runtime/Engine/Components/SkeletalMeshComponent.h"
#include "Source/Runtime/Engine/Animation/AnimSequence.h"
#include "Source/Runtime/AssetManagement/ResourceManager.h"
#include "Source/Runtime/AssetManagement/SkeletalMesh.h"
#include "Source/Editor/Gizmo/GizmoActor.h"
#include "Source/Runtime/Core/Misc/PathUtils.h"
#include "FViewport.h"
#include "FViewportClient.h"
#include "RenderManager.h"
#include "Renderer.h"
#include "Source/Runtime/RHI/D3D11RHI.h"
#include <filesystem>

BlendSpacePreviewWindow::BlendSpacePreviewWindow()
{
}

BlendSpacePreviewWindow::~BlendSpacePreviewWindow()
{
    DestroyViewerState();
}

bool BlendSpacePreviewWindow::Initialize(UBlendSpace2D* InBlendSpace, UWorld* InWorld, ID3D11Device* InDevice, const FString& InMeshPath)
{
    BlendSpace = InBlendSpace;
    World = InWorld;
    Device = InDevice;

    // 기본 창 크기 설정
    SetRect(100, 100, 700, 700);

    // ViewportRect는 첫 프레임에 OnRender()에서 설정됨 (0,0,0,0으로 시작하여 첫 프레임 렌더링 스킵)
    ViewportRect = FRect(0, 0, 0, 0);
    ViewportRect.UpdateMinMax();

    // 뷰어 상태 생성
    CreateViewerState();

    // 전달받은 메시 경로로 로드
    if (PreviewState && PreviewState->PreviewActor && !InMeshPath.empty())
    {
        UE_LOG("BlendSpacePreview: Loading mesh from path: %s", InMeshPath.c_str());
        USkeletalMesh* Mesh = UResourceManager::GetInstance().Load<USkeletalMesh>(InMeshPath);
        if (Mesh)
        {
            PreviewState->PreviewActor->SetSkeletalMesh(InMeshPath);
            PreviewState->CurrentMesh = Mesh;
            UE_LOG("BlendSpacePreview: Mesh loaded successfully");
        }
        else
        {
            UE_LOG("BlendSpacePreview: Failed to load mesh from %s", InMeshPath.c_str());
        }
    }
    else if (InMeshPath.empty())
    {
        UE_LOG("BlendSpacePreview: No mesh path provided - set Preview Mesh in the node");
    }

    return true;
}

void BlendSpacePreviewWindow::CreateViewerState()
{
    if (!World || !Device) return;

    PreviewState = SkeletalViewerBootstrap::CreateViewerState("BlendSpacePreview", World, Device, false);  // 물리 비활성화
    if (PreviewState && PreviewState->Viewport)
    {
        PreviewState->Viewport->Resize(0, 0, 400, 300);
    }

    // BlendSpace 프리뷰에서는 기즈모 비활성화 (모든 컴포넌트 숨김)
    if (PreviewState && PreviewState->World)
    {
        AGizmoActor* Gizmo = PreviewState->World->GetGizmoActor();
        if (Gizmo)
        {
            // 모든 기즈모 컴포넌트 비활성화
            for (USceneComponent* Comp : Gizmo->GetSceneComponents())
            {
                if (Comp)
                {
                    Comp->SetVisibility(false);
                }
            }
        }
    }

    // 카메라 위치/회전 설정 (메시 앞에서 바라보도록)
    if (PreviewState && PreviewState->Client)
    {
        ACameraActor* Camera = PreviewState->Client->GetCamera();
        if (Camera)
        {
            // 메시 앞에서 바라보는 위치
            Camera->SetActorLocation(FVector(3.0f, 0.0f, 1.0f));
            // SetAnglesImmediate로 Pitch/Yaw 즉시 적용 (메시를 바라보도록)
            Camera->SetAnglesImmediate(-10.0f, 180.0f);  // Pitch -10 (약간 아래), Yaw 180 (-X 방향)
        }
    }
}

void BlendSpacePreviewWindow::DestroyViewerState()
{
    if (PreviewState)
    {
        SkeletalViewerBootstrap::DestroyViewerState(PreviewState);
        PreviewState = nullptr;
    }
}

void BlendSpacePreviewWindow::OnRenderViewport()
{
    if (!bIsOpen || !PreviewState || !PreviewState->Viewport) return;

    if (ViewportRect.GetWidth() > 0 && ViewportRect.GetHeight() > 0)
    {
        const uint32 NewStartX = static_cast<uint32>(ViewportRect.Left);
        const uint32 NewStartY = static_cast<uint32>(ViewportRect.Top);
        const uint32 NewWidth = static_cast<uint32>(ViewportRect.GetWidth());
        const uint32 NewHeight = static_cast<uint32>(ViewportRect.GetHeight());
        PreviewState->Viewport->Resize(NewStartX, NewStartY, NewWidth, NewHeight);

        // 뷰포트 렌더링 (ImGui보다 먼저)
        PreviewState->Viewport->Render();
    }
}

void BlendSpacePreviewWindow::SetBlendSpace(UBlendSpace2D* InBlendSpace)
{
    BlendSpace = InBlendSpace;

    // 파라미터 초기화
    if (BlendSpace)
    {
        FVector2D Min = BlendSpace->GetMinParameter();
        FVector2D Max = BlendSpace->GetMaxParameter();
        CurrentX = (Min.X + Max.X) * 0.5f;
        CurrentY = (Min.Y + Max.Y) * 0.5f;
    }
}

void BlendSpacePreviewWindow::SetPreviewMesh(USkeletalMesh* InMesh)
{
    if (PreviewState && PreviewState->PreviewActor && InMesh)
    {
        PreviewState->PreviewActor->SetSkeletalMesh(InMesh->GetFilePath());
        PreviewState->CurrentMesh = InMesh;
    }
}

void BlendSpacePreviewWindow::OnUpdate(float DeltaSeconds)
{
    if (!bIsOpen) return;

    // ViewportClient Tick (카메라 컨트롤 등)
    if (PreviewState && PreviewState->Client)
    {
        PreviewState->Client->Tick(DeltaSeconds);
    }

    UpdatePreviewPose(DeltaSeconds);
}

void BlendSpacePreviewWindow::UpdatePreviewPose(float DeltaSeconds)
{
    if (!BlendSpace || !PreviewState || !PreviewState->PreviewActor) return;

    // BlendSpace에 현재 파라미터 설정
    BlendSpace->SetParameter(CurrentX, CurrentY);

    // 포즈 평가
    TArray<FTransform> BlendedPose;
    BlendSpace->EvaluatePose(0.0f, DeltaSeconds, BlendedPose);

    // 프리뷰 액터에 포즈 적용
    if (USkeletalMeshComponent* MeshComp = PreviewState->PreviewActor->GetSkeletalMeshComponent())
    {
        // 각 본에 대해 트랜스폼 적용
        for (int32 i = 0; i < BlendedPose.Num(); ++i)
        {
            MeshComp->SetBoneLocalTransform(i, BlendedPose[i]);
        }
    }
}

void BlendSpacePreviewWindow::OnRender()
{
    if (!bIsOpen) return;

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings;

    if (!bInitialPlacementDone)
    {
        ImGui::SetNextWindowPos(ImVec2(Rect.Left, Rect.Top));
        ImGui::SetNextWindowSize(ImVec2(Rect.GetWidth(), Rect.GetHeight()));
        bInitialPlacementDone = true;
    }

    FString WindowTitle = "Blend Space Preview";
    if (BlendSpace)
    {
        WindowTitle += "##BlendSpacePreview";
    }

    bool bWindowVisible = ImGui::Begin(WindowTitle.c_str(), &bIsOpen, flags);
    if (bWindowVisible)
    {
        // 창 영역 계산
        ImVec2 ContentMin = ImGui::GetWindowContentRegionMin();
        ImVec2 ContentMax = ImGui::GetWindowContentRegionMax();

        float ContentHeight = ContentMax.y - ContentMin.y;
        float ViewportPanelHeight = ContentHeight * ViewportRatio;

        // 뷰포트 렌더링 (상단 60%)
        RenderViewport(ViewportPanelHeight);

        ImGui::Dummy(ImVec2(0, 5));

        // BlendSpace 그리드 렌더링 (하단 40%)
        RenderBlendSpaceGrid();
    }
    ImGui::End();

    // 창이 보이지 않으면 ViewportRect 초기화
    if (!bWindowVisible)
    {
        ViewportRect = FRect(0, 0, 0, 0);
        ViewportRect.UpdateMinMax();
    }
}

void BlendSpacePreviewWindow::RenderViewport(float InViewportHeight)
{
    ImGui::BeginChild("BlendSpaceViewport", ImVec2(0, InViewportHeight), true, ImGuiWindowFlags_NoScrollbar);
    {
        bool bIsHovered = ImGui::IsWindowHovered();
        if (bIsHovered)
        {
            // 뷰포트 위에서는 ImGui가 마우스를 캡처하지 않도록 설정
            ImGui::GetIO().WantCaptureMouse = false;
        }

        ImVec2 childPos = ImGui::GetWindowPos();
        ImVec2 childSize = ImGui::GetWindowSize();
        ImVec2 rectMin = childPos;
        ImVec2 rectMax(childPos.x + childSize.x, childPos.y + childSize.y);

        // 뷰포트 영역 업데이트 (SSkeletalMeshViewerWindow와 동일한 패턴)
        ViewportRect.Left = rectMin.x;
        ViewportRect.Top = rectMin.y;
        ViewportRect.Right = rectMax.x;
        ViewportRect.Bottom = rectMax.y;
        ViewportRect.UpdateMinMax();

        // 마우스 입력 처리 (뷰포트 카메라 조작)
        if (PreviewState && PreviewState->Viewport && PreviewState->Client)
        {
            ImVec2 mousePos = ImGui::GetMousePos();
            int32 localX = static_cast<int32>(mousePos.x - ViewportRect.Left);
            int32 localY = static_cast<int32>(mousePos.y - ViewportRect.Top);

            // 우클릭 시작
            if (bIsHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                bViewportRightMouseDown = true;
                LastMouseX = localX;
                LastMouseY = localY;
                PreviewState->Viewport->ProcessMouseButtonDown(localX, localY, 1);
            }

            // 우클릭 해제 (뷰포트 밖에서도 감지)
            if (bViewportRightMouseDown && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
            {
                bViewportRightMouseDown = false;
                PreviewState->Viewport->ProcessMouseButtonUp(localX, localY, 1);
            }

            // 마우스 이동 (우클릭 드래그 중)
            if (bViewportRightMouseDown)
            {
                PreviewState->Viewport->ProcessMouseMove(localX, localY);
                LastMouseX = localX;
                LastMouseY = localY;
            }

            // 마우스 휠 (줌)
            if (bIsHovered)
            {
                float wheelDelta = ImGui::GetIO().MouseWheel;
                if (wheelDelta != 0.0f)
                {
                    PreviewState->Client->MouseWheel(wheelDelta * 0.1f);
                }
            }
        }

        // 렌더러에서 SRV 가져오기
        URenderer* CurrentRenderer = URenderManager::GetInstance().GetRenderer();
        if (CurrentRenderer)
        {
            D3D11RHI* RHIDevice = CurrentRenderer->GetRHIDevice();
            if (RHIDevice)
            {
                uint32 TotalWidth = RHIDevice->GetViewportWidth();
                uint32 TotalHeight = RHIDevice->GetViewportHeight();

                ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
                ImVec2 contentMax = ImGui::GetWindowContentRegionMax();

                // 실제 렌더링 영역의 UV 계산
                float actualLeft = ViewportRect.Left + contentMin.x;
                float actualTop = ViewportRect.Top + contentMin.y;

                ImVec2 uv0(actualLeft / TotalWidth, actualTop / TotalHeight);
                ImVec2 uv1((actualLeft + (contentMax.x - contentMin.x)) / TotalWidth,
                           (actualTop + (contentMax.y - contentMin.y)) / TotalHeight);

                ID3D11ShaderResourceView* SRV = RHIDevice->GetCurrentSourceSRV();
                if (SRV)
                {
                    ImGui::Image((void*)SRV, ImVec2(contentMax.x - contentMin.x, contentMax.y - contentMin.y), uv0, uv1);
                }
            }
        }
    }
    ImGui::EndChild();
}

void BlendSpacePreviewWindow::RenderBlendSpaceGrid()
{
    if (!BlendSpace)
    {
        ImGui::Text("No BlendSpace assigned");
        return;
    }

    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    // 그리드 영역 계산
    float GridSize = FMath::Min(GridRect.GetWidth(), GridRect.GetHeight() - 30);
    if (GridSize < 100) GridSize = 100;

    ImVec2 GridMin = ImGui::GetCursorScreenPos();
    ImVec2 GridMax = ImVec2(GridMin.x + GridSize, GridMin.y + GridSize);

    // 그리드 배경
    DrawList->AddRectFilled(GridMin, GridMax, IM_COL32(30, 30, 30, 255), 4.0f);
    DrawList->AddRect(GridMin, GridMax, IM_COL32(100, 100, 100, 255), 4.0f);

    // 중심선
    float CenterX = GridMin.x + GridSize * 0.5f;
    float CenterY = GridMin.y + GridSize * 0.5f;
    DrawList->AddLine(ImVec2(CenterX, GridMin.y), ImVec2(CenterX, GridMax.y), IM_COL32(60, 60, 60, 255));
    DrawList->AddLine(ImVec2(GridMin.x, CenterY), ImVec2(GridMax.x, CenterY), IM_COL32(60, 60, 60, 255));

    // 파라미터 범위
    FVector2D MinParam = BlendSpace->GetMinParameter();
    FVector2D MaxParam = BlendSpace->GetMaxParameter();
    float RangeX = MaxParam.X - MinParam.X;
    float RangeY = MaxParam.Y - MinParam.Y;
    if (RangeX <= 0) RangeX = 1.0f;
    if (RangeY <= 0) RangeY = 1.0f;

    // 샘플 포인트 그리기
    const TArray<FBlendSample2D>& Samples = BlendSpace->GetSamples();
    for (int32 i = 0; i < Samples.Num(); ++i)
    {
        const FBlendSample2D& Sample = Samples[i];
        float NormX = (Sample.Position.X - MinParam.X) / RangeX;
        float NormY = (Sample.Position.Y - MinParam.Y) / RangeY;

        float ScreenX = GridMin.x + NormX * GridSize;
        float ScreenY = GridMax.y - NormY * GridSize;  // Y 반전

        DrawList->AddCircleFilled(ImVec2(ScreenX, ScreenY), 6.0f, IM_COL32(200, 200, 200, 255));
        DrawList->AddCircle(ImVec2(ScreenX, ScreenY), 6.0f, IM_COL32(0, 0, 0, 255));
    }

    // 삼각형 그리기
    const TArray<FBlendTriangle>& Triangles = BlendSpace->GetTriangles();
    for (const FBlendTriangle& Tri : Triangles)
    {
        if (!Tri.IsValid()) continue;
        if (Tri.Indices[0] >= Samples.Num() ||
            Tri.Indices[1] >= Samples.Num() ||
            Tri.Indices[2] >= Samples.Num())
            continue;

        ImVec2 Points[3];
        for (int32 j = 0; j < 3; ++j)
        {
            const FVector2D& Pos = Samples[Tri.Indices[j]].Position;
            float NormX = (Pos.X - MinParam.X) / RangeX;
            float NormY = (Pos.Y - MinParam.Y) / RangeY;
            Points[j].x = GridMin.x + NormX * GridSize;
            Points[j].y = GridMax.y - NormY * GridSize;
        }

        DrawList->AddTriangle(Points[0], Points[1], Points[2], IM_COL32(80, 180, 80, 200), 1.0f);
    }

    // 현재 파라미터 위치 (빨간 십자)
    float NormCurX = (CurrentX - MinParam.X) / RangeX;
    float NormCurY = (CurrentY - MinParam.Y) / RangeY;
    float CursorX = GridMin.x + NormCurX * GridSize;
    float CursorY = GridMax.y - NormCurY * GridSize;

    // 십자 표시
    float CrossSize = 10.0f;
    DrawList->AddLine(ImVec2(CursorX - CrossSize, CursorY), ImVec2(CursorX + CrossSize, CursorY), IM_COL32(255, 50, 50, 255), 2.0f);
    DrawList->AddLine(ImVec2(CursorX, CursorY - CrossSize), ImVec2(CursorX, CursorY + CrossSize), IM_COL32(255, 50, 50, 255), 2.0f);

    // 그리드 영역 확보
    ImGui::Dummy(ImVec2(GridSize, GridSize));

    // Ctrl+마우스 드래그로 파라미터 조작
    ImVec2 MousePos = ImGui::GetMousePos();
    bool bMouseInGrid = (MousePos.x >= GridMin.x && MousePos.x <= GridMax.x &&
                         MousePos.y >= GridMin.y && MousePos.y <= GridMax.y);

    if (bMouseInGrid && ImGui::GetIO().KeyCtrl)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            // 마우스 위치를 파라미터로 변환
            float NormMouseX = (MousePos.x - GridMin.x) / GridSize;
            float NormMouseY = 1.0f - (MousePos.y - GridMin.y) / GridSize;  // Y 반전

            CurrentX = MinParam.X + NormMouseX * RangeX;
            CurrentY = MinParam.Y + NormMouseY * RangeY;

            // 범위 클램프
            CurrentX = FMath::Clamp(CurrentX, MinParam.X, MaxParam.X);
            CurrentY = FMath::Clamp(CurrentY, MinParam.Y, MaxParam.Y);
        }

        // 커서 변경 힌트
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    // 현재 파라미터 값 표시
    ImGui::Text("Parameter: (%.1f, %.1f)", CurrentX, CurrentY);
    ImGui::TextDisabled("Ctrl + Drag to move cursor");
}

void BlendSpacePreviewWindow::OnMouseDown(FVector2D MousePos, uint32 Button)
{
    if (!PreviewState || !PreviewState->Viewport) return;

    // 뷰포트 영역 내부인지 확인
    if (ViewportRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(ViewportRect.Left, ViewportRect.Top);

        if (Button == 1)  // 우클릭
        {
            bViewportRightMouseDown = true;
            LastMouseX = static_cast<int32>(LocalPos.X);
            LastMouseY = static_cast<int32>(LocalPos.Y);
        }

        PreviewState->Viewport->ProcessMouseButtonDown(
            static_cast<int32>(LocalPos.X),
            static_cast<int32>(LocalPos.Y),
            static_cast<int32>(Button));
    }
}

void BlendSpacePreviewWindow::OnMouseUp(FVector2D MousePos, uint32 Button)
{
    if (!PreviewState || !PreviewState->Viewport) return;

    FVector2D LocalPos = MousePos - FVector2D(ViewportRect.Left, ViewportRect.Top);

    if (Button == 1)  // 우클릭
    {
        bViewportRightMouseDown = false;
    }

    PreviewState->Viewport->ProcessMouseButtonUp(
        static_cast<int32>(LocalPos.X),
        static_cast<int32>(LocalPos.Y),
        static_cast<int32>(Button));
}

void BlendSpacePreviewWindow::OnMouseMove(FVector2D MousePos)
{
    if (!PreviewState || !PreviewState->Viewport) return;

    // 뷰포트 영역 내부이거나 우클릭 드래그 중일 때
    if (ViewportRect.Contains(MousePos) || bViewportRightMouseDown)
    {
        FVector2D LocalPos = MousePos - FVector2D(ViewportRect.Left, ViewportRect.Top);
        PreviewState->Viewport->ProcessMouseMove(
            static_cast<int32>(LocalPos.X),
            static_cast<int32>(LocalPos.Y));
    }
}

FString BlendSpacePreviewWindow::FindSkeletalMeshFromAnimPath(const FString& AnimPath)
{
    namespace fs = std::filesystem;

    // AnimPath 예시: "Data/FBX/ShinobiGirlAkane/Animation/Combo_Attack_01_01_Seq.FBX_Unreal Take"
    // 목표: "Data/FBX/ShinobiGirlAkane/SK_AKANE_NoMask.FBX" 찾기

    // 1. 애니메이션 이름 부분 제거 (마지막 _)
    size_t UnderscorePos = AnimPath.find_last_of('_');
    if (UnderscorePos == FString::npos)
    {
        return FString();
    }
    FString FbxPath = AnimPath.substr(0, UnderscorePos);  // "Data/FBX/.../Combo_Attack_01_01_Seq.FBX"

    // 2. filesystem::path로 변환
    FWideString WFbxPath = UTF8ToWide(FbxPath);
    fs::path FsPath(WFbxPath);

    // 3. "Animation" 폴더 상위로 이동
    fs::path ParentDir = FsPath.parent_path();  // "Data/FBX/ShinobiGirlAkane/Animation"

    // Animation 폴더인지 확인
    if (ParentDir.filename().wstring() == L"Animation")
    {
        ParentDir = ParentDir.parent_path();  // "Data/FBX/ShinobiGirlAkane"
    }

    // 4. 해당 폴더에서 SK_ 패턴 FBX 파일 찾기
    if (fs::exists(ParentDir) && fs::is_directory(ParentDir))
    {
        for (const auto& Entry : fs::directory_iterator(ParentDir))
        {
            if (!Entry.is_regular_file()) continue;

            fs::path EntryPath = Entry.path();
            FString Extension = WideToUTF8(EntryPath.extension().wstring());

            // 대소문자 무시 비교
            std::transform(Extension.begin(), Extension.end(), Extension.begin(), ::tolower);
            if (Extension != ".fbx") continue;

            FString Filename = WideToUTF8(EntryPath.filename().wstring());

            // SK_ 또는 SKM_ 패턴 확인
            if (Filename.find("SK_") == 0 || Filename.find("SKM_") == 0)
            {
                return WideToUTF8(EntryPath.wstring());
            }
        }
    }

    return FString();
}
