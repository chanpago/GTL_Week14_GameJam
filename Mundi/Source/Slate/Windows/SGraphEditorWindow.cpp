#include "pch.h"
#include "SGraphEditorWindow.h"

#include "BlueprintGraph/BlueprintActionDatabase.h"
#include "BlueprintGraph/EdGraph.h"
#include "BlueprintGraph/EdGraphNode.h"
#include "BlueprintGraph/EdGraphPin.h"

// ImGui-Node-Editor ID 변환 헬퍼
static inline ed::NodeId ToNodeId(int32 id) { return (uintptr_t)id; }
static inline ed::PinId ToPinId(int32 id) { return (uintptr_t)id; }
static inline ed::LinkId ToLinkId(int32 id) { return (uintptr_t)id; }

SGraphEditorWindow::SGraphEditorWindow()
{
}

SGraphEditorWindow::~SGraphEditorWindow()
{
    if (Builder)
    {
        delete Builder;
        Builder = nullptr;
    }

    if (Context)
    {
        ed::DestroyEditor(Context);
        Context = nullptr;
    }

    // @todo 그래프를 소유할 경우 여기서 소멸해야 함 
}

void SGraphEditorWindow::OnRender()
{
    if (!bIsOpen)
    {
        return;
    }
    
    if (ImGui::Begin("블루프린트 편집기", &bIsOpen, ImGuiWindowFlags_MenuBar))
    {
        RenderMenuBar();
        RenderEditor();
    }
    ImGui::End();
}

void SGraphEditorWindow::OnUpdate(float DeltaSeconds)
{
}

bool SGraphEditorWindow::Initialize(UEdGraph* InGraphToEdit)
{
    /** @note Initialize는 한 번만 호출되어야 함 */
    assert(InGraphToEdit);

    Graph = InGraphToEdit;

    ed::Config Config;
    Context = ed::CreateEditor(&Config);

    HeaderBackground = RESOURCE.Load<UTexture>("Data/Icon/BlueprintBackground.png");
    int32 TexWidth = HeaderBackground ? HeaderBackground->GetWidth() : 0;
    int32 TexHeight = HeaderBackground ? HeaderBackground->GetHeight() : 0;
    Builder = new util::BlueprintNodeBuilder(reinterpret_cast<ImTextureID>(HeaderBackground->GetShaderResourceView()), TexWidth, TexHeight);

    bIsOpen = true;

    return true;
}

void SGraphEditorWindow::RenderMenuBar()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("파일"))
        {
            if (ImGui::Button("저장"))
            {
                /** @todo 직렬화 */
            }
            if (ImGui::Button("불러오기"))
            {
                /** @todo 역직렬화 */
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void SGraphEditorWindow::RenderEditor()
{
    if (!Graph)
    {
        return;
    }

    ed::SetCurrentEditor(Context);
    ed::Begin("GraphEditorInternal");

    // #1. 모든 노드 렌더링
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        RenderNode(Node); 
    }

    // #2. 모든 링크 렌더링
    RenderLinks();

    // #3. 상호작용: 링크 생성
    HandleCreation();

    // #4. 상호작용: 링크/노드 삭제 
    HandleDeletion();

    // #5. 상호작용: 컨텍스트 메뉴
    ed::Suspend();
    RenderContextMenu();
    ed::Resume();

    ed::End();
    ed::SetCurrentEditor(nullptr);
}

void SGraphEditorWindow::RenderNode(UEdGraphNode* Node)
{
    if (!Node->bIsPositionSet)
    {
        ed::SetNodePosition(ToNodeId(Node->NodeID), Node->Position);
        Node->bIsPositionSet = true;
    }

    Builder->Begin(ToNodeId(Node->NodeID));

    // #1. 헤더
    Builder->Header(Node->TitleColor);
    {
        ImGui::TextUnformatted(Node->GetNodeTitle().c_str());
        ImGui::SameLine();
        ImGui::Dummy(ImVec2(0, 28)); // 헤더 높이 보정
    }
    Builder->EndHeader();

    // #2. 입력 핀
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->Direction == EEdGraphPinDirection::EGPD_Input)
        {
            RenderPin(Pin);
        }
    }

    // #3. 중간 영역 (커스텀 UI 콜백)
    Builder->Middle();
    Node->RenderBody();

    // #4. 출력 핀
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->Direction == EEdGraphPinDirection::EGPD_Output)
        {
            RenderPin(Pin);
        }
    }

    Builder->End();

    Node->Position = ed::GetNodePosition(ToNodeId(Node->NodeID));
}

