#include "global_graph_builder.h"
#include "tangram_utility.h"

#include <fstream>

#include <boost/lexical_cast.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/mcgregor_common_subgraphs.hpp>
#include <boost/property_map/shared_array_property_map.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/topological_sort.hpp>

#define DOT_FILE_TO_PNG 1

//todo：
// addHashDependencyGraph中 对Graph进行处理的时候有bug，目前存在两点问题
// 两个subgraph完全不相连的情况，比如Shader 计算RT1和RT2的代码之间完全没有关系
// 只有一个节点，并且不相连的情况

static CGlobalGraphsBuilder* global_graph_builder = nullptr;
static CGlobalAstNodeRecursiveCopy* global_ast_node_recursive_copy = nullptr;
using namespace tangram;
using namespace boost;

void buildGraphVertexInputEdges(CGraph& graph, std::map<SGraphVertexDesc, std::vector<SGraphEdgeDesc>>& vertex_input_edges)
{
	STopologicalOrderVetices topological_order_vertices;
	topological_sort(graph, std::back_inserter(topological_order_vertices));

	for (STopologicalOrderVetices::reverse_iterator vtx_desc_iter = topological_order_vertices.rbegin(); vtx_desc_iter != topological_order_vertices.rend(); ++vtx_desc_iter)
	{
		boost::graph_traits<CGraph>::out_edge_iterator e, e_end;
		for (boost::tie(e, e_end) = out_edges(*vtx_desc_iter, graph); e != e_end; ++e)
		{
			const auto& target_vtx_desc = target(*e, graph);
			vertex_input_edges[target_vtx_desc].push_back(*e);
		}
	}
}

