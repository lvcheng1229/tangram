#include "global_graph_builder.h"

using namespace boost;

void CGlobalGraphsBuilder::addHashDependencyGraph(std::vector<CHashNode>& hash_dependency_graphs)
{
	Graph builded_graph;
	VertexNameMap vname_map_simple1 = get(boost::vertex_name, builded_graph);

	for (auto& iter : hash_dependency_graphs)
	{
		iter.graph_vtx_idx = add_vertex(builded_graph);
		put(vname_map_simple1, iter.graph_vtx_idx, SShaderCodeVertex{ iter.hash_value,iter.debug_string.c_str()});
	}

	for (const auto& iter : hash_dependency_graphs)
	{
		uint32_t from_vtx_id = iter.graph_vtx_idx;
		for (const auto& ipt_id : iter.out_hash_nodes)
		{
			const CHashNode& hash_node = hash_dependency_graphs[ipt_id];
			const uint32_t to_vtx_id = hash_node.graph_vtx_idx;
			add_edge(from_vtx_id, to_vtx_id, builded_graph);
		}
	}

	unmerged_graphs.push_back(builded_graph);

#if TANGRAM_DEBUG

#endif
}

void CGlobalGraphsBuilder::mergeGraphs()
{
}
