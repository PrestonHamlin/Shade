#pragma once

#include <typeinfo>
#include <variant>
#include <imnodes.h>
#include "Util.h"


class Node;
class NodeLink;

// enum reflecting types which can be communicated via an edge/link
enum class AttributeType
{
    // numeric data types
    Float,
    Float2,
    Float3,
    Float4,
    Int,
    Int2,
    Int3,
    Int4,

    String,
    VoidPtr,    // pointer to some arbitrary bit of data
    NodePtr,    // direct pointer to another node
};

struct AttributeData
{
    int             ID;         // ID of this attribute
    AttributeType   DataType;   // data type to pass through links
    Node*           pNode;      // owning node
    NodeLink*       pLink;      // pointer to input/output link
    void*           pPassthrough;
    bool            IsInput;    // indicates if this is an input or output attribute
};

struct NodeLink
{
    int LinkID;
    AttributeType DataType;

    Node* InputNode;
    Node* OutputNode;

    int InputAttr;
    int OutputAttr;
};