void CGlobalGraphsBuilder::addHashDependencyGraph(std::vector<CHashNode>& hash_dependency_graphs, int shader_id)
{
	CGraph builded_graph;
	VertexNameMap vtx_name_map = get(boost::vertex_name, builded_graph);
	EdgeNameMap edge_name_map = get(boost::edge_name, builded_graph);

	for (auto& iter : hash_dependency_graphs)
	{
		iter.graph_vtx_idx = add_vertex(builded_graph);
		SShaderCodeVertexInfomation shader_code_info;
		shader_code_info.input_variable_num = iter.ipt_symbol_name_order_map.size();
		shader_code_info.output_variable_num = iter.opt_symbol_name_order_map.size();
		for (const auto& out_iter : iter.opt_symbol_name_order_map)
		{
			XXH64_hash_t symbol_hash = out_iter.first;
			const auto& input_iter = iter.ipt_symbol_name_order_map.find(symbol_hash);
			if (input_iter != iter.ipt_symbol_name_order_map.end())
			{
				shader_code_info.inout_variable_in2out[input_iter->second] = out_iter.second;
				shader_code_info.inout_variable_out2in[out_iter.second] = input_iter->second;
			}
		}
		shader_code_info.related_shaders.push_back(shader_id);
		
		SShaderCodeVertex shader_code_vertex;
		shader_code_vertex.hash_value = iter.hash_value;
		shader_code_vertex.vtx_info = shader_code_info;
		shader_code_vertex.debug_string = iter.debug_string.c_str();

		put(vtx_name_map, iter.graph_vtx_idx, shader_code_vertex);
	}

	for (const auto& iter : hash_dependency_graphs)
	{
		uint32_t from_vtx_id = iter.graph_vtx_idx;
		for (const auto& ipt_id : iter.out_hash_nodes)
		{
			const CHashNode& hash_node = hash_dependency_graphs[ipt_id];
			const uint32_t to_vtx_id = hash_node.graph_vtx_idx;
			const auto& edge_desc = add_edge(from_vtx_id, to_vtx_id, builded_graph);

			// todo: multi input node
			SShaderCodeEdge shader_code_edge;
			for (int ipt_symbol_idx = 0; ipt_symbol_idx < hash_node.ordered_input_symbols_hash.size(); ipt_symbol_idx++)
			{
				const XXH64_hash_t& symbol_hash = hash_node.ordered_input_symbols_hash[ipt_symbol_idx];
				const auto& from_node_symbol_idx = iter.opt_symbol_name_order_map.find(symbol_hash);
				if (from_node_symbol_idx != iter.opt_symbol_name_order_map.end())
				{
					assert(shader_code_edge.variable_map_next_to_pre.find(ipt_symbol_idx) == shader_code_edge.variable_map_next_to_pre.end());
					shader_code_edge.variable_map_next_to_pre[ipt_symbol_idx] = from_node_symbol_idx->second;
					shader_code_edge.variable_map_pre_to_next[from_node_symbol_idx->second] = ipt_symbol_idx;
				}
			}
			put(edge_name_map, edge_desc.first, shader_code_edge);
			
		}
	}

	// 对Graph进行处理，移除所有孤立点，即和当前Graph不相连的点
	CGraph processed_graph;
	{
		VertexNameMap new_vtx_name_map = get(boost::vertex_name, processed_graph);
		EdgeNameMap new_edge_name_map = get(boost::edge_name, processed_graph);

		VertexIndexMap vertex_index_map = get(vertex_index, builded_graph);
		VertexNameMap vertex_name_map = get(vertex_name, builded_graph);
		EdgeNameMap edge_name_map = get(edge_name, builded_graph);
		
		std::map<size_t, size_t> map_src_dst2;// = map_src_dst;

		auto es = boost::edges(builded_graph);
		for (auto iterator = es.first; iterator != es.second; iterator++)
		{
			size_t src_vtx_desc = source(*iterator, builded_graph);
			size_t dst_vtx_desc = target(*iterator, builded_graph);

			SShaderCodeVertex& src_shader_code_vtx = vertex_name_map[src_vtx_desc];
			SShaderCodeVertex& dst_shader_code_vtx = vertex_name_map[dst_vtx_desc];

			auto iter_from = map_src_dst2.find(src_vtx_desc);
			auto iter_dst = map_src_dst2.find(dst_vtx_desc);

			if (iter_from == map_src_dst2.end())
			{
				size_t dst = add_vertex(processed_graph);
				put(new_vtx_name_map, dst, src_shader_code_vtx);
				map_src_dst2[src_vtx_desc] = dst;
			}

			if (iter_dst == map_src_dst2.end())
			{
				size_t dst = add_vertex(processed_graph);
				put(new_vtx_name_map, dst, dst_shader_code_vtx);
				map_src_dst2[dst_vtx_desc] = dst;
			}
		}

		for (auto iterator = es.first; iterator != es.second; iterator++)
		{
			size_t src_property_map_idx = source(*iterator, builded_graph);
			size_t dst_property_map_idx = target(*iterator, builded_graph);
			const size_t  src_vtx_idx = vertex_index_map[src_property_map_idx];
			const size_t  dst_vtx_idx = vertex_index_map[dst_property_map_idx];
			auto iter_from = map_src_dst2.find(src_vtx_idx);
			auto iter_dst = map_src_dst2.find(dst_vtx_idx);
			if (iter_from != map_src_dst2.end() && iter_dst != map_src_dst2.end())
			{
				const auto& edge_desc = add_edge(iter_from->second, iter_dst->second, processed_graph);
				const SShaderCodeEdge& shader_code_edge = edge_name_map[*iterator];
				put(new_edge_name_map, edge_desc.first, shader_code_edge);
			}
		}
	}

	unmerged_graphs.push_back(processed_graph);

#if TANGRAM_DEBUG
	visGraph(&processed_graph);
#endif
}

