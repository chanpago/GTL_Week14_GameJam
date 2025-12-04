#include "pch.h"
#include "ImGui/imgui_stdlib.h"
#include "imgui-node-editor/imgui_node_editor.h"
#include "K2Node_Animation.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintEvaluator.h"
#include "Source/Editor/FBX/BlendSpace/BlendSpace1D.h"
#include "Source/Editor/FBX/BlendSpace/BlendSpace2D.h"
#include "Source/Runtime/Engine/Animation/AnimInstance.h"

namespace ed = ax::NodeEditor;

// ----------------------------------------------------------------
//	[AnimSequence] 애니메이션 시퀀스 노드 
// ----------------------------------------------------------------

IMPLEMENT_CLASS(UK2Node_AnimSequence)

UK2Node_AnimSequence::UK2Node_AnimSequence()
{
    TitleColor = ImColor(100, 120, 255);
}

void UK2Node_AnimSequence::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    UK2Node::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        FString AnimPath;
        if (FJsonSerializer::ReadString(InOutHandle, "AnimPath", AnimPath))
        {
            if (!AnimPath.empty())
            {
                Value = RESOURCE.Get<UAnimSequence>(AnimPath);
            }
        }
    }
    else
    {
        if (Value)
        {
            InOutHandle["AnimPath"] = Value->GetFilePath();
        }
    }
}

void UK2Node_AnimSequence::AllocateDefaultPins()
{
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::AnimSequence, "Value");
}

/**
 * @note ed::Suspend를 활용하지 않으면 스크린 공간이 아닌 캔버스 공간 좌표가 사용되어서
 * 정상적으로 창이 뜨지 않는 이슈 존재가 존재했었다. 현재는 해결되었다.
 */
void UK2Node_AnimSequence::RenderBody()
{
    FString PopupID = "##AnimSelectPopup_";
    PopupID += std::to_string(NodeID);

    FString PreviewName = "None";
    if (Value)
    {
        PreviewName = Value->GetFilePath();
    }

    if (ImGui::Button(PreviewName.c_str()))
    {
        ed::Suspend();
        ImGui::OpenPopup(PopupID.c_str());
        ed::Resume();
    }

    if (ImGui::IsPopupOpen(PopupID.c_str()))
    {
        ed::Suspend();
        
        if (ImGui::BeginPopup(PopupID.c_str()))
        {
            TArray<UAnimSequence*> AnimSequences = RESOURCE.GetAll<UAnimSequence>();

            for (UAnimSequence* Anim : AnimSequences)
            {
                if (!Anim) continue;

                const FString AssetName = Anim->GetFilePath();
                bool bIsSelected = (Value == Anim);
                  
                if (ImGui::Selectable(AssetName.c_str(), bIsSelected))
                {
                    Value = Anim;
                    // LoadMeta 호출 제거 - GetAnimNotifyEvents()가 이미 lazy loading을 수행함
                    // 불필요한 LoadMeta 호출은 메모리의 노티파이를 파일 데이터로 덮어씀
                    ImGui::CloseCurrentPopup();
                }

                if (bIsSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            
            ImGui::EndPopup();
        }
        ed::Resume();
    }
}

FBlueprintValue UK2Node_AnimSequence::EvaluatePin(const UEdGraphPin* OutputPin, FBlueprintContext* Context)
{
    if (OutputPin->PinName == "Value")
    {
        return Value;
    }
    return static_cast<UAnimSequence*>(nullptr);
}

// ----------------------------------------------------------------
//	[AnimStateEntry] 애니메이션 상태 머신 진입점
// ----------------------------------------------------------------

IMPLEMENT_CLASS(UK2Node_AnimStateEntry)

UK2Node_AnimStateEntry::UK2Node_AnimStateEntry()
{
    TitleColor = ImColor(150, 150, 150);
}

void UK2Node_AnimStateEntry::AllocateDefaultPins()
{
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::Exec, "Entry"); 
}

void UK2Node_AnimStateEntry::RenderBody()
{
}

void UK2Node_AnimStateEntry::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());

    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();

    ActionRegistrar.AddAction(Spawner);
}

void UK2Node_AnimSequence::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());

    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();

    ActionRegistrar.AddAction(Spawner);
}

// ----------------------------------------------------------------
//	[AnimState] 애니메이션 상태 노드
// ----------------------------------------------------------------

IMPLEMENT_CLASS(UK2Node_AnimState)

UK2Node_AnimState::UK2Node_AnimState()
{
    TitleColor = ImColor(200, 100, 100);
}

void UK2Node_AnimState::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    UK2Node::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        FJsonSerializer::ReadString(InOutHandle, "StateName", StateName);
    }
    else
    {
        InOutHandle["StateName"] = StateName;
    }
}

void UK2Node_AnimState::AllocateDefaultPins()
{
    // 상태 머신 그래프의 흐름(Flow)을 위한 Exec 핀
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Exec, "Enter");
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::Exec, "Exit");

    // FAnimationState의 멤버에 해당하는 핀들
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::AnimSequence, "Animation");
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Bool, "Looping", "false");
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Float, "PlayRate", "1.0");
}

void UK2Node_AnimState::RenderBody()
{
    ImGui::PushItemWidth(150.0f);
    ImGui::InputText("상태 이름", &StateName);
    ImGui::PopItemWidth();
}

void UK2Node_AnimState::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());

    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();

    ActionRegistrar.AddAction(Spawner);
}

// ----------------------------------------------------------------
//	[AnimTransition] 애니메이션 전이 노드
// ----------------------------------------------------------------

