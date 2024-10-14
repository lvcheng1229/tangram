#include "shader_compressor.h"
#include "tangram.h"

void CShaderCompressor::partition()
{
	init_ast_to_glsl();
	
	std::vector<SGraphVertexDesc> start_vertex_descs;
	for (STopologicalOrderVetices::reverse_iterator vtx_desc_iter = topological_order_vertices.rbegin(); vtx_desc_iter != topological_order_vertices.rend(); ++vtx_desc_iter)
	{
		auto input_edges_iter = vertex_input_edges.find(*vtx_desc_iter);
		if (input_edges_iter == vertex_input_edges.end()) // linker node
		{
			start_vertex_descs.push_back(*vtx_desc_iter);
		}
	}

	for (int start_vertex_idx = 0; start_vertex_idx < start_vertex_descs.size(); start_vertex_idx++)
	{
		const SGraphVertexDesc& vtx_desc = start_vertex_descs[start_vertex_idx];
		if (node_found.find(vtx_desc) == node_found.end())
		{
			node_found.insert(vtx_desc);
			SShaderCodeVertex& shader_vertex = vtx_name_map[vtx_desc];
			shader_vertex.code_block_index = block_index;
			for (int same_hash_idx = 0; same_hash_idx < start_vertex_descs.size(); same_hash_idx++)
			{
				if (same_hash_idx != start_vertex_idx)
				{
					const SGraphVertexDesc& same_hash_vtx_desc = start_vertex_descs[same_hash_idx];
					SShaderCodeVertex& same_hash_shader_vertex = vtx_name_map[same_hash_vtx_desc];
					
					if (same_hash_shader_vertex.vertex_shader_ids_hash == shader_vertex.vertex_shader_ids_hash)
					{
						if (node_found.find(same_hash_vtx_desc) == node_found.end())
						{
							node_found.insert(same_hash_vtx_desc);
							same_hash_shader_vertex.interm_node->traverse(glsl_converter);
							same_hash_shader_vertex.code_block_index = block_index;
						}
					}
				}
			}
			block_index++;
		}
	}

	for (STopologicalOrderVetices::reverse_iterator vtx_desc_iter = topological_order_vertices.rbegin(); vtx_desc_iter != topological_order_vertices.rend(); ++vtx_desc_iter)
	{
		if (node_found.find(*vtx_desc_iter) == node_found.end())
		{
			node_found.insert(*vtx_desc_iter);
			SShaderCodeVertex& shader_vertex = vtx_name_map[*vtx_desc_iter];
			shader_vertex.code_block_index = block_index;
			shader_vertex.interm_node->traverse(glsl_converter);
			floodSearch(*vtx_desc_iter, shader_vertex.vertex_shader_ids_hash);
			block_index++;
		}
	}
}

void CShaderCompressor::floodSearch(SGraphVertexDesc vtx_desc, std::size_t pre_node_block_hash)
{
	auto input_edges_iter = vertex_input_edges.find(vtx_desc);
	if (input_edges_iter != vertex_input_edges.end())
	{
		const std::vector<SGraphEdgeDesc>& input_edges = input_edges_iter->second;
		for (const auto& input_edge_iter : input_edges)
		{
			const auto vertex_desc = source(input_edge_iter, graph);
			SShaderCodeVertex& next_shader_vertex = vtx_name_map[vertex_desc];
			if (next_shader_vertex.vertex_shader_ids_hash == pre_node_block_hash)
			{
				if (node_found.find(vertex_desc) == node_found.end())
				{
					node_found.insert(vertex_desc);
					next_shader_vertex.interm_node->traverse(glsl_converter);
					next_shader_vertex.code_block_index = block_index;
					floodSearch(vertex_desc, pre_node_block_hash);
				}
			}
		}
	}


	auto out_edges_iter = out_edges(vtx_desc, graph);
	for (auto edge_iterator = out_edges_iter.first; edge_iterator != out_edges_iter.second; edge_iterator++)
	{
		const auto vertex_desc = target(*edge_iterator, graph);
		SShaderCodeVertex& next_shader_vertex = vtx_name_map[vertex_desc];
		if (next_shader_vertex.vertex_shader_ids_hash == pre_node_block_hash)
		{
			next_shader_vertex.code_block_index = block_index;
			if (node_found.find(vertex_desc) == node_found.end())
			{
				node_found.insert(vertex_desc);
				next_shader_vertex.interm_node->traverse(glsl_converter);
				floodSearch(vertex_desc, pre_node_block_hash);
			}
		}
	}
}

void CGlobalGraphsBuilder::shaderGraphLevelCompress(CGraph* graph, std::map<SGraphVertexDesc, std::vector<SGraphEdgeDesc>>& vertex_input_edges)
{
	STopologicalOrderVetices topological_order_vertices;
	topological_sort(*graph, std::back_inserter(topological_order_vertices));
	VertexNameMap vtx_name_map = get(boost::vertex_name, *graph);

	for (STopologicalOrderVetices::reverse_iterator vtx_desc_iter = topological_order_vertices.rbegin(); vtx_desc_iter != topological_order_vertices.rend(); ++vtx_desc_iter)
	{
		SShaderCodeVertex& shader_code_vtx = vtx_name_map[*vtx_desc_iter];
		SShaderCodeVertexInfomation& info = shader_code_vtx.vtx_info;
		std::size_t seed = 42;
		for (int index = 0; index < info.related_shaders.size(); index++)
		{
			hash_combine(seed, info.related_shaders[index]);
		}
		shader_code_vtx.vertex_shader_ids_hash = seed;
	}

	CShaderCompressor graph_partitioner(vertex_input_edges, topological_order_vertices, *graph);
	graph_partitioner.partition();
}