#if TANGRAM_DEBUG
void CGlobalGraphsBuilder::visGraph(CGraph* graph, bool visualize_symbol_name, bool visualize_shader_id, bool visualize_graph_partition)
{
	graphviz_debug_idx++;

	std::string out_dot_file = "digraph G{";

	VertexIndexMap vertex_index_map = get(vertex_index, *graph);
	VertexNameMap vertex_name_map = get(vertex_name, *graph);
	EdgeNameMap edge_name_map = get(edge_name, *graph);

	auto out_vetices = boost::vertices(*graph);
	for (auto vp = out_vetices; vp.first != vp.second; ++vp.first)
	{
		size_t property_map_idx = *vp.first;
		
		auto vtx_adj_vertices = adjacent_vertices(property_map_idx, *graph);
	
		//if (vtx_adj_vertices.first != vtx_adj_vertices.second)
		{
			size_t  vtx_idx = vertex_index_map[property_map_idx];
			SShaderCodeVertex& shader_code_vtx = vertex_name_map[property_map_idx];
	
			out_dot_file += std::format("Node_{0} [label=\"{1}", vtx_idx, shader_code_vtx.debug_string);

			SShaderCodeVertexInfomation& vtx_info = shader_code_vtx.vtx_info;

			if (visualize_symbol_name)
			{
				if (vtx_info.ipt_variable_names.size() != 0 || vtx_info.opt_variable_names.size() != 0)
				{
					out_dot_file += "\n";
				}

				if (vtx_info.ipt_variable_names.size() > 0)
				{
					out_dot_file += "input symbols name:\n";
					for (int ipt_symbol_idx = 0; ipt_symbol_idx < vtx_info.ipt_variable_names.size(); ipt_symbol_idx++)
					{
						out_dot_file += std::format("input symbol {0}: {1}\n", ipt_symbol_idx, vtx_info.ipt_variable_names[ipt_symbol_idx]);
					}
				}

				if (vtx_info.opt_variable_names.size() > 0)
				{
					out_dot_file += "ouput symbols name:\n";
					for (int opt_symbol_idx = 0; opt_symbol_idx < vtx_info.opt_variable_names.size(); opt_symbol_idx++)
					{
						out_dot_file += std::format("output symbol {0}: {1}\n", opt_symbol_idx, vtx_info.opt_variable_names[opt_symbol_idx]);
					}
				}
			}

			if (visualize_shader_id)
			{
				out_dot_file += "\n";
				out_dot_file += "Shader IDs: ";
				for (int shader_id_idx = 0; shader_id_idx < vtx_info.related_shaders.size(); shader_id_idx++)
				{
					out_dot_file += std::to_string(vtx_info.related_shaders[shader_id_idx]);
					out_dot_file += " ";
				}
			}

			if (visualize_graph_partition)
			{
				out_dot_file += "\n";
				out_dot_file += "Code Block Index: ";
				out_dot_file += std::to_string(shader_code_vtx.code_block_index);
			}
			

			out_dot_file += "\"";
			if (visualize_shader_id)
			{
				std::hash<int> int_hash;
				std::size_t hash_value = int_hash(shader_code_vtx.code_block_index);
				char red = (hash_value & 0xFF); red = red < 120 ? (red * 2) : red;
				char green = ((hash_value >> 8)& 0xFF); green = green < 80 ? (green * 2) : green;
				char blue = (hash_value >> 16)& 0xFF; blue = blue < 60 ? (blue * 2) : blue;
				out_dot_file += std::format(",style=\"filled\",color = \"#{:x}{:x}{:x}\"", red, green, blue);
			}
			out_dot_file += " ]\n";
		}
	}
	
	auto es = boost::edges(*graph);
	for (auto iterator = es.first; iterator != es.second; iterator++)
	{
		size_t src_property_map_idx = source(*iterator, *graph);
		size_t dst_property_map_idx = target(*iterator, *graph);
		size_t  src_vtx_idx = vertex_index_map[src_property_map_idx];
		size_t  dst_vtx_idx = vertex_index_map[dst_property_map_idx];
		out_dot_file += std::format("Node_{0} -> Node_{1}", src_vtx_idx, dst_vtx_idx);
		const auto& edge_desc = edge(src_vtx_idx, dst_vtx_idx, *graph);
		const SShaderCodeEdge& edge = edge_name_map[edge_desc.first];
		if (edge.variable_map_next_to_pre.size() != 0)
		{
			out_dot_file += "[label = \"";
			for (auto var_map_iter = edge.variable_map_next_to_pre.begin(); var_map_iter != edge.variable_map_next_to_pre.end(); var_map_iter++)
			{
				out_dot_file += std::format("var {0} -> var {1}\n", var_map_iter->first, var_map_iter->second);
			}
			out_dot_file += "\"]";
		}

		out_dot_file += ";\n";
	}

	out_dot_file += "}";

	std::string dot_file_path = intermediatePath() + std::to_string(graphviz_debug_idx) + ".dot";
	std::ofstream output_dotfile(dot_file_path);

	output_dotfile << out_dot_file;

	output_dotfile.close();

#if DOT_FILE_TO_PNG
	std::string dot_to_png_cmd = "dot -Tpng " + dot_file_path + " -o " + intermediatePath() + std::to_string(graphviz_debug_idx) + ".png";
	system(dot_to_png_cmd.c_str());
#endif
}
#endif


struct SMcsDesc
{
	CGraph* m_graph1;
	CGraph* m_graph2;
	SMcsResult* result;
	bool m_need_index_mapping = false;
	std::map<size_t, size_t>* m_index_mapping[2];
};

struct SOutputMaxComSubGraph
{
public:
	typedef typename boost::graph_traits< CGraph >::vertices_size_type VertexSizeFirst;
	typedef shared_array_property_map< bool, VertexIndexMap > MembershipMap;
	typedef typename membership_filtered_graph_traits< CGraph, MembershipMap >::graph_type MembershipFilteredGraph;

