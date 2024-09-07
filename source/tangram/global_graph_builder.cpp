#include <boost/lexical_cast.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/mcgregor_common_subgraphs.hpp>
#include <boost/property_map/shared_array_property_map.hpp>
#include <boost/graph/depth_first_search.hpp>

#include "global_graph_builder.h"
#include "graphviz.h"

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
	visGraph(&builded_graph);
#endif
}

void CGlobalGraphsBuilder::visGraph(Graph* graph)
{
	graphviz_debug_idx++;

	std::string out_dot_file = "digraph G{";

	VertexIndexMap vertex_index_map = get(vertex_index, *graph);
	VertexNameMap vertex_name_map = get(vertex_name, *graph);

	auto out_vetices = boost::vertices(*graph);
	for (auto vp = out_vetices; vp.first != vp.second; ++vp.first)
	{
		size_t property_map_idx = *vp.first;
		
		auto vtx_adj_vertices = adjacent_vertices(property_map_idx, *graph);

		if (vtx_adj_vertices.first != vtx_adj_vertices.second)
		{
			size_t  vtx_idx = vertex_index_map[property_map_idx];
			SShaderCodeVertex& shader_code_vtx = vertex_name_map[property_map_idx];

			out_dot_file += std::format("Node_{0} [label=\"{1}\"]\n", vtx_idx, shader_code_vtx.debug_string);
		}
	}

	auto es = boost::edges(*graph);
	for (auto iterator = es.first; iterator != es.second; iterator++)
	{
		size_t src_property_map_idx = source(*iterator, *graph);
		size_t dst_property_map_idx = target(*iterator, *graph);
		size_t  src_vtx_idx = vertex_index_map[src_property_map_idx];
		size_t  dst_vtx_idx = vertex_index_map[dst_property_map_idx];
		out_dot_file += std::format("Node_{0} -> Node_{1}\n", src_vtx_idx, dst_vtx_idx);
	}

	out_dot_file += "}";
}

void CGlobalGraphsBuilder::mergeGraphs()
{
}

class CMergeGraphDFSVisitor : public boost::default_dfs_visitor
{
public:
	CMergeGraphDFSVisitor(CGlobalGraphsBuilder::Graph ipt_new_graph)
		:new_graph(ipt_new_graph) {}


	void examine_edge(SShaderCodeEdge e, CGlobalGraphsBuilder::Graph g)const
	{

	}

private:
	CGlobalGraphsBuilder::Graph new_graph;
};

struct SOutputMaxComSubGraph
{
public:
	typedef typename boost::graph_traits< CGlobalGraphsBuilder::Graph >::vertices_size_type VertexSizeFirst;

	SOutputMaxComSubGraph(const CGlobalGraphsBuilder::Graph& graph1) : m_graph1(graph1) {}

	template < typename CorrespondenceMapFirstToSecond, typename CorrespondenceMapSecondToFirst >
	bool operator()(CorrespondenceMapFirstToSecond correspondence_map_1_to_2, CorrespondenceMapSecondToFirst correspondence_map_2_to_1, VertexSizeFirst subgraph_size)
	{

		// Fill membership map for first graph
		typedef typename property_map< CGlobalGraphsBuilder::Graph, vertex_index_t >::type VertexIndexMap;
		typedef shared_array_property_map< bool, VertexIndexMap > MembershipMap;

		MembershipMap membership_map1(num_vertices(m_graph1), get(vertex_index, m_graph1));

		fill_membership_map< CGlobalGraphsBuilder::Graph >(m_graph1, correspondence_map_1_to_2, membership_map1);

		// Generate filtered graphs using membership map
		typedef typename membership_filtered_graph_traits< CGlobalGraphsBuilder::Graph, MembershipMap >::graph_type MembershipFilteredGraph;

		MembershipFilteredGraph subgraph1 = make_membership_filtered_graph(m_graph1, membership_map1);

		// Print the graph out to the console
		std::cout << "Found common subgraph (size " << subgraph_size << ")"
			<< std::endl;
		print_graph(subgraph1);
		std::cout << std::endl;

		// Explore the entire space
		return (true);
	}

private:
	//std::set<>*;
	//std::map<>*;

	const CGlobalGraphsBuilder::Graph& m_graph1;
	VertexSizeFirst m_max_subgraph_size;
};

CGlobalGraphsBuilder::Graph CGlobalGraphsBuilder::mergeGraph(Graph* graph_a, Graph* graph_b)
{
	VertexNameMap vtx_name_map_a = get(boost::vertex_name, *graph_a);
	VertexNameMap vtx_name_map_b = get(boost::vertex_name, *graph_b);

	SOutputMaxComSubGraph output_maxcom_graph(*graph_a);

	mcgregor_common_subgraphs_maximum_unique(graph_a, graph_b, true, output_maxcom_graph,
		vertices_equivalent(make_property_map_equivalent(vtx_name_map_a, vtx_name_map_b)));
	
	Graph new_graph = *graph_a;


	CMergeGraphDFSVisitor merge_graph_dfs_visitor(new_graph);
	depth_first_search(*graph_a, visitor(merge_graph_dfs_visitor));

	//��AͼΪ������DFS����Bͼ
	//����Bͼ�������������κ�һ������
	//	��������������󹫹���ͼ�Ľڵ�֮һ
	//		����Ҫ�ٴμ���
	//	������ǹ����ڵ�
	//		���ӸĽڵ㣬�����Ľڵ����߼��뵽ͼ��
	//		�������г���
	//			������߶�Ӧ������������󹫹���ͼ�Ľڵ�֮һ
	//				����һ���ߣ������������



	return Graph();
}

void initialGlobalShaderGraphBuild()
{
}

void addHashedGraphToGlobalGraphBuilder(std::vector<CHashNode>& hash_dependency_graphs)
{
}

void finalizeGlobalShaderGraphBuild()
{
}
