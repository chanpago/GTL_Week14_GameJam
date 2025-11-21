#include "pch.h"
#include "ParticleViewerWindow.h"
#include "Source/Runtime/Renderer/FViewport.h"
#include "Source/Runtime/Renderer/FViewportClient.h"
#include "Source/Runtime/Engine/Particle/ParticleSystem.h"
#include "Source/Runtime/Engine/Particle/ParticleEmitter.h"
#include "Source/Runtime/Engine/Particle/ParticleLODLevel.h"
#include "Source/Runtime/Engine/Particle/Modules/ParticleModule.h"
#include "imgui.h"

SParticleViewerWindow::SParticleViewerWindow()
{
}

SParticleViewerWindow::~SParticleViewerWindow()
{
    if (Viewport)
    {
        delete Viewport;
        Viewport = nullptr;
    }

    if (ViewportClient)
    {
        delete ViewportClient;
        ViewportClient = nullptr;
    }
}

bool SParticleViewerWindow::Initialize(float StartX, float StartY, float Width, float Height, UWorld* InWorld, ID3D11Device* InDevice)
{
    World = InWorld;
    Device = InDevice;

    // 뷰포트 생성 (나중에 파티클 렌더링용)
    // Viewport = new FViewport(...);
    // ViewportClient = new FParticleViewerViewportClient(...);

    return true;
}

