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


class CVariableRenameFloodFillSearch
{
public:
	CVariableRenameFloodFillSearch(
		std::map<SGraphVertexDesc, std::vector<SGraphEdgeDesc>>& vertex_input_edges,
		const std::string& ipt_symbol_name,
		int output_symbol_index,
		SGraphVertexDesc vtx_desc,
		CGraph& graph
	)
		:vertex_input_edges(vertex_input_edges)
		, symbol_name(ipt_symbol_name)
		, output_symbol_index(output_symbol_index)
		, vtx_desc(vtx_desc)
		, graph(graph)
	{
		vtx_name_map = get(boost::vertex_name, graph);
		edge_name_map = get(boost::edge_name, graph);
	}

	void renameVariable()
	{
		floodFillSearch(vtx_desc,-1, output_symbol_index);
	}

	void floodFillSearch(SGraphVertexDesc vtx_desc, int symbol_input_index = -1, int symbol_output_index = -1)
	{
		if (symbol_input_index != -1)
		{
			const std::vector<SGraphEdgeDesc>& input_edges = vertex_input_edges.find(vtx_desc)->second;
			for (const auto& input_edge_iter : input_edges)
			{
				const SShaderCodeEdge& input_edge = edge_name_map[input_edge_iter];
				
				const auto vertex_desc = source(input_edge_iter, graph);
				if (node_found.find(vertex_desc) == node_found.end())
				{
					node_found.insert(vertex_desc);
					SShaderCodeVertex& input_vertex = vtx_name_map[vertex_desc];

					auto pre_index_iter = input_edge.variable_map_next_to_pre.find(symbol_input_index);
					if (pre_index_iter != input_edge.variable_map_next_to_pre.end())
					{
						int input_node_symbol_index = pre_index_iter->second;
						input_vertex.vtx_info.opt_variable_names[input_node_symbol_index] = symbol_name;

						// 如果这个变量是输入变量也是输出变量
						const auto& out2in_index_iter = input_vertex.vtx_info.inout_variable_out2in.find(input_node_symbol_index);
						floodFillSearch(vertex_desc, (out2in_index_iter == input_vertex.vtx_info.inout_variable_out2in.end()) ? -1 : out2in_index_iter->second, input_node_symbol_index);
					}
				}
			}
		}

		if (symbol_output_index != -1)
		{
			auto out_edges_iter = out_edges(vtx_desc, graph);
			for (auto edge_iterator = out_edges_iter.first; edge_iterator != out_edges_iter.second; edge_iterator++)
			{
				const SShaderCodeEdge& output_edge = edge_name_map[*edge_iterator];
				const auto vertex_desc = target(*edge_iterator, graph);
				if (node_found.find(vertex_desc) == node_found.end())
				{
					node_found.insert(vertex_desc);
					SShaderCodeVertex& output_vertex = vtx_name_map[vertex_desc];

					auto next_index_iter = output_edge.variable_map_pre_to_next.find(symbol_output_index);
					if (next_index_iter != output_edge.variable_map_pre_to_next.end())
					{
						int output_node_symbol_index = next_index_iter->second;
						output_vertex.vtx_info.ipt_variable_names[output_node_symbol_index] = symbol_name;
						// 如果这个变量是输入变量也是输出变量
						const auto& in2out_index_iter = output_vertex.vtx_info.inout_variable_in2out.find(output_node_symbol_index);
						floodFillSearch(vertex_desc, output_node_symbol_index, (in2out_index_iter == output_vertex.vtx_info.inout_variable_in2out.end()) ? -1 : in2out_index_iter->second);
					}
				}
			}
		}
		
	}

private:
	const std::map<SGraphVertexDesc, std::vector<SGraphEdgeDesc>>& vertex_input_edges;
	const std::string symbol_name;
	int output_symbol_index;
	SGraphVertexDesc vtx_desc;
	CGraph& graph;

	VertexNameMap vtx_name_map;
	EdgeNameMap edge_name_map;

	std::set<SGraphVertexDesc> node_found;
};

void CGlobalGraphsBuilder::variableRename(CGraph* graph, std::map<SGraphVertexDesc, std::vector<SGraphEdgeDesc>>& vertex_input_edges)
{
	STopologicalOrderVetices topological_order_vertices;
	topological_sort(*graph, std::back_inserter(topological_order_vertices));
	VertexNameMap vtx_name_map = get(boost::vertex_name, *graph);

	for (STopologicalOrderVetices::reverse_iterator vtx_desc_iter = topological_order_vertices.rbegin(); vtx_desc_iter != topological_order_vertices.rend(); ++vtx_desc_iter)
	{
		SShaderCodeVertex& shader_code_vtx = vtx_name_map[*vtx_desc_iter];
		shader_code_vtx.vtx_info.ipt_variable_names.resize(shader_code_vtx.vtx_info.input_variable_num);
		shader_code_vtx.vtx_info.opt_variable_names.resize(shader_code_vtx.vtx_info.output_variable_num);
	}

	for (STopologicalOrderVetices::reverse_iterator vtx_desc_iter = topological_order_vertices.rbegin(); vtx_desc_iter != topological_order_vertices.rend(); ++vtx_desc_iter)
	{
		SShaderCodeVertex& shader_code_vtx = vtx_name_map[*vtx_desc_iter];
		SShaderCodeVertexInfomation& vtx_info = shader_code_vtx.vtx_info;
		for (int opt_symbol_idx = 0; opt_symbol_idx < shader_code_vtx.vtx_info.output_variable_num; opt_symbol_idx++)
		{
			if (shader_code_vtx.vtx_info.inout_variable_out2in.find(opt_symbol_idx) == shader_code_vtx.vtx_info.inout_variable_out2in.end())
			{
				if (shader_code_vtx.should_rename == true)
				{
					variable_name_manager.getNewSymbolName(XXH64_hash_t(0), vtx_info.opt_variable_names[opt_symbol_idx]);
				}
				else
				{
					assert(vtx_info.opt_variable_names.size() == 1);
					vtx_info.opt_variable_names[0] = shader_code_vtx.symbol_name;
				}
				
				CVariableRenameFloodFillSearch variable_rename_flood_search(vertex_input_edges, vtx_info.opt_variable_names[opt_symbol_idx], opt_symbol_idx, *vtx_desc_iter, *graph);
				variable_rename_flood_search.renameVariable();
			}
		}
	}
}