void SGraphEditorWindow::RenderPin(UEdGraphPin* Pin)
{
    if (Pin->Direction == EEdGraphPinDirection::EGPD_Input)
    {
        Builder->Input(ToPinId(Pin->PinID));
        DrawPinIcon(Pin, Pin->LinkedTo.Num() > 0, 255);
        ImGui::SameLine();
        if (!Pin->PinName.empty())
        {
            ImGui::TextUnformatted(Pin->PinName.c_str());
        }
        Builder->EndInput();
    }
    else if (Pin->Direction == EEdGraphPinDirection::EGPD_Output)
    {
        Builder->Output(ToPinId(Pin->PinID));
        if (!Pin->PinName.empty())
        {
            ImGui::TextUnformatted(Pin->PinName.c_str());
            ImGui::SameLine(); 
        }
        DrawPinIcon(Pin, Pin->LinkedTo.Num() > 0, 255);
        Builder->EndOutput();
    }
    else // EGPD_MAX
    {
        UE_LOG("SGraphEditorWindow::RenderPin: 잘못된 핀 방향입니다.");
    }
}

void SGraphEditorWindow::RenderLinks()
{
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        for (UEdGraphPin* Pin : Node->Pins)
        {
            if (Pin->Direction == EEdGraphPinDirection::EGPD_Output)
            {
                for (UEdGraphPin* OtherPin : Pin->LinkedTo)
                {
                    ed::Link(ToLinkId(GetLinkId(Pin, OtherPin)),
                             ToPinId(Pin->PinID),
                             ToPinId(OtherPin->PinID));
                }
            }
        }
    }
}

void SGraphEditorWindow::HandleCreation()
{
    if (ed::BeginCreate())
    {
        ed::PinId StartPinID_UI;
        ed::PinId EndPinID_UI;
        if (ed::QueryNewLink(&StartPinID_UI, &EndPinID_UI))
        {
            UEdGraphPin* StartPin = Graph->FindPin((int32)(uintptr_t)StartPinID_UI);
            UEdGraphPin* EndPin = Graph->FindPin((int32)(uintptr_t)EndPinID_UI);

            // --- 링크 유효성 검사 --- 
            bool bAllowLink = true;
            if (!StartPin || !EndPin)
            {
                bAllowLink = false;
            }
            else if (StartPin->OwningNode == EndPin->OwningNode)
            {
                bAllowLink = false;
            }
            else if (StartPin->Direction == EndPin->Direction)
            {
                bAllowLink = false;
            }
            else if (StartPin->PinType.PinCategory != EndPin->PinType.PinCategory)
            {
                bAllowLink = false;
            }
            // --- 링크 유효성 검사 종료 ---

            if (bAllowLink && ed::AcceptNewItem())
            {
                if (StartPin->Direction == EEdGraphPinDirection::EGPD_Output)
                {
                    StartPin->MakeLinkTo(EndPin);
                }
                else if (StartPin->Direction == EEdGraphPinDirection::EGPD_Input)
                {
                    EndPin->MakeLinkTo(StartPin);
                }
            }
        }
    }
    ed::EndCreate();
}

