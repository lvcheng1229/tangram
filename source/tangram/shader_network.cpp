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
				const CHashNode& hash_node = hash_tree[ipt_id];
				const uint32_t input_node_idx = hash_node.idx_in_vtx_arr;
				const SShaderVertex& shader_vtex = shader_vertices[input_node_idx];
				addUniqueEdge(edge_idx, shader_vtex.unique_id, true);
			}

			for (const auto& opt_id : iter.out_hash_nodes)
			{
				const CHashNode& hash_node = hash_tree[opt_id];
				const uint32_t output_node_idx = hash_node.idx_in_vtx_arr;;
				const SShaderVertex& shader_vtex = shader_vertices[output_node_idx];
				addUniqueEdge(edge_idx, shader_vtex.unique_id, false);
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
		std::sort(sorted_indices.rbegin(), sorted_indices.rend());

		// temp codey
		{
			int idx = 0;

			const uint32_t node_idx = sorted_indices[idx].second;
			CHashNode& hash_node = hash_tree[node_idx];

			auto unique_ids_iter = hash_to_unique_ids.find(hash_node.hash_value);
			assert_t(unique_ids_iter != hash_to_unique_ids.end());// temp assert
			SUniqueIDArray& unique_ids = unique_ids_iter->second;
			assert_t(unique_ids.size() == 1);
		}

		// todo: merge these structures together
		std::vector<SSubNetwork> sub_networks;
		std::unordered_map<uint32_t,uint32_t> vtx_ntwk_map;
		int cur_sub_ntwk_idx = 0;

		for (int idx = 0; idx < sorted_indices.size(); idx++)
		{
			const uint32_t node_idx = sorted_indices[idx].second;
			CHashNode& hash_node = hash_tree[node_idx];

			// if the node is not the non-linker node
			if (hash_node.input_hash_nodes.size() > 0)
			{
				auto unique_ids_iter = hash_to_unique_ids.find(hash_node.hash_value);
				assert_t(unique_ids_iter != hash_to_unique_ids.end());// temp assert
				SUniqueIDArray& unique_ids = unique_ids_iter->second;
				assert_t(unique_ids.size() == 1);
				for (auto& iter : unique_ids) //todo: search the node with the higher weight
				{
					const uint32_t input_vtx_idx = unique_id_idx_map.find(iter)->second;
					// if the node has not been visited
					if (vtx_ntwk_map.find(input_vtx_idx) == vtx_ntwk_map.end())
					{
						SSubNetwork sub_network;
						subNetworkIterInputNode(hash_tree, hash_node, iter, sub_network, vtx_ntwk_map, cur_sub_ntwk_idx, sub_networks);
						if (sub_network.sub_vetices.size() > 0)
						{
							sub_networks.push_back(sub_network);
							cur_sub_ntwk_idx++;

							continue;
							assert_t(unique_ids.size() == 1); //todo:  fixme remove continue
						}
					}
				}
			}
		}

		// 1.merge subnetwork
		using SMergedSubnerwork = std::vector<uint32_t>;
		std::vector<SMergedSubnerwork> mg_sub_ntwk_indices;
		for (int sub_ntwk_idx = 0; sub_ntwk_idx < sub_networks.size(); sub_ntwk_idx++)
		{

		}

		// 2.generate subnetwork group by link the subnetwork in a same network
		
		// 3.find the best subnetwork group
		
		// 4.if ratio over 50%
		
		// 5.add the vertices into the global network (add the new vertex that is not in the subnet work group)
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
	addHashValueMap(added_vertex.hash_value, global_unique_id);
	return array_index;
}

inline void CShaderNetwork::addUniqueEdge(uint32_t edge_idx, uint32_t unique_id, bool is_add_ipt_edge)
{
	//hash_to_unique_ids assert_t(false);
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
	//hash_to_unique_ids assert_t(false);
	if (unique_id_idx_map.find(shader_vtx.unique_id) == unique_id_idx_map.end())
	{
		shader_vertices.push_back(shader_vtx);
	}
}

inline void CShaderNetwork::addVertexEdge(uint32_t src_vtx_idx, uint32_t dst_vtx_idx)
{

}

void CShaderNetwork::addHashValueMap(XXH64_hash_t hash_value, uint32_t unique_id)
{
	const auto& map_iter = hash_to_unique_ids.find(hash_value);
	if (map_iter != hash_to_unique_ids.end())
	{
		SUniqueIDArray& unque_ids = map_iter->second;
		const auto id_iter = std::find(unque_ids.begin(), unque_ids.end(), unique_id);
		if (id_iter == unque_ids.end())
		{
			unque_ids.push_back(unique_id);
		}
	}
	else
	{
		SUniqueIDArray unque_ids;
		unque_ids.push_back(unique_id);
		hash_to_unique_ids[hash_value] = unque_ids;
	}
}