void SParticleViewerWindow::OnRender()
{
    if (!bIsOpen)
        return;

    // ImGui 창 설정
    ImGui::SetNextWindowSize(ImVec2(1400, 900), ImGuiCond_FirstUseEver);
    if (!bInitialPlacementDone)
    {
        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
        bInitialPlacementDone = true;
    }

    ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
    if (!ImGui::Begin("Particle Editor", &bIsOpen, flags))
    {
        ImGui::End();
        return;
    }

    // 1. 메뉴바
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save")) {}
            if (ImGui::MenuItem("Close")) { Close(); }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo")) {}
            if (ImGui::MenuItem("Redo")) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Asset"))
        {
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window"))
        {
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // 2. 툴바
    const float toolbarHeight = 40.0f;
    ImGui::BeginChild("Toolbar", ImVec2(0, toolbarHeight), true);
    {
        if (ImGui::Button("Save")) {}
        ImGui::SameLine();
        if (ImGui::Button("Restart Sim")) {}
        ImGui::SameLine();
        if (ImGui::Button("Undo")) {}
        ImGui::SameLine();
        if (ImGui::Button("Redo")) {}
        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();
        if (ImGui::Button("Thumbnail")) {}
        ImGui::SameLine();
        if (ImGui::Button("Bounds")) {}
    }
    ImGui::EndChild();

    // 3. 메인 컨텐츠 영역
    ImVec2 contentSize = ImGui::GetContentRegionAvail();
    const float curveEditorHeight = contentSize.y * 0.3f;
    const float mainContentHeight = contentSize.y - curveEditorHeight;
    const float leftPanelWidth = contentSize.x * 0.4f;
    const float rightPanelWidth = contentSize.x - leftPanelWidth;

    // 좌측: 뷰포트 + Properties
    ImGui::BeginChild("LeftMain", ImVec2(leftPanelWidth, mainContentHeight), false);
    {
        const float viewportHeight = mainContentHeight * 0.6f;
        const float propertiesHeight = mainContentHeight - viewportHeight;

        // 뷰포트
        ImGui::BeginChild("Viewport", ImVec2(0, viewportHeight), true);
        {
            ImGui::Text("Viewport (3D Preview)");

            // 뷰포트 영역 계산
            ImVec2 viewportMin = ImGui::GetCursorScreenPos();
            ImVec2 viewportSize = ImGui::GetContentRegionAvail();
            ImVec2 viewportMax = ImVec2(viewportMin.x + viewportSize.x, viewportMin.y + viewportSize.y);
            CenterRect = FRect(viewportMin.x, viewportMin.y, viewportMax.x, viewportMax.y);
        }
        ImGui::EndChild();

        // Properties
        ImGui::BeginChild("Properties", ImVec2(0, propertiesHeight), true);
        {
            ImGui::Text("Properties");
            ImGui::Separator();

            if (CurrentParticleSystem)
            {
                ImGui::Text("Particle System: %s", CurrentParticleSystem->GetName());
                ImGui::Spacing();

                // System properties
                if (ImGui::CollapsingHeader("System Properties", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Text("System Update Mode: EPSUM_RealTime");
                    ImGui::Text("Update Time FPS: 60.000000");
                    ImGui::Text("Warmup Time: 0.000000");
                }

                if (ImGui::CollapsingHeader("Thumbnail"))
                {
                    ImGui::Text("Thumbnail Warmup: 1.000000");
                }

                if (ImGui::CollapsingHeader("LOD"))
                {
                    ImGui::Text("LOD Distance Check Time: 0.250000");
                    ImGui::Text("LOD Method: PARTICLESYSTEMLODMETHOD_Automatic");
                }
            }
            else
            {
                ImGui::TextDisabled("No particle system loaded");
            }
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // 우측: Emitter/모듈 패널
    ImGui::BeginChild("EmitterPanel", ImVec2(rightPanelWidth, mainContentHeight), true);
    {
        ImGui::Text("Emitters");
        ImGui::Separator();

        if (CurrentParticleSystem && CurrentParticleSystem->Emitters.Num() > 0)
        {
            // Emitter 탭 형태로 표시
            for (int i = 0; i < CurrentParticleSystem->Emitters.Num(); i++)
            {
                UParticleEmitter* Emitter = CurrentParticleSystem->Emitters[i];
                if (!Emitter) continue;

                ImGui::PushID(i);
                if (ImGui::CollapsingHeader(Emitter->GetName().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                {
                    // LOD Level 정보
                    if (Emitter->LODLevels.Num() > 0)
                    {
                        UParticleLODLevel* LOD = Emitter->LODLevels[0];
                        if (LOD)
                        {
                            ImGui::Indent();

                            // 모듈 리스트 (AllModulesCache 사용)
                            for (int m = 0; m < LOD->AllModulesCache.Num(); m++)
                            {
                                UParticleModule* Module = LOD->AllModulesCache[m];
                                if (Module)
                                {
                                    ImGui::Selectable(Module->GetClass()->Name);
                                }
                            }

                            ImGui::Unindent();
                        }
                    }
                }
                ImGui::PopID();
            }
        }
        else
        {
            ImGui::TextDisabled("No emitters");
        }
    }
    ImGui::EndChild();

    // 4. 하단: Curve Editor
    ImGui::BeginChild("CurveEditor", ImVec2(0, curveEditorHeight), true);
    {
        ImGui::Text("Curve Editor");
        ImGui::Separator();

        // 커브 편집 툴바
        if (ImGui::Button("Horizontal")) {}
        ImGui::SameLine();
        if (ImGui::Button("Vertical")) {}
        ImGui::SameLine();
        if (ImGui::Button("All")) {}
        ImGui::SameLine();
        if (ImGui::Button("Selected")) {}
        ImGui::SameLine();
        if (ImGui::Button("Pan")) {}
        ImGui::SameLine();
        if (ImGui::Button("Zoom")) {}

        ImGui::Separator();
        ImGui::TextDisabled("Curve editing area (Coming Soon)");
    }
    ImGui::EndChild();

    ImGui::End();
}

void SParticleViewerWindow::OnUpdate(float DeltaSeconds)
{
    // 파티클 시스템 업데이트 (나중에 구현)
}

void SParticleViewerWindow::OnMouseMove(FVector2D MousePos)
{
}

void SParticleViewerWindow::OnMouseDown(FVector2D MousePos, uint32 Button)
{
}

void SParticleViewerWindow::OnMouseUp(FVector2D MousePos, uint32 Button)
{
}

void SParticleViewerWindow::OnRenderViewport()
{
    // 뷰포트 렌더링 (나중에 구현)
}

void SParticleViewerWindow::LoadParticleSystem(const FString& Path)
{
    // TODO: 경로에서 파티클 시스템 로드
}

void SParticleViewerWindow::LoadParticleSystem(UParticleSystem* ParticleSystem)
{
    CurrentParticleSystem = ParticleSystem;
}