	SOutputMaxComSubGraph(SMcsDesc& mcsDesc):
		m_graph1(*mcsDesc.m_graph1),
		m_graph2(*mcsDesc.m_graph2),
		result(mcsDesc.result),
		m_need_index_mapping(mcsDesc.m_need_index_mapping)
	{
		m_index_mapping[0] = mcsDesc.m_index_mapping[0];
		m_index_mapping[1] = mcsDesc.m_index_mapping[1];
	}

	template < typename CorrespondenceMapFirstToSecond, typename CorrespondenceMapSecondToFirst >
	bool operator()(CorrespondenceMapFirstToSecond correspondence_map_1_to_2, CorrespondenceMapSecondToFirst correspondence_map_2_to_1, VertexSizeFirst subgraph_size)
	{
		//todo: if total sub graph weight < xx, skip
		result->subgraph_size = subgraph_size;
		if (subgraph_size < 3)
		{
			return true;
		}
		
#if TANGRAM_DEBUG
		std::cout << "Found common subgraph (size " << subgraph_size << ")" << std::endl;
#endif
		//graph1 common vertices indices
		{
			MembershipMap membership_map(num_vertices(m_graph1), get(vertex_index, m_graph1));
			fill_membership_map< CGraph >(m_graph1, correspondence_map_1_to_2, membership_map);
			MembershipFilteredGraph subgraph = make_membership_filtered_graph(m_graph1, membership_map);
			output_common_vtx_indices(subgraph, 1 - 1,false, correspondence_map_1_to_2);
		}

		{
			MembershipMap membership_map(num_vertices(m_graph2), get(vertex_index, m_graph2));
			fill_membership_map< CGraph >(m_graph2, correspondence_map_2_to_1, membership_map);
			MembershipFilteredGraph subgraph = make_membership_filtered_graph(m_graph2, membership_map);
			output_common_vtx_indices(subgraph, 2 - 1,true, correspondence_map_2_to_1);
		}
		
		return (true);
	}

	template < typename CorrespondenceMap>
	void output_common_vtx_indices(MembershipFilteredGraph& subgraph, int output_graph_index, bool generate_vtx_map, CorrespondenceMap vtx_map)
	{
#if TANGRAM_DEBUG
		print_graph(subgraph);
		std::cout << std::endl;
#endif

		VertexIndexMap vertex_index_map = get(vertex_index, subgraph);
		VertexNameMap vertex_name_map = get(vertex_name, subgraph);
		auto common_vetices = boost::vertices(subgraph);
		for (auto vp = common_vetices; vp.first != vp.second; ++vp.first)
		{
			size_t vtx_desc = *vp.first;
			size_t vtx_index = vertex_index_map[vtx_desc];
			if (m_need_index_mapping)
			{
				vtx_index = (*(m_index_mapping[output_graph_index]))[vtx_index];
			}
			result->graph_common_vtx_indices[output_graph_index].insert(vtx_index);
			if (generate_vtx_map)
			{
				assert(output_graph_index == 1);
				
				// map b to a
				size_t vtx_desc_a = get(vtx_map, vtx_desc);
				if (m_need_index_mapping)
				{
					vtx_desc_a = (*(m_index_mapping[0]))[vtx_desc_a];
				}
				result->vtx_desc_map_b2a[vtx_index] = vtx_desc_a;
			}
#if TANGRAM_DEBUG
			const SShaderCodeVertex& shader_code_vtx = vertex_name_map[vtx_desc];
			std::cout << "common subgraph vertex, vertex index: " << vtx_index << " vertex hash string " << shader_code_vtx.debug_string << std::endl;
#endif
		};
	}

private:

	bool m_need_index_mapping = false;
	std::map<size_t, size_t>* m_index_mapping[2];
	SMcsResult* result;
	const CGraph& m_graph1;
	const CGraph& m_graph2;
};




