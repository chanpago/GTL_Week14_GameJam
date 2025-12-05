#include "pch.h"
#include "SGraphEditorWindow.h"

#include "PlatformProcess.h"
#include "USlateManager.h"
#include "BlueprintGraph/BlueprintActionDatabase.h"
#include "BlueprintGraph/EdGraph.h"
#include "BlueprintGraph/EdGraphNode.h"
#include "BlueprintGraph/EdGraphPin.h"
#include "BlueprintGraph/K2Node_Animation.h"
#include "AnimGraph/BlendSpacePreviewWindow.h"
#include "Source/Editor/FBX/BlendSpace/BlendSpace2D.h"

// ImGui-Node-Editor ID 변환 헬퍼
static inline ed::NodeId ToNodeId(int32 id) { return (uintptr_t)id; }
static inline ed::PinId ToPinId(int32 id) { return (uintptr_t)id; }
static inline ed::LinkId ToLinkId(int64 id) { return (uintptr_t)id; }

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

    if (BlendSpacePreview)
    {
        delete BlendSpacePreview;
        BlendSpacePreview = nullptr;
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

    // BlendSpace 프리뷰 창 렌더링
    RenderBlendSpacePreview();

    if (!bIsOpen)
    {
        USlateManager::GetInstance().CloseAnimationGraphEditor();
    }
}

void SGraphEditorWindow::OnUpdate(float DeltaSeconds)
{
    if (BlendSpacePreview && BlendSpacePreview->IsOpen())
    {
        BlendSpacePreview->OnUpdate(DeltaSeconds);
    }
}

