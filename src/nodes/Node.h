#pragma once

#include "Common.h"
#include <imnodes.h>
#include "NodeLink.h"


class NodeEditor;

// classifications of node types to determine validity of node connections
enum class NodeClass
{
    Invalid,
    Numeric,            // constants and math
    Pipeline,           // D3D12_GRAPHICS_PIPELINE_STATE_DESC
    PipelineComponent,  // root signature, rasterizer state, blend state, etc...
    Shader
};

// nodes for numerical operations
enum class NodeNumericSubtype
{
    // constants
    Constant,       // user-provided constant
    SystemValue,    // system value such as frame number
    // logistics
    Passthrough,    // debug display
    Splitter,
    Swizzle,
    // scalar math
    Add,
    Subtract,
    Multiply,
    Divide
};

// nodes for components which comprise a pipeline
enum class NodePipelineComponentSubtype
{
    RootSignature,
    Resource,
    BlendState,
    RasterizerState,
    DepthStencilState,
    InputLayoutDesc
};

// enum cateloging all subtypes
enum class NodeTypeFlat
{
    Null,
    Numeric,
    NumericConstant,
    NumericSystemValue,
    NumericPassthrough,
    NumericSplitter,
    NumericSwizzle,
    NumericAdd,
    NumericSubtract,
    NumericMultiply,
    NumericDivide,
    PipelineStateObject,
    PipelineComponent,
    PipelineComponentRootSignature,
    PipelineComponentResource,
    PipelineComponentBlendState,
    PipelineComponentRasterizerState,
    PipelineComponentInputLayoutDesc
};


// TODO: move into namespace and simplify naming
class Node
{
public:
    Node(NodeClass nodeClass = NodeClass::Invalid, std::string nodeName = "Unnamed Node");

    // getters
    const int Id() const                                    {return m_id;}
    const NodeClass Class() const                           {return m_nodeClass;}
    const std::string Name() const                          {return m_name;}
    virtual NodeTypeFlat NodeType() const                   {return NodeTypeFlat::Null;}

    // attribute management
    virtual const std::vector<int> Attributes() const       {return {};}
    virtual const std::vector<int> AttributesIn() const     {return {};}
    virtual const std::vector<int> AttributesOut() const    {return {};}
    virtual AttributeData* GetAttribute(const int attr)     {return nullptr;}

    // incoming/outgoing link management
    virtual void AddLink(int linkId, Node* pStart, int startAttr, Node* pEnd, int endAttr) {}
    virtual void LinkInput(int inAttr, Node* pOther, int otherAttr)     {}
    virtual void LinkOutput(int outAttr, Node* pOther, int otherAttr)   {} // TODO: multiple outputs, or force splitter
    virtual void LinkAttribute(int attr, AttributeData pOtherData)      {};

    // state updates
    const bool IsDirty() const                              {return m_isDirty;}
    void SetDirty()                                         {m_isDirty = true;}
    void ClearDirty()                                       {m_isDirty = false;}
    virtual void SetChildrenDirty()                         {}

    // primary functions
    virtual void Update()                                   {}
    virtual void Draw();

    // static counters to ensure no collisions in ImNode IDs
    static int NodeCounter;
    static int AttributeCounter;
    static int EdgeCounter;

protected:
    // draw helpers
    void DrawTitleBar();

    int          m_id;
    NodeTypeFlat m_nodeType;
    NodeClass    m_nodeClass;
    std::string  m_name;
    bool         m_isDirty;

    NodeEditor*  m_pNodeEditor;
};


//=====================================================================================================================
//                                              Primary Node Classes
//=====================================================================================================================
class NodePipelineStateDesc : public Node // populates a PSO description
{
public:
    NodePipelineStateDesc(std::string nodeName = "Unnamed PSO Node");

    virtual NodeTypeFlat NodeType() const;

    virtual void Draw();

protected:
    D3D12_GRAPHICS_PIPELINE_STATE_DESC m_psoDesc;
};


class NodeNumeric : public Node // performs math operations
{
public:
    NodeNumeric(NodeNumericSubtype subtype, std::string nodeName = "Unnamed Numeric Node");

    virtual NodeTypeFlat NodeType() const;

    virtual void Draw();

protected:

};


//=====================================================================================================================
//                                              Numeric Node Subtypes
//=====================================================================================================================
class NodeNumericConstant : public NodeNumeric // stores a constant
{
public:
    NodeNumericConstant(std::string nodeName = "Numeric Constant");

    virtual NodeTypeFlat NodeType() const;

    virtual const std::vector<int> Attributes() const;
    virtual const std::vector<int> AttributesIn() const;
    virtual const std::vector<int> AttributesOut() const;

    virtual AttributeData* GetAttribute(const int attr);

    virtual void Draw();

protected:
    float m_value;

    AttributeData m_attrInput;
    AttributeData m_attrOutput;
};