void CGlobalGraphsBuilder::findCommonSubgraphMax(CGraph& graph_a, CGraph& graph_b, SMcsResult& mcsResult)
{
	// iteration 0
	{
		VertexNameMap vtx_name_map_a = get(boost::vertex_name, graph_a);
		VertexNameMap vtx_name_map_b = get(boost::vertex_name, graph_b);
		SMcsDesc mcs_desc;
		mcs_desc.m_graph1 = &graph_a;
		mcs_desc.m_graph2 = &graph_b;
		mcs_desc.result = &mcsResult;
		mcs_desc.m_need_index_mapping = false;
		mcs_desc.m_index_mapping[0] = nullptr;
		mcs_desc.m_index_mapping[1] = nullptr;
		SOutputMaxComSubGraph output_maxcom_graph(mcs_desc);
		mcgregor_common_subgraphs_maximum_unique(graph_a, graph_b, true, output_maxcom_graph, vertices_equivalent(make_property_map_equivalent(vtx_name_map_a, vtx_name_map_b)));
	}

	CGraph pre_iteration_graph_a = graph_a;
	CGraph pre_iteration_graph_b = graph_b;

	std::map<size_t, size_t> pre_map_new_to_origin_a;
	std::map<size_t, size_t> pre_map_new_to_origin_b;

	while (mcsResult.subgraph_size >= 3)
	{
		std::map<size_t, size_t> map_new_to_origin0;
		std::map<size_t, size_t> map_new_to_origin1;

		CGraph generated_sub_graph_a;
		CGraph generated_sub_graph_b;
		generateSubgraphRemoveMcs(pre_iteration_graph_a, mcsResult.graph_common_vtx_indices[0], pre_map_new_to_origin_a, generated_sub_graph_a, map_new_to_origin0);
		generateSubgraphRemoveMcs(pre_iteration_graph_b, mcsResult.graph_common_vtx_indices[1], pre_map_new_to_origin_b, generated_sub_graph_b, map_new_to_origin1);

		VertexNameMap vtx_name_map_a = get(boost::vertex_name, generated_sub_graph_a);
		VertexNameMap vtx_name_map_b = get(boost::vertex_name, generated_sub_graph_b);

		SMcsDesc mcs_desc;
		mcs_desc.m_graph1 = &generated_sub_graph_a;
		mcs_desc.m_graph2 = &generated_sub_graph_b;
		mcs_desc.result = &mcsResult;
		mcs_desc.m_need_index_mapping = true;
		mcs_desc.m_index_mapping[0] = &map_new_to_origin0;
		mcs_desc.m_index_mapping[1] = &map_new_to_origin1;
		SOutputMaxComSubGraph output_maxcom_graph(mcs_desc);
		mcgregor_common_subgraphs_maximum_unique(generated_sub_graph_a, generated_sub_graph_b, true, output_maxcom_graph, vertices_equivalent(make_property_map_equivalent(vtx_name_map_a, vtx_name_map_b)));

		pre_map_new_to_origin_a = map_new_to_origin0;
		pre_map_new_to_origin_b = map_new_to_origin1;

		pre_iteration_graph_a.clear();
		pre_iteration_graph_b.clear();

		pre_iteration_graph_a = generated_sub_graph_a;
		pre_iteration_graph_b = generated_sub_graph_b;
	}
}

void CGlobalGraphsBuilder::generateSubgraphRemoveMcs(CGraph& origin_graph, const std::set<size_t>& mcs_vtx_indices, const std::map<size_t, size_t>& pre_map_new_to_origin, CGraph& generated_subgraph, std::map<size_t, size_t>& map_new_to_origin)
{
	VertexNameMap new_vertex_name_map = get(vertex_name, generated_subgraph);

	std::map<size_t, size_t> map_origin_to_new;

	VertexIndexMap origin_vertex_index_map = get(vertex_index, origin_graph);
	VertexNameMap origin_vertex_name_map = get(vertex_name, origin_graph);
	auto out_vetices = boost::vertices(origin_graph);

	for (auto vp = out_vetices; vp.first != vp.second; ++vp.first)
	{
		size_t property_map_idx = *vp.first;
		size_t  vtx_idx = origin_vertex_index_map[property_map_idx];
		SShaderCodeVertex& shader_code_vtx = origin_vertex_name_map[property_map_idx];

		if (mcs_vtx_indices.find(vtx_idx) == mcs_vtx_indices.end())
		{
			size_t new_vtx_index = add_vertex(generated_subgraph);
			put(new_vertex_name_map, new_vtx_index, shader_code_vtx);

			if (pre_map_new_to_origin.size() != 0)
			{
				map_new_to_origin[new_vtx_index] = pre_map_new_to_origin.find(vtx_idx)->second;
			}
			else
			{
				map_new_to_origin[new_vtx_index] = vtx_idx;
			}
			
			map_origin_to_new[vtx_idx] = new_vtx_index;
		}
	}

	auto es = boost::edges(origin_graph);
	for (auto iterator = es.first; iterator != es.second; iterator++)
	{
		size_t src_property_map_idx = source(*iterator, origin_graph);
		size_t dst_property_map_idx = target(*iterator, origin_graph);

		size_t src_vtx_index = origin_vertex_index_map[src_property_map_idx];
		size_t dst_vtx_index = origin_vertex_index_map[dst_property_map_idx];

		if ((mcs_vtx_indices.find(src_vtx_index) == mcs_vtx_indices.end()) && (mcs_vtx_indices.find(dst_vtx_index) == mcs_vtx_indices.end()))
		{
			size_t src_new_vtx_idx = map_origin_to_new.find(src_vtx_index)->second;
			size_t dst_new_vtx_idx = map_origin_to_new.find(dst_vtx_index)->second;
			add_edge(src_new_vtx_idx, dst_new_vtx_idx, generated_subgraph);
		}

	}

	visGraph(&generated_subgraph);
}

