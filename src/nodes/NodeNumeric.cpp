#include "Node.h"
#include "Util.h"


//=====================================================================================================================
//                                                  Numeric Constant
//=====================================================================================================================
NodeNumericConstant::NodeNumericConstant(std::string nodeName) :
    NodeNumeric(NodeNumericSubtype::Constant, nodeName),
    m_value(3.14f),
    m_attrInput({AttributeCounter++, AttributeType::Float, this, nullptr, nullptr, true}),
    m_attrOutput({AttributeCounter++, AttributeType::Float, this, nullptr, nullptr, true})
{
}


NodeTypeFlat NodeNumericConstant::NodeType() const
{
    return NodeTypeFlat::NumericConstant;
}

const std::vector<int> NodeNumericConstant::Attributes() const
{
    return {m_attrInput.ID, m_attrOutput.ID};
}
const std::vector<int> NodeNumericConstant::AttributesIn() const
{
    return {m_attrInput.ID};
}
const std::vector<int> NodeNumericConstant::AttributesOut() const
{
    return {m_attrOutput.ID};
}

AttributeData* NodeNumericConstant::GetAttribute(const int attr)
{
    AttributeData* pResult = nullptr;

    if (attr == m_attrInput.ID)
    {
        pResult = &m_attrInput;
    }
    else if (attr == m_attrOutput.ID)
    {
        pResult = &m_attrOutput;
    }
    else
    {
        PrintMessage(Warning, "Node {} is being queried for missing attribute {}", m_id, attr);
    }

    return pResult;
}

void NodeNumericConstant::Draw()
{
    ImNodes::BeginNode(m_id);

    DrawTitleBar();

    ImNodes::BeginInputAttribute(m_attrInput.ID);
    ImNodes::EndInputAttribute();

    // default width is stupidly large
    float titleLength = ImGui::CalcTextSize((std::to_string(m_id) + ' ' + m_name).c_str()).x;
    float valueLength = ImGui::CalcTextSize(std::to_string(m_value).c_str()).x;
    float dragWidth = std::max(titleLength, valueLength) + ImGui::GetStyle().FramePadding.x * 2.0f;
    ImGui::SetNextItemWidth(dragWidth);
    ImGui::DragFloat("##ConstantNodeValue", &m_value);

    ImNodes::BeginOutputAttribute(m_attrOutput.ID);
    ImNodes::EndOutputAttribute();

    ImNodes::EndNode();
}
