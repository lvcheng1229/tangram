#include <format>
#include "graphviz.h"

uint32_t CGraphVis::addNode(const SNodeDesc& node_desc)
{
    nodes.emplace_back(SNode{ node_desc.lable});
    return nodes.size() - 1;
}

void CGraphVis::generate()
{
    std::string out_dot_file = "digraph G{";
    for (int idx = 0; idx < nodes.size(); idx++)
    {
        out_dot_file += std::format("Node_{0} [label=\"{1}\"]\n", idx, nodes[idx].lable);
    }

    for (int idx = 0; idx < edges.size(); idx++)
    {
        out_dot_file += std::format("Node_{0} -> Node_{1}\n", edges[idx].from, edges[idx].to);
    }

    out_dot_file += "}";

}