IMPLEMENT_CLASS(UK2Node_AnimTransition)

UK2Node_AnimTransition::UK2Node_AnimTransition()
{
    TitleColor = ImColor(100, 100, 200);
}

void UK2Node_AnimTransition::AllocateDefaultPins()
{
    // 상태 머신 그래프의 흐름(Flow)을 위한 Exec 핀
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Exec, "Execute"); 
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::Exec, "Transition To");
    
    // FStateTransition의 멤버에 해당하는 핀들
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Bool, "Can Transition");
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Float, "Blend Time");
}

void UK2Node_AnimTransition::RenderBody()
{
}

void UK2Node_AnimTransition::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());

    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();

    ActionRegistrar.AddAction(Spawner);
}

// ----------------------------------------------------------------
//	[BlendSpace1D] 1D 블렌드 스페이스 노드
// ----------------------------------------------------------------

IMPLEMENT_CLASS(UK2Node_BlendSpace1D)

UK2Node_BlendSpace1D::UK2Node_BlendSpace1D()
{
    TitleColor = ImColor(100, 200, 100);

    // 기본 3개 샘플 슬롯 생성
    SampleAnimations.SetNum(3);
    SamplePositions.SetNum(3);
    SamplePositions[0] = 0.0f;
    SamplePositions[1] = 100.0f;
    SamplePositions[2] = 200.0f;

    // BlendSpace 객체 생성
    BlendSpace = NewObject<UBlendSpace1D>();
}

void UK2Node_BlendSpace1D::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    UK2Node::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        FJsonSerializer::ReadFloat(InOutHandle, "MinRange", MinRange);
        FJsonSerializer::ReadFloat(InOutHandle, "MaxRange", MaxRange);
        
        // 샘플 개수 로드
        int32 NumSamples = 3;
        FJsonSerializer::ReadInt32(InOutHandle, "NumSamples", NumSamples);

        SampleAnimations.SetNum(NumSamples);
        SamplePositions.SetNum(NumSamples);

        // 각 샘플 로드
        for (int32 i = 0; i < NumSamples; ++i)
        {
            FString AnimKey = "SampleAnim_" + std::to_string(i);
            FString PosKey = "SamplePos_" + std::to_string(i);

            FString AnimPath;
            if (FJsonSerializer::ReadString(InOutHandle, AnimKey, AnimPath) && !AnimPath.empty())
            {
                SampleAnimations[i] = RESOURCE.Get<UAnimSequence>(AnimPath);
            }

            FJsonSerializer::ReadFloat(InOutHandle, PosKey, SamplePositions[i]);
        }

        RebuildBlendSpace();
    }
    else
    {
        InOutHandle["MinRange"] = MinRange;
        InOutHandle["MaxRange"] = MaxRange;
        
        // 샘플 개수 저장
        InOutHandle["NumSamples"] = SampleAnimations.Num();

        // 각 샘플 저장
        for (int32 i = 0; i < SampleAnimations.Num(); ++i)
        {
            FString AnimKey = "SampleAnim_" + std::to_string(i);
            FString PosKey = "SamplePos_" + std::to_string(i);

            if (SampleAnimations[i])
            {
                InOutHandle[AnimKey] = SampleAnimations[i]->GetFilePath();
            }
            InOutHandle[PosKey] = SamplePositions[i];
        }
    }
}

void UK2Node_BlendSpace1D::AllocateDefaultPins()
{
    // 입력: 파라미터 (예: Speed)
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Float, "Parameter", "0.0");

    // 출력: 블렌드된 포즈
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::AnimSequence, "Output");
}

