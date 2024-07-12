#include <algorithm>
#include "shader_network.h"


static CShaderNetwork* global_shader_network = nullptr;

void CShaderNetwork::addAstHashTree(std::vector<CHashNode>& hash_tree)
{
	if (global_shader_network->getShaderVertexNum() == 0)
	{
		for ( auto& iter : hash_tree)
		{
			iter.idx_in_vtx_arr = addUniqueVertex(SShaderVertex{ iter.weight,1,iter.weight ,0xFFFFFFFF,0xFFFFFFFF ,0xFFFFFFFF ,iter.hash_value,iter.debug_string.c_str() });
		}

		for (const auto& iter : hash_tree)
		{
			uint32_t edge_idx = iter.idx_in_vtx_arr;
			for (const auto& ipt_id : iter.input_hash_nodes)
			{
				addUniqueEdge(edge_idx, ipt_id,true);
			}

			for (const auto& opt_id : iter.out_hash_nodes)
			{
				addUniqueEdge(edge_idx, opt_id, false);
			}
		}
	}
	else
	{
		std::vector<SUniqueIDArray> sub_unique_id_paths;
		std::vector<XXH64_hash_t> hash_nodes_found;
		std::vector<std::pair<float, uint32_t>> sorted_indices;
		sorted_indices.resize(hash_tree.size());
		for (int idx = 0; idx < hash_tree.size(); idx++)
		{
			sorted_indices[idx].first = hash_tree[idx].weight;
			sorted_indices[idx].second = idx;
		}
		std::sort(sorted_indices.begin(), sorted_indices.end());

		for (auto& iter : hash_tree)
		{
			auto unique_ids_iter = hash_to_unique_ids.find(iter.hash_value);
			if (unique_ids_iter != hash_to_unique_ids.end())
			{
				SUniqueIDArray& unique_ids = unique_ids_iter->second;
			}
			
				// fina a similar path
		}

	}
}

inline uint32_t CShaderNetwork::addUniqueVertex(const SShaderVertex& shader_vtx)
{
	global_unique_id++;
	uint32_t array_index = shader_vertices.size();
	shader_vertices.push_back(shader_vtx);
	
	uint32_t edge_idx = shader_vtx_edges.size();
	shader_vtx_edges.push_back(SShaderVertexEdges{});

	SShaderVertex& added_vertex = shader_vertices.back();
	added_vertex.idx_in_vtx_arr = array_index;
	added_vertex.unique_id = global_unique_id;
	added_vertex.vtx_edge_idx = edge_idx;

	unique_id_idx_map[global_unique_id] = array_index;
	return array_index;
}

inline void CShaderNetwork::addUniqueEdge(uint32_t edge_idx, uint32_t unique_id, bool is_add_ipt_edge)
{
	if (is_add_ipt_edge)
	{
		shader_vtx_edges[edge_idx].ipt_vtx_unique_ids.push_back(unique_id);
	}
	else
	{
		shader_vtx_edges[edge_idx].opt_vtx_unique_ids.push_back(unique_id);
	}
}

inline void CShaderNetwork::addVertex(const SShaderVertex& shader_vtx)
{
	if (unique_id_idx_map.find(shader_vtx.unique_id) == unique_id_idx_map.end())
	{
		shader_vertices.push_back(shader_vtx);
	}
}

inline void CShaderNetwork::addVertexEdge(uint32_t src_vtx_idx, uint32_t dst_vtx_idx)
{

}

void initShaderNetwork()
{
	if (global_shader_network)
	{
		global_shader_network = new CShaderNetwork();
	}
}

void addAstHashTree(std::vector<CHashNode>& hash_tree)
{
	global_shader_network->addAstHashTree(hash_tree);
}

void finalizeShaderNetwork()
{
	if (global_shader_network)
	{
		delete global_shader_network;
	}
}