void CGlobalGraphsBuilder::mergeGraphs()
{
	//for (int test_index = 0; test_index < 4; test_index += 2)
	int test_index = 1;
	{
		mergeGraph(&unmerged_graphs[test_index], &unmerged_graphs[test_index + 1]);
	}
}

class CStartVertexVisitor : public boost::default_dfs_visitor
{
public:
	CStartVertexVisitor(CGraph& ipt_graph, std::map<XXH64_hash_t, size_t>* ipt_vtx_hash_to_vtx_desc, std::set<size_t>* ipt_vertices_desc)
		:ipt_vtx_hash_to_vtx_desc(ipt_vtx_hash_to_vtx_desc)
		, ipt_vertices_desc(ipt_vertices_desc)
	{
		vtx_name_map = get(boost::vertex_name, ipt_graph);
	}
	using Vertex = boost::graph_traits<CGraph>::vertex_descriptor;

	void start_vertex(Vertex vtx_desc, const CGraph& graph)
	{
		const SShaderCodeVertex& shader_vtx = vtx_name_map[vtx_desc];
		if (ipt_vtx_hash_to_vtx_desc)
		{
			(*ipt_vtx_hash_to_vtx_desc)[shader_vtx.hash_value] = vtx_desc;
		}

		if (ipt_vertices_desc)
		{
			(*ipt_vertices_desc).insert(vtx_desc);
		}

#if TANGRAM_DEBUG
		std::cout << "start vertex of graph a: " << shader_vtx.debug_string << std::endl;
#endif
	}

	std::map<XXH64_hash_t, size_t>* ipt_vtx_hash_to_vtx_desc;
	std::set<size_t>* ipt_vertices_desc;
	VertexNameMap vtx_name_map;
};

class CGraphMerger
{
public:
	using Vertex = boost::graph_traits<CGraph>::vertex_descriptor;
	using Edge = boost::graph_traits<CGraph>::edge_descriptor;

	CGraphMerger(CGraph& ipt_new_graph, CGraph& ipt_graph_b, std::map<XXH64_hash_t, size_t>& ipt_vtx_hash_to_vtx_desc_a, std::set<size_t>& mcs_vertices_b, std::map<size_t, size_t>& mcs_vtx_desc_map_b2a) :
		new_graph(ipt_new_graph),
		m_graph_b(ipt_graph_b),
		mcs_vertices_b(mcs_vertices_b),
		mcs_vtx_desc_map_b2a(mcs_vtx_desc_map_b2a),
		ipt_vtx_hash_to_vtx_desc_a(ipt_vtx_hash_to_vtx_desc_a)
	{
		vtx_name_map_b = get(boost::vertex_name, m_graph_b);
		edge_name_map_b = get(boost::edge_name, m_graph_b);
		vtx_name_map_new_graph = get(boost::vertex_name, new_graph);
		edge_name_map_new_graph = get(boost::edge_name, new_graph);

		CStartVertexVisitor ipt_vtx_visitor_b(m_graph_b, nullptr, &st_vtx_descs_b);
		depth_first_search(m_graph_b, visitor(ipt_vtx_visitor_b));

		topological_sort(m_graph_b, std::back_inserter(topological_order_vertices));

#if TANGRAM_DEBUG
		for (STopologicalOrderVetices::reverse_iterator vtx_desc_iter = topological_order_vertices.rbegin(); vtx_desc_iter != topological_order_vertices.rend(); ++vtx_desc_iter)
		{
			const SShaderCodeVertex& shader_vtx = vtx_name_map_b[*vtx_desc_iter];
			std::cout << "topologic vertices order output:" << *vtx_desc_iter << " hash string:" << shader_vtx.debug_string << std::endl;
		}
#endif
	}

	void mergeGraph()
	{
		for (STopologicalOrderVetices::reverse_iterator vtx_desc_iter = topological_order_vertices.rbegin(); vtx_desc_iter != topological_order_vertices.rend(); ++vtx_desc_iter)
		{
			iteratorVertex(*vtx_desc_iter, m_graph_b);
		}
	}