void UK2Node_BlendSpace1D::RenderBody()
{
    // -------------------------------------------------------------------------
    // 0. 데이터 및 객체 무결성 검사
    // -------------------------------------------------------------------------
    if (!BlendSpace)
    {
        BlendSpace = NewObject<UBlendSpace1D>();
    }

    if (SamplePositions.Num() == 0 || SamplePositions.Num() != SampleAnimations.Num())
    {
        SamplePositions = { 0.0f, 100.0f };
        SampleAnimations.SetNum(2); 
        SampleAnimations[0] = nullptr; SampleAnimations[1] = nullptr;
    }

    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    
    // -------------------------------------------------------------------------
    // 1. Range (Min/Max) 설정 UI
    // -------------------------------------------------------------------------
    // BlendSpace 객체에서 현재 범위를 가져옵니다.
    bool bRangeChanged = false;

    ImGui::PushItemWidth(60.0f); // 입력 필드 너비 고정
    
    ImGui::Text("Min"); ImGui::SameLine();
    if (ImGui::DragFloat("##MinRange", &MinRange, 1.0f)) bRangeChanged = true;
    
    ImGui::SameLine(); 
    ImGui::Text("Max"); ImGui::SameLine();
    if (ImGui::DragFloat("##MaxRange", &MaxRange, 1.0f)) bRangeChanged = true;

    ImGui::PopItemWidth();

    // 값이 변경되었다면 BlendSpace에 적용하고, Max가 Min보다 작아지지 않도록 방어
    if (bRangeChanged)
    {
        if (MaxRange < MinRange) MaxRange = MinRange + 10.0f;
        BlendSpace->SetParameterRange(MinRange, MaxRange);
    }

    // -------------------------------------------------------------------------
    // 2. 상단 컨트롤 (Add 버튼)
    // -------------------------------------------------------------------------
    float CanvasWidth = ImGui::GetContentRegionAvail().x;
    if (CanvasWidth < 200.0f) CanvasWidth = 200.0f;

    // Add 버튼 (현재 범위의 중간값에 추가)
    if (ImGui::Button("+ Add Sample", ImVec2(CanvasWidth, 0)))
    {
        float MidPos = MinRange + (MaxRange - MinRange) * 0.5f;
        SamplePositions.Add(MidPos);
        SampleAnimations.Add(nullptr);
        RebuildBlendSpace();
    }
    
    ImGui::Dummy(ImVec2(0, 4.0f)); // 간격

    // -------------------------------------------------------------------------
    // 3. 타임라인 바 그리기
    // -------------------------------------------------------------------------
    ImVec2 CursorPos = ImGui::GetCursorScreenPos();
    float BarHeight = 24.0f;
    
    ImVec2 BarMin = CursorPos;
    ImVec2 BarMax = ImVec2(BarMin.x + CanvasWidth, BarMin.y + BarHeight);
    
    // 바 배경 (어두운 회색)
    DrawList->AddRectFilled(BarMin, BarMax, IM_COL32(30, 30, 30, 255), 4.0f);
    DrawList->AddRect(BarMin, BarMax, IM_COL32(100, 100, 100, 255), 4.0f);

    // 공간 확보 (다음 위젯이 겹치지 않도록)
    ImGui::Dummy(ImVec2(CanvasWidth, BarHeight));

    bool bNeedRebuild = false;
    float RangeLength = MaxRange - MinRange;
    if (RangeLength <= 0.0f) RangeLength = 1.0f; // 0 나누기 방지

    // -------------------------------------------------------------------------
    // 4. 샘플 포인트 루프
    // -------------------------------------------------------------------------
    for (int32 i = 0; i < SamplePositions.Num(); ++i)
    {
        ImGui::PushID(i);

        // [위치 정규화] 현재 Min/Max 범위에 맞춰 X 좌표 계산
        // 범위 밖으로 나간 샘플도 그릴 것인지, 클램핑할 것인지 결정 (여기선 클램핑)
        float ClampedPos = FMath::Clamp(SamplePositions[i], MinRange, MaxRange);
        float NormalizedPos = (ClampedPos - MinRange) / RangeLength;
        
        float X = BarMin.x + (NormalizedPos * CanvasWidth);
        float Y_Center = BarMin.y + (BarHeight * 0.5f);

        float MarkerSize = 6.0f;
        // 마름모 좌표
        ImVec2 P1(X, BarMin.y + 2);
        ImVec2 P2(X + MarkerSize, Y_Center);
        ImVec2 P3(X, BarMax.y - 2);
        ImVec2 P4(X - MarkerSize, Y_Center);

        // ---------------------------------------------------------------------
        // 인터랙션 영역 (InvisibleButton)
        // ---------------------------------------------------------------------
        ImGui::SetCursorScreenPos(ImVec2(X - MarkerSize, BarMin.y));
        FString BtnID = "##MarkerBtn" + std::to_string(i);
        
        ImGui::InvisibleButton(BtnID.c_str(), ImVec2(MarkerSize * 2, BarHeight));
        
        bool bIsHovered = ImGui::IsItemHovered();
        bool bIsActive = ImGui::IsItemActive();

        // ---------------------------------------------------------------------
        // [드래그 로직]
        // ---------------------------------------------------------------------
        if (bIsActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f))
        {
            float DragDelta = ImGui::GetIO().MouseDelta.x;
            if (FMath::Abs(DragDelta) > 0.0f)
            {
                // 픽셀 -> 값 변환
                float ValueDelta = (DragDelta / CanvasWidth) * RangeLength;
                SamplePositions[i] += ValueDelta;
                
                // 현재 범위 내로 제한
                SamplePositions[i] = FMath::Clamp(SamplePositions[i], MinRange, MaxRange);
                bNeedRebuild = true;
            }
        }

        // ---------------------------------------------------------------------
        // [팝업 로직] (클릭 시)
        // ---------------------------------------------------------------------
        FString PopupID = "AnimSelectPopup_" + std::to_string(NodeID) + "_" + std::to_string(i);

        if (ImGui::IsItemDeactivated())
        {
            // 드래그 거리가 짧을 때만 클릭으로 인정
            if (ImGui::GetIO().MouseDragMaxDistanceSqr[0] < 25.0f) 
            {
                ed::Suspend(); // [좌표계 전환]
                ImGui::OpenPopup(PopupID.c_str());
                ed::Resume();
            }
        }
        
        // 우클릭 삭제
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            if (SamplePositions.Num() > 1)
            {
                SamplePositions.RemoveAt(i);
                SampleAnimations.RemoveAt(i);
                bNeedRebuild = true;
                ImGui::PopID();
                break; 
            }
        }

        // ---------------------------------------------------------------------
        // [팝업 렌더링]
        // ---------------------------------------------------------------------
        if (ImGui::IsPopupOpen(PopupID.c_str()))
        {
            ed::Suspend(); // [좌표계 전환]
            if (ImGui::BeginPopup(PopupID.c_str()))
            {
                ImGui::Text("Select Animation");
                ImGui::Separator();

                TArray<UAnimSequence*> AllAnims = RESOURCE.GetAll<UAnimSequence>();
                for (UAnimSequence* Anim : AllAnims)
                {
                    if (!Anim) continue;
                    bool bSelected = (SampleAnimations[i] == Anim);
                    if (ImGui::Selectable(Anim->GetFilePath().c_str(), bSelected))
                    {
                        SampleAnimations[i] = Anim;
                        bNeedRebuild = true;
                        ImGui::CloseCurrentPopup();
                    }
                    if (bSelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndPopup();
            }
            ed::Resume(); // [복구]
        }

        // ---------------------------------------------------------------------
        // 시각적 피드백 (마커 & 텍스트)
        // ---------------------------------------------------------------------
        ImU32 MarkerColor = IM_COL32(200, 200, 200, 255);
        if (bIsActive) MarkerColor = IM_COL32(255, 200, 50, 255);
        else if (bIsHovered) MarkerColor = IM_COL32(255, 255, 100, 255);
        
        if (SampleAnimations[i] == nullptr) MarkerColor = IM_COL32(255, 50, 50, 255);

        DrawList->AddQuadFilled(P1, P2, P3, P4, MarkerColor);
        DrawList->AddQuad(P1, P2, P3, P4, IM_COL32(0, 0, 0, 255));

        if (SampleAnimations[i])
        {
            std::string PathStr = SampleAnimations[i]->GetFilePath().c_str();
            size_t LastSlash = PathStr.find_last_of("/\\");
            std::string FileName = (LastSlash == std::string::npos) ? PathStr : PathStr.substr(LastSlash + 1);
            if (FileName.length() > 8) FileName = FileName.substr(0, 6) + "..";

            ImVec2 TextPos = ImVec2(X - (ImGui::CalcTextSize(FileName.c_str()).x * 0.5f), BarMax.y + 2);
            DrawList->AddText(TextPos, IM_COL32(255, 255, 255, 200), FileName.c_str());
        }

        // ---------------------------------------------------------------------
        // [툴팁 로직] (위치 수정됨)
        // ---------------------------------------------------------------------
        if (bIsHovered)
        {
            ed::Suspend(); 
            ImGui::BeginTooltip();
            ImGui::Text("Index: %d", i);
            ImGui::Text("Pos: %.1f", SamplePositions[i]);
            ImGui::Text("Anim: %s", SampleAnimations[i] ? SampleAnimations[i]->GetFilePath().c_str() : "None");
            ImGui::TextDisabled("(Drag to move, Click to edit)");
            ImGui::EndTooltip();
            ed::Resume();
        }

        ImGui::PopID();
    }
    
    ImGui::Dummy(ImVec2(0, 15.0f));

    if (bNeedRebuild)
    {
        RebuildBlendSpace();
    }
}