bool SGraphEditorWindow::Initialize(UEdGraph* InGraphToEdit, UWorld* InWorld, ID3D11Device* InDevice)
{
    /** @note Initialize는 한 번만 호출되어야 함 */
    assert(InGraphToEdit);

    Graph = InGraphToEdit;
    World = InWorld;
    Device = InDevice;

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
            // =====================================================
            // [저장] Save
            // =====================================================
            if (ImGui::MenuItem("저장", "Ctrl+S")) 
            {
                if (Graph)
                {
                    ed::SetCurrentEditor(Context);
                    
                    // @note 에디터 상에서 움직인 위치를 실제 노드 위치에 반영해야 함
                    for (UEdGraphNode* Node : Graph->Nodes)
                    {
                        ImVec2 Pos = ed::GetNodePosition(Node->NodeID);
                
                        Node->NodePosX = Pos.x;
                        Node->NodePosY = Pos.y;
                
                        Node->Position = Pos;
                    }

                    std::filesystem::path SavePath = FPlatformProcess::OpenSaveFileDialog(
                        L"Data/Graphs", 
                        L"graph", 
                        L"Graph Files", 
                        L"NewGraph.graph"
                    );

                    if (!SavePath.empty())
                    {
                        JSON GraphJson = JSON::Make(JSON::Class::Object);
                        
                        Graph->Serialize(false, GraphJson);

                        if (FJsonSerializer::SaveJsonToFile(GraphJson, SavePath.wstring()))
                        {
                            UE_LOG("SGraphEditorWindow::RenderMenuBar: 그래프가 '%ls'에 성공적으로 저장되었습니다.", SavePath.c_str());
                        }
                        else
                        {
                            UE_LOG("SGraphEditorWindow::RenderMenuBar: 그래프를 '%ls'에 저장하는데 실패했습니다.", SavePath.c_str());
                        }
                    }
                }
            }

            // =====================================================
            // [불러오기] Load
            // =====================================================
            if (ImGui::MenuItem("불러오기", "Ctrl+O"))
            {
                std::filesystem::path LoadPath = FPlatformProcess::OpenLoadFileDialog(
                    L"Data/Graphs", 
                    L"graph", 
                    L"Graph Files"
                );

                if (!LoadPath.empty())
                {
                    JSON LoadedJson;

                    if (FJsonSerializer::LoadJsonFromFile(LoadedJson, LoadPath.wstring()))
                    {
                        if (Graph == nullptr)
                        {
                            Graph = NewObject<UEdGraph>();
                        }

                        Graph->Serialize(true, LoadedJson);
                        
                        UE_LOG("SGraphEditorWindow::RenderMenuBar: 그래프가 '%ls'로부터 성공적으로 로드되었습니다.", LoadPath.c_str());
                    }
                    else
                    {
                        UE_LOG("SGraphEditorWindow::RenderMenuBar: 그래프를 '%ls'로부터 로드하는데 실패했습니다.", LoadPath.c_str());
                    }
                }
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

    // #5. 상호작용: 노드 더블클릭 (BlendSpace 프리뷰 등)
    HandleDoubleClick();

    // #6. 상호작용: 컨텍스트 메뉴
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
    ImGui::PushID(Node->NodeID);
    Node->RenderBody();
    ImGui::PopID();

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
                UEdGraphPin* InputPin = (StartPin->Direction == EEdGraphPinDirection::EGPD_Input) ? StartPin : EndPin;
                UEdGraphPin* OutputPin = (StartPin->Direction == EEdGraphPinDirection::EGPD_Output) ? StartPin : EndPin;

                // @note 실행 흐름(Exec)을 표현하는 핀이 아니라면 기존 연결을 끊는다. (데이터 핀은 하나의 값만 넣을 수 있다)
                if (InputPin->PinType.PinCategory != FEdGraphPinCategory::Exec)
                {
                    InputPin->BreakAllLinks();        
                }

                OutputPin->MakeLinkTo(InputPin);
            }
        }

        ed::PinId DraggingPinID_UI = 0;
        if (ed::QueryNewNode(&DraggingPinID_UI))
        {
            if (ed::AcceptNewItem(ImColor(255, 50, 50), 2.0f))
            {
                int32 PinID = (int32)(uintptr_t)DraggingPinID_UI;
                UEdGraphPin* DraggingPin = Graph->FindPin(PinID);

                if (DraggingPin)
                {
                    DraggingPin->BreakAllLinks();
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
                uint32 StartPinID = static_cast<uint32>(LinkID >> 32);
                uint32 EndPinID = static_cast<uint32>(LinkID & 0xFFFFFFFF);

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
                uint32 NodeID = (uint32)(uintptr_t)DestroyedNodeID_UI;
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

void SGraphEditorWindow::HandleDoubleClick()
{
    // 더블클릭된 노드 확인
    ed::NodeId DoubleClickedNodeId = ed::GetDoubleClickedNode();
    if (!DoubleClickedNodeId)
    {
        return;
    }

    int32 NodeID = static_cast<int32>(DoubleClickedNodeId.Get());

    // 그래프에서 해당 노드 찾기
    UEdGraphNode* ClickedNode = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (Node->NodeID == NodeID)
        {
            ClickedNode = Node;
            break;
        }
    }

    if (!ClickedNode)
    {
        return;
    }

    // BlendSpace2D 노드인지 확인
    UK2Node_BlendSpace2D* BlendSpaceNode = Cast<UK2Node_BlendSpace2D>(ClickedNode);
    if (BlendSpaceNode)
    {
        // UI 데이터를 BlendSpace 객체에 동기화
        BlendSpaceNode->RebuildBlendSpace();

        if (BlendSpaceNode->BlendSpace)
        {
            // 이미 프리뷰 창이 열려있으면 닫고 새로 열기
            if (BlendSpacePreview)
            {
                delete BlendSpacePreview;
                BlendSpacePreview = nullptr;
            }

            // 새 프리뷰 창 생성
            BlendSpacePreview = new BlendSpacePreviewWindow();
            BlendSpacePreview->Initialize(BlendSpaceNode->BlendSpace, World, Device, BlendSpaceNode->PreviewMeshPath);

            UE_LOG("SGraphEditorWindow: BlendSpace2D 프리뷰 창을 열었습니다.");
        }
    }
}

void SGraphEditorWindow::RenderBlendSpacePreview()
{
    if (!BlendSpacePreview)
    {
        return;
    }

    // 프리뷰 창 렌더링
    BlendSpacePreview->OnRender();

    // 창이 닫혔으면 정리
    if (!BlendSpacePreview->IsOpen())
    {
        delete BlendSpacePreview;
        BlendSpacePreview = nullptr;
    }
}

void SGraphEditorWindow::RenderBlendSpacePreviewViewport()
{
    if (BlendSpacePreview && BlendSpacePreview->IsOpen())
    {
        BlendSpacePreview->OnRenderViewport();
    }
}

void SGraphEditorWindow::OnMouseDown(FVector2D MousePos, uint32 Button)
{
    if (BlendSpacePreview && BlendSpacePreview->IsOpen())
    {
        BlendSpacePreview->OnMouseDown(MousePos, Button);
    }
}

void SGraphEditorWindow::OnMouseUp(FVector2D MousePos, uint32 Button)
{
    if (BlendSpacePreview && BlendSpacePreview->IsOpen())
    {
        BlendSpacePreview->OnMouseUp(MousePos, Button);
    }
}

void SGraphEditorWindow::OnMouseMove(FVector2D MousePos)
{
    if (BlendSpacePreview && BlendSpacePreview->IsOpen())
    {
        BlendSpacePreview->OnMouseMove(MousePos);
    }
}

bool SGraphEditorWindow::IsMouseInBlendSpaceViewport(const FVector2D& MousePos) const
{
    if (BlendSpacePreview && BlendSpacePreview->IsOpen())
    {
        return BlendSpacePreview->GetViewportRect().Contains(MousePos);
    }
    return false;
}