	void iteratorVertex(Vertex vtx_desc, const CGraph& graph_b)
	{
		const SShaderCodeVertex& shader_vtx = vtx_name_map_b[vtx_desc];
		const std::vector<int>& graphb_vertex_related_shaders = shader_vtx.vtx_info.related_shaders;

#if TANGRAM_DEBUG
		std::cout << shader_vtx.debug_string << std::endl;
#endif

		bool is_start_vertex = (st_vtx_descs_b.find(vtx_desc) != st_vtx_descs_b.end());
		if (is_start_vertex)
		{
			auto start_vtx_iter = ipt_vtx_hash_to_vtx_desc_a.find(shader_vtx.hash_value);
			if (start_vtx_iter != ipt_vtx_hash_to_vtx_desc_a.end())
			{
				// don't add
				map_idx_b2ng[vtx_desc] = start_vtx_iter->second;
				SShaderCodeVertex& merged_shader_code_vertex = vtx_name_map_new_graph[start_vtx_iter->second];
				std::vector<int>& merged_vertex_related_shaders = merged_shader_code_vertex.vtx_info.related_shaders;
				merged_vertex_related_shaders.insert(merged_vertex_related_shaders.end(), graphb_vertex_related_shaders.begin(), graphb_vertex_related_shaders.end());
			}
			else
			{
				size_t mew_vtx_idx = add_vertex(new_graph);
				put(vtx_name_map_new_graph, mew_vtx_idx, shader_vtx);
				map_idx_b2ng[vtx_desc] = mew_vtx_idx;

				// for each output edges
				auto es = out_edges(vtx_desc, graph_b);
				for (auto iterator = es.first; iterator != es.second; iterator++)
				{
					size_t dst_vtx_desc = target(*iterator, graph_b);
					if (isCommonSubGraphVerticesB(dst_vtx_desc))
					{
						Vertex correspond_vtx_desc = mcs_vtx_desc_map_b2a.find(dst_vtx_desc)->second;
						const auto& edge_desc = add_edge(mew_vtx_idx, correspond_vtx_desc, new_graph);
						put(edge_name_map_new_graph, edge_desc.first, edge_name_map_b[*iterator]);
					}
				}
			}
		}
		else
		{
			bool is_common_subgraph_vertices = isCommonSubGraphVerticesB(vtx_desc);
			if (is_common_subgraph_vertices)
			{
				Vertex correspond_vtx_desc = mcs_vtx_desc_map_b2a.find(vtx_desc)->second;
				SShaderCodeVertex& merged_shader_code_vertex = vtx_name_map_new_graph[correspond_vtx_desc];
				std::vector<int>& merged_vertex_related_shaders = merged_shader_code_vertex.vtx_info.related_shaders;
				merged_vertex_related_shaders.insert(merged_vertex_related_shaders.end(), graphb_vertex_related_shaders.begin(), graphb_vertex_related_shaders.end());
				map_idx_b2ng[vtx_desc] = correspond_vtx_desc;
			}
			else
			{
				// 如果该节点不是Common Subgraph里面的一个节点，新加一个节点，并进行以下操作
				// 遍历该节点的所有入边，将其加入到merged graph里面
				// 遍历该节点的所有出边，如果出边的Dst节点在CommonSubGraph里面，将其加入到merged graph里面

				size_t mew_vtx_idx = add_vertex(new_graph);
				put(vtx_name_map_new_graph, mew_vtx_idx, shader_vtx);
				map_idx_b2ng[vtx_desc] = mew_vtx_idx;

				//for each input edges
				for (auto& ipt_edge_vtx : input_edges_b[vtx_desc])
				{
					auto ipt_edge_vtx_a = map_idx_b2ng.find(ipt_edge_vtx)->second;
					const auto& new_edge_desc = add_edge(ipt_edge_vtx_a, mew_vtx_idx, new_graph);
					const auto& origin_edge_desc = edge(ipt_edge_vtx, vtx_desc, m_graph_b);
					put(edge_name_map_new_graph, new_edge_desc.first, edge_name_map_b[origin_edge_desc.first]);
				}

				// for each output edges
				auto es = out_edges(vtx_desc, graph_b);
				for (auto iterator = es.first; iterator != es.second; iterator++)
				{
					size_t dst_vtx_desc = target(*iterator, graph_b);
					if (isCommonSubGraphVerticesB(dst_vtx_desc))
					{
						Vertex correspond_vtx_desc = mcs_vtx_desc_map_b2a.find(dst_vtx_desc)->second;
						const auto& edge_desc = add_edge(mew_vtx_idx, correspond_vtx_desc, new_graph);
						put(edge_name_map_new_graph, edge_desc.first, edge_name_map_b[*iterator]);
					}
				}
			}
		}

		{
			auto es = out_edges(vtx_desc, graph_b);

			for (auto iterator = es.first; iterator != es.second; iterator++)
			{
				size_t dst_vtx_desc = target(*iterator, graph_b);
				input_edges_b[dst_vtx_desc].push_back(vtx_desc);
			}
		}

	}

private:
	bool isCommonSubGraphVerticesB(Vertex vtx_desc)
	{
		return mcs_vertices_b.find(vtx_desc) != mcs_vertices_b.end();
	}