FBlueprintValue UK2Node_BlendSpace1D::EvaluatePin(const UEdGraphPin* OutputPin, FBlueprintContext* Context)
{
    if (OutputPin->PinName == "Output")
    {
        float Parameter = FBlueprintEvaluator::EvaluateInput<float>(FindPin("Parameter"), Context);

        if (BlendSpace)
        {
            BlendSpace->SetParameter(Parameter);
        }
        return BlendSpace;
    }
    return static_cast<UBlendSpace1D*>(nullptr);
}

void UK2Node_BlendSpace1D::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());

    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();

    ActionRegistrar.AddAction(Spawner);
}

void UK2Node_BlendSpace1D::RebuildBlendSpace()
{
    if (!BlendSpace)
    {
        BlendSpace = NewObject<UBlendSpace1D>();
    }

    BlendSpace->ClearSamples();

    BlendSpace->SetParameterRange(MinRange, MaxRange);

    for (int32 i = 0; i < SampleAnimations.Num(); ++i)
    {
        if (SampleAnimations[i])
        {
            BlendSpace->AddSample(SampleAnimations[i], SamplePositions[i]);
        }
    }
}

// ----------------------------------------------------------------
//	[BlendSpace2D] 2D 블렌드 스페이스 노드
// ----------------------------------------------------------------

IMPLEMENT_CLASS(UK2Node_BlendSpace2D)

UK2Node_BlendSpace2D::UK2Node_BlendSpace2D()
{
    TitleColor = ImColor(100, 180, 100);

    // 기본 5개 샘플 (중앙 + 4방향)
    SampleAnimations.SetNum(5);
    SamplePositions.SetNum(5);
    SamplePositions[0] = FVector2D(0.0f, 0.0f);      // Center
    SamplePositions[1] = FVector2D(0.0f, 100.0f);   // Forward
    SamplePositions[2] = FVector2D(0.0f, -100.0f);  // Backward
    SamplePositions[3] = FVector2D(-100.0f, 0.0f);  // Left
    SamplePositions[4] = FVector2D(100.0f, 0.0f);   // Right

    BlendSpace = NewObject<UBlendSpace2D>();
}

