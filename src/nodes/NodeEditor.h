#pragma once

#include <imnodes.h>
#include "Common.h"
#include "Node.h"
#include <map>


class NodeEditor
{
public:
    NodeEditor();
    ~NodeEditor();

    int AddNode(NodeTypeFlat nodeType, ImVec2 position);
    void Update();
    void Draw();

private:

    void DrawControls();
    void DrawNodes();

    uint m_linkCounter;

    std::vector<Node*> m_nodes;
    std::vector<NodeLink*> m_nodeLinks;

    std::map<int, Node*> m_mapAttributeToNode;    // used for updating nodes when links change
    std::map<int, AttributeData> m_mapAttributeToAttributeData;

    bool m_shouldReevaluate;
};
