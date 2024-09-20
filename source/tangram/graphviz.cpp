#include <format>
#include <set>
#include "graphviz.h"

uint32_t CGraphVis::addNode(const SNodeDesc& node_desc)
{
    nodes.emplace_back(SNode{ node_desc.lable});
    return nodes.size() - 1;
}

void CGraphVis::generate()
{


}