void UK2Node_BlendSpace2D::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    UK2Node::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        FJsonSerializer::ReadFloat(InOutHandle, "MinX", MinX);
        FJsonSerializer::ReadFloat(InOutHandle, "MaxX", MaxX);
        FJsonSerializer::ReadFloat(InOutHandle, "MinY", MinY);
        FJsonSerializer::ReadFloat(InOutHandle, "MaxY", MaxY);

        int32 NumSamples = 5;
        FJsonSerializer::ReadInt32(InOutHandle, "NumSamples", NumSamples);

        SampleAnimations.SetNum(NumSamples);
        SamplePositions.SetNum(NumSamples);

        for (int32 i = 0; i < NumSamples; ++i)
        {
            FString AnimKey = "SampleAnim_" + std::to_string(i);
            FString PosXKey = "SamplePosX_" + std::to_string(i);
            FString PosYKey = "SamplePosY_" + std::to_string(i);

            FString AnimPath;
            if (FJsonSerializer::ReadString(InOutHandle, AnimKey, AnimPath) && !AnimPath.empty())
            {
                SampleAnimations[i] = RESOURCE.Get<UAnimSequence>(AnimPath);
            }

            float PosX = 0.0f, PosY = 0.0f;
            FJsonSerializer::ReadFloat(InOutHandle, PosXKey, PosX);
            FJsonSerializer::ReadFloat(InOutHandle, PosYKey, PosY);
            SamplePositions[i] = FVector2D(PosX, PosY);
        }

        // 삼각형 로드
        int32 NumTriangles = 0;
        FJsonSerializer::ReadInt32(InOutHandle, "NumTriangles", NumTriangles);

        TriangleIndicesA.SetNum(NumTriangles);
        TriangleIndicesB.SetNum(NumTriangles);
        TriangleIndicesC.SetNum(NumTriangles);

        for (int32 i = 0; i < NumTriangles; ++i)
        {
            FString KeyA = "TriA_" + std::to_string(i);
            FString KeyB = "TriB_" + std::to_string(i);
            FString KeyC = "TriC_" + std::to_string(i);

            FJsonSerializer::ReadInt32(InOutHandle, KeyA, TriangleIndicesA[i]);
            FJsonSerializer::ReadInt32(InOutHandle, KeyB, TriangleIndicesB[i]);
            FJsonSerializer::ReadInt32(InOutHandle, KeyC, TriangleIndicesC[i]);
        }

        RebuildBlendSpace();
    }
    else
    {
        InOutHandle["MinX"] = MinX;
        InOutHandle["MaxX"] = MaxX;
        InOutHandle["MinY"] = MinY;
        InOutHandle["MaxY"] = MaxY;

        InOutHandle["NumSamples"] = SampleAnimations.Num();

        for (int32 i = 0; i < SampleAnimations.Num(); ++i)
        {
            FString AnimKey = "SampleAnim_" + std::to_string(i);
            FString PosXKey = "SamplePosX_" + std::to_string(i);
            FString PosYKey = "SamplePosY_" + std::to_string(i);

            if (SampleAnimations[i])
            {
                InOutHandle[AnimKey] = SampleAnimations[i]->GetFilePath();
            }
            InOutHandle[PosXKey] = SamplePositions[i].X;
            InOutHandle[PosYKey] = SamplePositions[i].Y;
        }

        // 삼각형 저장
        InOutHandle["NumTriangles"] = TriangleIndicesA.Num();

        for (int32 i = 0; i < TriangleIndicesA.Num(); ++i)
        {
            FString KeyA = "TriA_" + std::to_string(i);
            FString KeyB = "TriB_" + std::to_string(i);
            FString KeyC = "TriC_" + std::to_string(i);

            InOutHandle[KeyA] = TriangleIndicesA[i];
            InOutHandle[KeyB] = TriangleIndicesB[i];
            InOutHandle[KeyC] = TriangleIndicesC[i];
        }
    }
}

void UK2Node_BlendSpace2D::AllocateDefaultPins()
{
    // 입력: X (Direction), Y (Speed)
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Float, "X", "0.0");
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Float, "Y", "0.0");

    // 출력: 블렌드된 포즈
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::AnimSequence, "Output");
}