void SGraphEditorWindow::HandleDeletion()
{
    if (ed::BeginDelete())
    {
        // # 1. 링크 삭제 처리
        ed::LinkId DestroyedLinkID_UI;
        while (ed::QueryDeletedLink(&DestroyedLinkID_UI))
        {
            if (ed::AcceptDeletedItem())
            {
                // Link ID에서 Pin ID들을 역산
                LinkId LinkID = static_cast<LinkId>(DestroyedLinkID_UI);
                int32 StartPinID = static_cast<int32>(LinkID >> 32);
                int32 EndPinID = static_cast<int32>(LinkID & 0xFFFFFFFF);

                UEdGraphPin* StartPin = Graph->FindPin(StartPinID);
                UEdGraphPin* EndPin = Graph->FindPin(EndPinID);

                if (StartPin && EndPin)
                {
                    StartPin->BreakLinkTo(EndPin); // 양방향 연결 해제
                }
            }
        }

        // #2. 노드 삭제 처리
        ed::NodeId DestroyedNodeID_UI;
        while (ed::QueryDeletedNode(&DestroyedNodeID_UI))
        {
            if (ed::AcceptDeletedItem())
            {
                int32 NodeID = (int32)(uintptr_t)DestroyedNodeID_UI;
                Graph->RemoveNode(NodeID);
            }
        }
    }
    ed::EndDelete();
}

void SGraphEditorWindow::RenderContextMenu()
{
    if (ed::ShowBackgroundContextMenu() && !ImGui::IsPopupOpen("NodeContextMenu"))
    {
        ImGui::OpenPopup("NodeContextMenu");
    }

    if (ImGui::BeginPopup("NodeContextMenu"))
    {
        ImVec2 ClickPos = ed::ScreenToCanvas(ImGui::GetMousePosOnOpeningCurrentPopup());
        FBlueprintActionDatabase& Database = FBlueprintActionDatabase::GetInstance();

        // @todo 현재는 매 프레임마다 카테고리 맵을 생성함. 액션 수가 늘어날 수록 성능에 부담이 될 수 있음.
        TMap<FString, TArray<UBlueprintNodeSpawner*>> CategorizedActions;
        for (UBlueprintNodeSpawner* Action : Database.ActionRegistry)
        {
            CategorizedActions[Action->Category].Add(Action); 
        }

        for (auto const& [Category, Actions] : CategorizedActions)
        {
            if (ImGui::BeginMenu(Category.c_str()))
            {
                for (UBlueprintNodeSpawner* Action : Actions)
                {
                    if (ImGui::MenuItem(Action->MenuName.c_str()))
                    {
                        UEdGraphNode* NewNode = Action->SpawnNode(Graph, ClickPos);
                        if (NewNode)
                        {
                            NewNode->NodeID = Graph->GetNextUniqueID();
                            Graph->AddNode(NewNode);
                        }
                    }
                }
                ImGui::EndMenu();
            }
        }
        ImGui::EndPopup();
    }
}

ImColor SGraphEditorWindow::GetPinColor(const FEdGraphPinType& PinType)
{
    // EPinType 대신 FName 카테고리로 비교
    if (PinType.PinCategory == FEdGraphPinCategory::Exec) return ImColor(255, 255, 255);
    if (PinType.PinCategory == FEdGraphPinCategory::Float) return ImColor(147, 226, 74);
    if (PinType.PinCategory == FEdGraphPinCategory::Int) return ImColor(68, 201, 156);
    if (PinType.PinCategory == FEdGraphPinCategory::AnimSequence) return ImColor(100, 120, 255);
    // (bool, object 등 추가)
    
    return ImColor(200, 200, 200); // Default
}

void SGraphEditorWindow::DrawPinIcon(const UEdGraphPin* Pin, bool bIsConnected, int32 Alpha)
{
    ax::Drawing::IconType IconType;
    ImColor Color = GetPinColor(Pin->PinType);
    Color.Value.w = Alpha / 255.0f;

    if (Pin->PinType.PinCategory == FEdGraphPinCategory::Exec)
    {
        IconType = ax::Drawing::IconType::Flow;
    }
    else
    {
        // @todo PinType.IsArray() 등에 따라 아이콘 변경)
        IconType = ax::Drawing::IconType::Circle;
    }
    
    ax::Widgets::Icon(ImVec2(24, 24), IconType, bIsConnected, Color, ImColor(32, 32, 32, Alpha));
}

SGraphEditorWindow::LinkId SGraphEditorWindow::GetLinkId(UEdGraphPin* StartPin, UEdGraphPin* EndPin) const
{
    LinkId High = static_cast<LinkId>(StartPin->PinID);
    LinkId Low = static_cast<LinkId>(EndPin->PinID);

    return (High << 32) | Low;
}
