#include "NodeEditor.h"

#include "Util.h"


NodeEditor::NodeEditor() :
    m_linkCounter(0),
    m_shouldReevaluate(true)
{
}
NodeEditor::~NodeEditor()
{
}


int NodeEditor::AddNode(NodeTypeFlat nodeType, ImVec2 position)
{
    int nodeId = -1;
    Node* pNode = nullptr;

    switch (nodeType)
    {
    case NodeTypeFlat::Null:
    {
        pNode = new Node();
        break;
    }
    // Numeric
    case NodeTypeFlat::NumericConstant:
    {
        pNode = new NodeNumericConstant();
        break;
    }
    default:
    {
        PrintMessage(Error, "Unhandled Node type in AddNode: {}", magic_enum::enum_name(nodeType));
        break;
    }
    };

    if (pNode != nullptr)
    {
        m_nodes.push_back(pNode);
        nodeId = pNode->Id();
        PrintMessage(Info, "Added new node #{}: {}", nodeId, magic_enum::enum_name(nodeType));
        for (const int attr : pNode->Attributes())
        {
            m_mapAttributeToNode[attr] = pNode;
        }

        // if called, must have an accompanying BeginNode/EndNode pair for ID
        ImNodes::SetNodeScreenSpacePos(nodeId, position);
    }

    return nodeId;
}

void NodeEditor::Update()
{
    int startAttr = -1;
    int endAttr = -1;

    // check for new links
    if (ImNodes::IsLinkCreated(&startAttr, &endAttr))
    {
        PrintMessage(Info, "Attempting node link {} -> {}", startAttr, endAttr);
        Node* pStartNode = m_mapAttributeToNode[startAttr];
        Node* pEndNode   = m_mapAttributeToNode[endAttr];
        PrintMessage(Info, "\t{}", pStartNode->Name());
        PrintMessage(Info, "\t{}", pEndNode->Name());


        AttributeData* pStart = pStartNode->GetAttribute(startAttr);
        AttributeData* pEnd = pEndNode->GetAttribute(endAttr);

        if (pStart->DataType == pEnd->DataType)
        {
            NodeLink* pLink = new NodeLink;
            pLink->LinkID = m_linkCounter++;
            pLink->DataType = pStart->DataType;
            pLink->InputAttr = startAttr;
            pLink->InputNode = pStartNode;
            pLink->OutputAttr = endAttr;
            pLink->OutputNode = pEndNode;

            pStart->pLink = pLink;
            pEnd->pLink = pLink;

            m_nodeLinks.push_back(pLink);
        }
        else
        {
            PrintMessage(Error, "Attempting to link unrelated attributes");
        }
    }

    // TODO: check for deleted nodes
    // TODO: check for updated links
    // TODO: check for deleted links
    // TODO: handlers for context menu popups
}

void NodeEditor::Draw()
{
    ImGui::Begin("Node Editor", nullptr, ImGuiWindowFlags_MenuBar);

    //menu bar and controls
    DrawControls();

    // TODO: minimap

    // node editor
    {
        // list of nodes and edges
        {
            // TODO: highlight on hover
            // TODO: select on clicking list entry
            ImGuiWindowFlags listWindowFlags = ImGuiWindowFlags_HorizontalScrollbar;
            ImGui::Begin("Contents", nullptr, listWindowFlags);
            for (const Node* pNode : m_nodes)
            {
                ImGui::Text("%d %s\n    %s",
                            pNode->Id(), pNode->Name().c_str(),
                            magic_enum::enum_name(pNode->NodeType()).data());
            }

            ImGui::Separator();

            for (const NodeLink* pLink : m_nodeLinks)
            {
                ImGui::Text("%d %d->%d (%d->%d)\n    %s",
                            pLink->LinkID, pLink->InputNode->Id(), pLink->OutputNode->Id(), pLink->InputAttr, pLink->OutputAttr,
                            magic_enum::enum_name(pLink->DataType).data());
            }

            ImGui::End();
        }

        // actual editor region
        {
            ImNodes::BeginNodeEditor();

            // editor right-click context menu
            {
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
                if (ImNodes::IsEditorHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(1))
                {
                    ImGui::OpenPopup("add node");
                }

                // TODO: move to popup handler
                if (ImGui::BeginPopup("add node"))
                {
                    const ImVec2 nodePosition = ImGui::GetMousePosOnOpeningCurrentPopup();

                    if (ImGui::MenuItem("null"))
                    {
                        PrintMessage("Attempting to add Null node\n");
                        AddNode(NodeTypeFlat::Null, nodePosition);
                    }
                    else if (ImGui::MenuItem("constant"))
                    {
                        PrintMessage("Attempting to add constant node\n");
                        AddNode(NodeTypeFlat::NumericConstant, nodePosition);
                    }

                    ImGui::EndPopup();
                }
                ImGui::PopStyleVar();
            }
        }

        // TODO: node right-click context menu

        // draw nodes
        DrawNodes();

        // draw links
        for (const NodeLink* pLink : m_nodeLinks)
        {
            ImNodes::Link(pLink->LinkID, pLink->InputAttr, pLink->OutputAttr);
        }

        ImNodes::EndNodeEditor();
    }

    ImGui::End();
}

void NodeEditor::DrawControls()
{
    // TODO: useful menu options
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("node menu"))
        {
            ImGui::MenuItem("a");
            ImGui::MenuItem("b");
            ImGui::Text("some menu text");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("other node menu"))
        {
            ImGui::Text("some more text");
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void NodeEditor::DrawNodes()
{
    // draw all nodes
    for (auto pNode : m_nodes)
    {
        pNode->Draw();
    }
}