void UK2Node_BlendSpace2D::RenderBody()
{
    if (!BlendSpace)
    {
        BlendSpace = NewObject<UBlendSpace2D>();
    }

    if (SamplePositions.Num() == 0 || SamplePositions.Num() != SampleAnimations.Num())
    {
        SamplePositions = { FVector2D(0.0f, 0.0f) };
        SampleAnimations.SetNum(1);
        SampleAnimations[0] = nullptr;
    }

    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    // -------------------------------------------------------------------------
    // 1. Range 설정 UI
    // -------------------------------------------------------------------------
    bool bRangeChanged = false;

    ImGui::PushItemWidth(50.0f);

    ImGui::Text("X:"); ImGui::SameLine();
    if (ImGui::DragFloat("##MinX", &MinX, 1.0f)) bRangeChanged = true;
    ImGui::SameLine(); ImGui::Text("~"); ImGui::SameLine();
    if (ImGui::DragFloat("##MaxX", &MaxX, 1.0f)) bRangeChanged = true;

    ImGui::Text("Y:"); ImGui::SameLine();
    if (ImGui::DragFloat("##MinY", &MinY, 1.0f)) bRangeChanged = true;
    ImGui::SameLine(); ImGui::Text("~"); ImGui::SameLine();
    if (ImGui::DragFloat("##MaxY", &MaxY, 1.0f)) bRangeChanged = true;

    ImGui::PopItemWidth();

    if (bRangeChanged)
    {
        if (MaxX < MinX) MaxX = MinX + 10.0f;
        if (MaxY < MinY) MaxY = MinY + 10.0f;
        BlendSpace->SetParameterRange(FVector2D(MinX, MinY), FVector2D(MaxX, MaxY));
    }

    // -------------------------------------------------------------------------
    // 2. Add Sample / Clear Selection 버튼
    // -------------------------------------------------------------------------
    float CanvasSize = 200.0f;

    if (ImGui::Button("+ Add Sample", ImVec2(95, 0)))
    {
        SamplePositions.Add(FVector2D(0.0f, 0.0f));
        SampleAnimations.Add(nullptr);
        RebuildBlendSpace();
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear Sel", ImVec2(95, 0)))
    {
        SelectedIndices.Empty();
    }

    // 선택 상태 표시
    if (SelectedIndices.Num() > 0)
    {
        ImGui::Text("Selected: %d/3", SelectedIndices.Num());
        if (SelectedIndices.Num() == 3)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "(Click to connect!)");
        }
    }
    else
    {
        ImGui::TextDisabled("Shift+Click to select points");
    }

    // 삼각형 삭제 버튼
    if (TriangleIndicesA.Num() > 0)
    {
        if (ImGui::Button("Clear Triangles", ImVec2(CanvasSize, 0)))
        {
            TriangleIndicesA.Empty();
            TriangleIndicesB.Empty();
            TriangleIndicesC.Empty();
            RebuildBlendSpace();
        }
    }

    ImGui::Dummy(ImVec2(0, 4.0f));

    // -------------------------------------------------------------------------
    // 3. 2D 캔버스 그리기
    // -------------------------------------------------------------------------
    ImVec2 CursorPos = ImGui::GetCursorScreenPos();
    ImVec2 CanvasMin = CursorPos;
    ImVec2 CanvasMax = ImVec2(CanvasMin.x + CanvasSize, CanvasMin.y + CanvasSize);

    // 캔버스 배경
    DrawList->AddRectFilled(CanvasMin, CanvasMax, IM_COL32(30, 30, 30, 255), 4.0f);
    DrawList->AddRect(CanvasMin, CanvasMax, IM_COL32(100, 100, 100, 255), 4.0f);

    // 중심선
    float CenterX = CanvasMin.x + CanvasSize * 0.5f;
    float CenterY = CanvasMin.y + CanvasSize * 0.5f;
    DrawList->AddLine(ImVec2(CenterX, CanvasMin.y), ImVec2(CenterX, CanvasMax.y), IM_COL32(60, 60, 60, 255));
    DrawList->AddLine(ImVec2(CanvasMin.x, CenterY), ImVec2(CanvasMax.x, CenterY), IM_COL32(60, 60, 60, 255));

    // 삼각형 그리기
    DrawTriangles(DrawList, CanvasMin, CanvasSize);

    ImGui::Dummy(ImVec2(CanvasSize, CanvasSize));

    bool bNeedRebuild = false;
    float RangeX = MaxX - MinX;
    float RangeY = MaxY - MinY;
    if (RangeX <= 0.0f) RangeX = 1.0f;
    if (RangeY <= 0.0f) RangeY = 1.0f;

    // -------------------------------------------------------------------------
    // 4. 샘플 포인트 루프
    // -------------------------------------------------------------------------
    for (int32 i = 0; i < SamplePositions.Num(); ++i)
    {
        ImGui::PushID(i);

        // 위치 정규화
        float ClampedX = FMath::Clamp(SamplePositions[i].X, MinX, MaxX);
        float ClampedY = FMath::Clamp(SamplePositions[i].Y, MinY, MaxY);
        float NormX = (ClampedX - MinX) / RangeX;
        float NormY = (ClampedY - MinY) / RangeY;

        // 화면 좌표 (Y축 반전: 위가 +Y)
        float ScreenX = CanvasMin.x + NormX * CanvasSize;
        float ScreenY = CanvasMax.y - NormY * CanvasSize;

        float MarkerSize = 8.0f;

        // 선택 여부 확인
        bool bIsSelected = SelectedIndices.Contains(i);

        // 인터랙션 영역
        ImGui::SetCursorScreenPos(ImVec2(ScreenX - MarkerSize, ScreenY - MarkerSize));
        FString BtnID = "##Marker2D_" + std::to_string(i);

        ImGui::InvisibleButton(BtnID.c_str(), ImVec2(MarkerSize * 2, MarkerSize * 2));

        bool bIsHovered = ImGui::IsItemHovered();
        bool bIsActive = ImGui::IsItemActive();

        // Shift+클릭: 선택 토글
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::GetIO().KeyShift)
        {
            if (bIsSelected)
            {
                SelectedIndices.Remove(i);
            }
            else
            {
                if (SelectedIndices.Num() < 3)
                {
                    SelectedIndices.Add(i);
                }

                // 3개 선택되면 삼각형 생성
                if (SelectedIndices.Num() == 3)
                {
                    TriangleIndicesA.Add(SelectedIndices[0]);
                    TriangleIndicesB.Add(SelectedIndices[1]);
                    TriangleIndicesC.Add(SelectedIndices[2]);
                    SelectedIndices.Empty();
                    bNeedRebuild = true;
                }
            }
        }
        // 일반 드래그 (Shift 없이)
        else if (bIsActive && !ImGui::GetIO().KeyShift && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f))
        {
            ImVec2 Delta = ImGui::GetIO().MouseDelta;
            if (FMath::Abs(Delta.x) > 0.0f || FMath::Abs(Delta.y) > 0.0f)
            {
                float ValueDeltaX = (Delta.x / CanvasSize) * RangeX;
                float ValueDeltaY = -(Delta.y / CanvasSize) * RangeY;

                SamplePositions[i].X += ValueDeltaX;
                SamplePositions[i].Y += ValueDeltaY;

                SamplePositions[i].X = FMath::Clamp(SamplePositions[i].X, MinX, MaxX);
                SamplePositions[i].Y = FMath::Clamp(SamplePositions[i].Y, MinY, MaxY);
                bNeedRebuild = true;
            }
        }

        // 일반 클릭 (Shift 없이, 드래그 아닐 때): 애니메이션 선택 팝업
        FString PopupID = "AnimSelectPopup2D_" + std::to_string(NodeID) + "_" + std::to_string(i);

        if (ImGui::IsItemDeactivated() && !ImGui::GetIO().KeyShift)
        {
            if (ImGui::GetIO().MouseDragMaxDistanceSqr[0] < 25.0f)
            {
                ed::Suspend();
                ImGui::OpenPopup(PopupID.c_str());
                ed::Resume();
            }
        }

        // 우클릭 삭제
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            if (SamplePositions.Num() > 1)
            {
                // 선택 목록에서도 제거
                SelectedIndices.Remove(i);
                // 삭제된 인덱스보다 큰 선택 인덱스 조정
                for (int32& SelIdx : SelectedIndices)
                {
                    if (SelIdx > i) SelIdx--;
                }
                // 삼각형에서 이 점을 사용하는 것들 제거 + 인덱스 조정
                for (int32 t = TriangleIndicesA.Num() - 1; t >= 0; --t)
                {
                    if (TriangleIndicesA[t] == i || TriangleIndicesB[t] == i || TriangleIndicesC[t] == i)
                    {
                        TriangleIndicesA.RemoveAt(t);
                        TriangleIndicesB.RemoveAt(t);
                        TriangleIndicesC.RemoveAt(t);
                    }
                    else
                    {
                        if (TriangleIndicesA[t] > i) TriangleIndicesA[t]--;
                        if (TriangleIndicesB[t] > i) TriangleIndicesB[t]--;
                        if (TriangleIndicesC[t] > i) TriangleIndicesC[t]--;
                    }
                }

                SamplePositions.RemoveAt(i);
                SampleAnimations.RemoveAt(i);
                bNeedRebuild = true;
                ImGui::PopID();
                break;
            }
        }

        // 팝업 렌더링
        if (ImGui::IsPopupOpen(PopupID.c_str()))
        {
            ed::Suspend();
            if (ImGui::BeginPopup(PopupID.c_str()))
            {
                ImGui::Text("Select Animation");
                ImGui::Separator();

                TArray<UAnimSequence*> AllAnims = RESOURCE.GetAll<UAnimSequence>();
                for (UAnimSequence* Anim : AllAnims)
                {
                    if (!Anim) continue;
                    bool bSelected = (SampleAnimations[i] == Anim);
                    if (ImGui::Selectable(Anim->GetFilePath().c_str(), bSelected))
                    {
                        SampleAnimations[i] = Anim;
                        bNeedRebuild = true;
                        ImGui::CloseCurrentPopup();
                    }
                    if (bSelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndPopup();
            }
            ed::Resume();
        }

        // 마커 렌더링
        ImU32 MarkerColor = IM_COL32(200, 200, 200, 255);
        ImU32 OutlineColor = IM_COL32(0, 0, 0, 255);

        if (bIsSelected)
        {
            MarkerColor = IM_COL32(50, 200, 255, 255);  // 선택됨: 하늘색
            OutlineColor = IM_COL32(255, 255, 0, 255);  // 노란 테두리
        }
        else if (bIsActive)
        {
            MarkerColor = IM_COL32(255, 200, 50, 255);
        }
        else if (bIsHovered)
        {
            MarkerColor = IM_COL32(255, 255, 100, 255);
        }

        if (SampleAnimations[i] == nullptr && !bIsSelected)
        {
            MarkerColor = IM_COL32(255, 50, 50, 255);  // 애니메이션 없음: 빨간색
        }

        DrawList->AddCircleFilled(ImVec2(ScreenX, ScreenY), MarkerSize, MarkerColor);
        DrawList->AddCircle(ImVec2(ScreenX, ScreenY), MarkerSize, OutlineColor, 0, bIsSelected ? 3.0f : 1.0f);

        // 인덱스 번호 표시
        char IndexStr[8];
        snprintf(IndexStr, sizeof(IndexStr), "%d", i);
        ImVec2 TextSize = ImGui::CalcTextSize(IndexStr);
        DrawList->AddText(ImVec2(ScreenX - TextSize.x * 0.5f, ScreenY - TextSize.y * 0.5f), IM_COL32(0, 0, 0, 255), IndexStr);

        // 툴팁
        if (bIsHovered)
        {
            ed::Suspend();
            ImGui::BeginTooltip();
            ImGui::Text("Index: %d", i);
            ImGui::Text("Pos: (%.1f, %.1f)", SamplePositions[i].X, SamplePositions[i].Y);
            ImGui::Text("Anim: %s", SampleAnimations[i] ? SampleAnimations[i]->GetFilePath().c_str() : "None");
            ImGui::Separator();
            ImGui::TextDisabled("Shift+Click: Select for triangle");
            ImGui::TextDisabled("Drag: Move position");
            ImGui::TextDisabled("Click: Set animation");
            ImGui::TextDisabled("Right-click: Delete");
            ImGui::EndTooltip();
            ed::Resume();
        }

        ImGui::PopID();
    }

    ImGui::Dummy(ImVec2(0, 15.0f));

    if (bNeedRebuild)
    {
        RebuildBlendSpace();
    }
}