	VertexNameMap vtx_name_map_b;
	VertexNameMap vtx_name_map_new_graph;

	EdgeNameMap edge_name_map_b;
	EdgeNameMap edge_name_map_new_graph;

	CGraph& new_graph;
	CGraph& m_graph_b;

	// max common subgraph vertices of graph b 
	const std::set<size_t>& mcs_vertices_b;

	// max common subgraph map b to a
	const std::map<size_t, size_t>& mcs_vtx_desc_map_b2a;

	// only store start(input) vertex !!!! (avoid the repeated hash vertex)
	const std::map<XXH64_hash_t, size_t>& ipt_vtx_hash_to_vtx_desc_a;

	//start(input) vertex descriptors
	std::set<size_t> st_vtx_descs_b;

	//vertex propertry index map (from to new graph)
	std::map<size_t, size_t>map_idx_b2ng;

	using SInputEdges = std::vector<size_t>;
	std::map<size_t, SInputEdges> input_edges_b; // input edges of a vertex

	STopologicalOrderVetices topological_order_vertices;
};

CGraph CGlobalGraphsBuilder::mergeGraph(CGraph* graph_a, CGraph* graph_b)
{
	SMcsResult mcs_result;
	findCommonSubgraphMax(*graph_a, *graph_b, mcs_result);

	std::map<XXH64_hash_t, size_t> ipt_vtx_hash_to_vtx_desc_a;
	CStartVertexVisitor ipt_vtx_visitor_a(*graph_a, &ipt_vtx_hash_to_vtx_desc_a, nullptr);
	depth_first_search(*graph_a, visitor(ipt_vtx_visitor_a));



	CGraph new_graph = *graph_a;
	CGraphMerger graph_merger(new_graph, *graph_b, ipt_vtx_hash_to_vtx_desc_a, mcs_result.graph_common_vtx_indices[1], mcs_result.vtx_desc_map_b2a);
	graph_merger.mergeGraph();

	std::map<SGraphVertexDesc, std::vector<SGraphEdgeDesc>> vertex_input_edges;
	buildGraphVertexInputEdges(new_graph, vertex_input_edges);
	variableRename(&new_graph, vertex_input_edges);
	graphPartition(&new_graph, vertex_input_edges);

#if TANGRAM_DEBUG
	visGraph(&new_graph, true, false, false);
#endif
	//查找最大公共子图的时候记录graph1 的最大公共子图的所有index

	//以A图为基础，DFS遍历B图
	//对于B图中所遍历到的任何一个顶点
	//	并且AB图的下一个节点都是，如果下一个顶点是最大公共子图的节点之一
	//		不需要再次加入下一个节点
	//	如果下一个节点不是公共节点
	//		增加改节点，并将改节点的入边加入到图中
	//		遍历<所有>出边
	//			如果出边对应出顶点输入最大公共子图的节点之一
	//				增加一条边，连接这个顶点

	// removeRedundantNode
	// 

	

	return CGraph();
}

void initGlobalShaderGraphBuild()
{
	if (global_graph_builder == nullptr)
	{
		global_graph_builder = new CGlobalGraphsBuilder();
		global_ast_node_recursive_copy = new CGlobalAstNodeRecursiveCopy();
	}
}

void addHashedGraphToGlobalGraphBuilder(std::vector<CHashNode>& hash_dependency_graphs, int shader_id)
{
	if (global_graph_builder == nullptr)
	{
		assert(false && "global_graph_builder == nullptr");
	}

	global_graph_builder->addHashDependencyGraph(hash_dependency_graphs, shader_id);
}

CGlobalAstNodeRecursiveCopy* getGlobalAstNodeRecursiveCopy()
{
	return global_ast_node_recursive_copy;
}

void buildGlobalShaderGraph()
{
	global_graph_builder->mergeGraphs();
}

void releaseGlobalShaderGraph()
{
	if (global_graph_builder != nullptr)
	{
		delete global_graph_builder;
	}
}
