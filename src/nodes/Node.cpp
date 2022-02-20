#include "Node.h"


int Node::NodeCounter       = 0;
int Node::AttributeCounter  = 0;
int Node::EdgeCounter       = 0;


//=====================================================================================================================
//                                                  Null Node
//=====================================================================================================================
Node::Node(NodeClass nodeClass, std::string nodeName) :
    m_id(NodeCounter++),
    m_nodeClass(nodeClass),
    m_name(nodeName)
{
}

void Node::Draw()
{
    ImNodes::BeginNode(m_id);

    DrawTitleBar();

    ImNodes::EndNode();
}

void Node::DrawTitleBar()
{
    ImNodes::BeginNodeTitleBar();
    ImGui::Text("%u| %s", m_id, m_name.c_str());
    ImNodes::EndNodeTitleBar();
}


//=====================================================================================================================
//                                          Pipeline State Descriptor Node
//=====================================================================================================================
NodePipelineStateDesc::NodePipelineStateDesc(std::string nodeName) :
    Node(NodeClass::Pipeline, nodeName)
{
}

NodeTypeFlat NodePipelineStateDesc::NodeType() const
{
    return NodeTypeFlat::PipelineStateObject;
}

void NodePipelineStateDesc::Draw()
{
    ImNodes::BeginNode(m_id);

    // title bar
    ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(11, 109, 191, 255));
    DrawTitleBar();
    ImNodes::PopColorStyle();

    // input nodes
    //ImNodes::BeginInputAttribute

    // output nodes?
    //ImNodes::BeginOutputAttribute(pNode->id);
    //ImNodes::EndOutputAttribute();

    ImNodes::EndNode();
}


//=====================================================================================================================
//                                                  Numeric Node
//=====================================================================================================================
NodeNumeric::NodeNumeric(NodeNumericSubtype subtype, std::string nodeName) :
    Node(NodeClass::Numeric, nodeName)
{
}

NodeTypeFlat NodeNumeric::NodeType() const
{
    return NodeTypeFlat::NumericConstant;
}

void NodeNumeric::Draw()
{
    ImNodes::BeginNode(m_id);

    // title bar
    ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(40, 40, 40, 255));
    DrawTitleBar();
    ImNodes::PopColorStyle();

    // input nodes
    //ImNodes::BeginInputAttribute

    // output nodes?
    //ImNodes::BeginOutputAttribute(pNode->id);
    //ImNodes::EndOutputAttribute();

    ImNodes::EndNode();
}