void UK2Node_BlendSpace2D::DrawTriangles(ImDrawList* DrawList, ImVec2 CanvasMin, float CanvasSize)
{
    // UI의 SamplePositions와 수동 삼각형 인덱스를 사용하여 그리기
    float RangeX = MaxX - MinX;
    float RangeY = MaxY - MinY;
    if (RangeX <= 0.0f) RangeX = 1.0f;
    if (RangeY <= 0.0f) RangeY = 1.0f;

    // 수동 삼각형 그리기
    for (int32 i = 0; i < TriangleIndicesA.Num(); ++i)
    {
        int32 IdxA = TriangleIndicesA[i];
        int32 IdxB = TriangleIndicesB[i];
        int32 IdxC = TriangleIndicesC[i];

        // 유효성 검사
        if (IdxA < 0 || IdxA >= SamplePositions.Num() ||
            IdxB < 0 || IdxB >= SamplePositions.Num() ||
            IdxC < 0 || IdxC >= SamplePositions.Num())
            continue;

        ImVec2 Points[3];
        int32 Indices[3] = { IdxA, IdxB, IdxC };

        for (int32 j = 0; j < 3; ++j)
        {
            const FVector2D& Pos = SamplePositions[Indices[j]];
            float NormX = (Pos.X - MinX) / RangeX;
            float NormY = (Pos.Y - MinY) / RangeY;
            Points[j].x = CanvasMin.x + NormX * CanvasSize;
            Points[j].y = CanvasMin.y + CanvasSize - NormY * CanvasSize;
        }

        // 삼각형 채우기 (반투명)
        DrawList->AddTriangleFilled(Points[0], Points[1], Points[2], IM_COL32(80, 150, 80, 60));
        // 삼각형 외곽선
        DrawList->AddTriangle(Points[0], Points[1], Points[2], IM_COL32(80, 180, 80, 200), 1.5f);
    }
}