/**
* @brief Subnetwork iteration. given a hash tree node
* @param	hash_tree & ipt_node
*/
ELinkType CShaderNetwork::subNetworkIterInputNode(const std::vector<CHashNode>& hash_tree, const CHashNode& ipt_node, const uint32_t ipt_vtx_unique_id, SSubNetwork& sub_network, std::unordered_map<uint32_t, uint32_t>& vtx_ntwk_map, int cur_sub_ntwk_idx, std::vector<SSubNetwork>& sub_ntwks)
{
	const uint32_t input_vtx_idx = unique_id_idx_map.find(ipt_vtx_unique_id)->second;
	const SShaderVertex& input_shader_vtx = shader_vertices[input_vtx_idx];

	// if the node is already iterated, link current vertex to the exsited sub_network
	const auto& vtx_ntwk_iter = vtx_ntwk_map.find(input_vtx_idx);
	if ((input_shader_vtx.hash_value == ipt_node.hash_value) && (ipt_node.input_hash_nodes.size() > 0 /*non linker object*/) && (vtx_ntwk_iter != vtx_ntwk_map.end()) && (vtx_ntwk_iter->second != cur_sub_ntwk_idx))
	{
		int other_sub_ntwk = vtx_ntwk_iter->second;
		sub_ntwks[other_sub_ntwk].opt_sub_networks.push_back(cur_sub_ntwk_idx);
		sub_network.opt_sub_networks.push_back(other_sub_ntwk);
		SSubNetwork::SSubNetworkVertex ipt_network_vtx = SSubNetwork::SSubNetworkVertex{ input_vtx_idx ,INVALIDA_EDGE_DIX ,other_sub_ntwk };
		sub_network.sub_vetices.push_back(ipt_network_vtx);
		return ELinkType::LT_LINKER_SUBNETWORK;
	}

	if (input_shader_vtx.hash_value == ipt_node.hash_value && ipt_node.input_hash_nodes.size() > 0 /*non linker object*/) //todo: fix me
	{
		// iterate the input shader edges of the shader vertex in the shader network
		const SShaderVertexEdges& ipt_shader_edges = shader_vtx_edges[input_shader_vtx.vtx_edge_idx];

		// todo: output edges
		SShaderVertexEdges valid_shader_edges;
		ELinkType edge_link_types = ELinkType::LT_NONE;
		for (int next_ipt_edge_idx = 0; next_ipt_edge_idx < ipt_shader_edges.ipt_vtx_unique_ids.size(); next_ipt_edge_idx++)
		{
			const uint32_t ipt_vtx_unique_id = ipt_shader_edges.ipt_vtx_unique_ids[next_ipt_edge_idx];
			for (int ipt_hash_node_idx = 0; ipt_hash_node_idx < ipt_node.input_hash_nodes.size(); ipt_hash_node_idx++)
			{
				uint32_t ipt_hash_node_mapped_idx = ipt_node.input_hash_nodes[ipt_hash_node_idx];
				ELinkType link_type = subNetworkIterInputNode(hash_tree, hash_tree[ipt_hash_node_mapped_idx], ipt_vtx_unique_id,  sub_network, vtx_ntwk_map, cur_sub_ntwk_idx, sub_ntwks);
				edge_link_types = ELinkType(edge_link_types | link_type);
				//if (link_type & LT_VILID_LINK)
				if (link_type & LT_NON_LINKER_NODE)
				{
					valid_shader_edges.ipt_vtx_unique_ids.push_back(ipt_vtx_unique_id);
				}
			}
		}

		if (valid_shader_edges.ipt_vtx_unique_ids.size() > 0) // Current node is linked with the non-linker node
		{
			SSubNetwork::SSubNetworkVertex ipt_network_vtx = SSubNetwork::SSubNetworkVertex{ input_vtx_idx ,static_cast<uint32_t>(sub_network.sub_edges.size()) };
			sub_network.sub_edges.push_back(valid_shader_edges);
			sub_network.sub_vetices.push_back(ipt_network_vtx);
			vtx_ntwk_map.insert(std::pair<uint32_t, uint32_t>(input_vtx_idx, cur_sub_ntwk_idx));
		}
		else  if(edge_link_types & LT_VILID_LINK)// Current node is linked with the linker node
		{
			SSubNetwork::SSubNetworkVertex ipt_network_vtx = SSubNetwork::SSubNetworkVertex{ input_vtx_idx ,INVALIDA_EDGE_DIX };
			sub_network.sub_vetices.push_back(ipt_network_vtx);
			vtx_ntwk_map.insert(std::pair<uint32_t, uint32_t>(input_vtx_idx, cur_sub_ntwk_idx));
		}
		return ELinkType::LT_NON_LINKER_NODE;
	}
	else if(input_shader_vtx.hash_value == ipt_node.hash_value && ipt_node.input_hash_nodes.size() == 0 ) // This node is a linker node
	{
		// Don't consider linker nodes for graph ? Remove these codes?
		//SSubNetwork::SSubNetworkVertex ipt_network_vtx = SSubNetwork::SSubNetworkVertex{ input_vtx_idx ,INVALIDA_EDGE_DIX };
		//sub_network.sub_vetices.push_back(ipt_network_vtx);
		//vtx_ntwk_map.insert(std::pair<uint32_t, uint32_t>(input_vtx_idx, cur_sub_ntwk_idx));
		return ELinkType::LT_LINKER_NODE;
	}
	return ELinkType::LT_INVILID_LINK;
}

void initShaderNetwork()
{
	if (!global_shader_network)
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