FBlueprintValue UK2Node_BlendSpace2D::EvaluatePin(const UEdGraphPin* OutputPin, FBlueprintContext* Context)
{
    if (OutputPin->PinName == "Output")
    {
        float X = FBlueprintEvaluator::EvaluateInput<float>(FindPin("X"), Context);
        float Y = FBlueprintEvaluator::EvaluateInput<float>(FindPin("Y"), Context);

        if (BlendSpace)
        {
            BlendSpace->SetParameter(X, Y);
        }
        return BlendSpace;
    }
    return static_cast<UBlendSpace2D*>(nullptr);
}

void UK2Node_BlendSpace2D::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());

    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();

    ActionRegistrar.AddAction(Spawner);
}

void UK2Node_BlendSpace2D::RebuildBlendSpace()
{
    if (!BlendSpace)
    {
        BlendSpace = NewObject<UBlendSpace2D>();
    }

    BlendSpace->ClearSamples();
    BlendSpace->ClearTriangles();
    BlendSpace->SetParameterRange(FVector2D(MinX, MinY), FVector2D(MaxX, MaxY));

    // UI 인덱스 -> BlendSpace 인덱스 매핑 (애니메이션 없는 샘플은 건너뛰므로)
    TArray<int32> UIToBlendSpaceIndex;
    UIToBlendSpaceIndex.SetNum(SampleAnimations.Num());

    int32 BlendSpaceIdx = 0;
    for (int32 i = 0; i < SampleAnimations.Num(); ++i)
    {
        if (SampleAnimations[i])
        {
            BlendSpace->AddSample(SampleAnimations[i], SamplePositions[i].X, SamplePositions[i].Y);
            UIToBlendSpaceIndex[i] = BlendSpaceIdx++;
        }
        else
        {
            UIToBlendSpaceIndex[i] = -1;  // 애니메이션 없음
        }
    }

    // 수동 삼각형 추가 (UI 인덱스를 BlendSpace 인덱스로 변환)
    for (int32 i = 0; i < TriangleIndicesA.Num(); ++i)
    {
        int32 UIIdxA = TriangleIndicesA[i];
        int32 UIIdxB = TriangleIndicesB[i];
        int32 UIIdxC = TriangleIndicesC[i];

        // UI 인덱스 유효성 검사
        if (UIIdxA < 0 || UIIdxA >= UIToBlendSpaceIndex.Num() ||
            UIIdxB < 0 || UIIdxB >= UIToBlendSpaceIndex.Num() ||
            UIIdxC < 0 || UIIdxC >= UIToBlendSpaceIndex.Num())
            continue;

        int32 BSIdxA = UIToBlendSpaceIndex[UIIdxA];
        int32 BSIdxB = UIToBlendSpaceIndex[UIIdxB];
        int32 BSIdxC = UIToBlendSpaceIndex[UIIdxC];

        // 모든 점에 애니메이션이 할당되어 있어야 유효한 삼각형
        if (BSIdxA >= 0 && BSIdxB >= 0 && BSIdxC >= 0)
        {
            BlendSpace->AddTriangle(BSIdxA, BSIdxB, BSIdxC);
        }
    }
}

// ----------------------------------------------------------------
//	[IsAnimationFinished] 애니메이션 종료 여부 확인 노드
// ----------------------------------------------------------------

IMPLEMENT_CLASS(UK2Node_IsAnimationFinished)

UK2Node_IsAnimationFinished::UK2Node_IsAnimationFinished()
{
    TitleColor = ImColor(100, 120, 255);
}

void UK2Node_IsAnimationFinished::AllocateDefaultPins()
{
    // 출력: bool (애니메이션이 끝났으면 true)
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::Bool, "Finished");
}

void UK2Node_IsAnimationFinished::RenderBody()
{
    // 노드 바디에 표시할 추가 UI 없음
}

FBlueprintValue UK2Node_IsAnimationFinished::EvaluatePin(const UEdGraphPin* OutputPin, FBlueprintContext* Context)
{
    if (OutputPin->PinName == "Finished")
    {
        if (!Context || !Context->SourceObject)
        {
            return false;
        }

        // Context에서 AnimInstance 가져오기
        UAnimInstance* AnimInstance = Cast<UAnimInstance>(Context->SourceObject);
        if (!AnimInstance)
        {
            return false;
        }

        // IsPlaying()이 false면 애니메이션이 끝난 것
        // (루핑이 아닌 애니메이션이 끝까지 재생되면 bIsPlaying = false가 됨)
        bool bIsFinished = !AnimInstance->IsPlaying();
        return bIsFinished;
    }
    return false;
}

void UK2Node_IsAnimationFinished::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());

    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();

    ActionRegistrar.AddAction(Spawner);
}