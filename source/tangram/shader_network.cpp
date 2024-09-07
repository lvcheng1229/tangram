#include <algorithm>
#include "shader_network.h"
#include "graphviz.h"

/*static CGlobalShaderNetwork* global_shader_network = nullptr;

void CGlobalShaderNetwork::addAstHashTree(std::vector<CHashNode>& hash_tree)
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
		std::unordered_map<uint32_t, SIteratedVtxInfor> vtx_ntwk_map;
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

		//	1.merge subnetwork
		//	using SMergedSubnerwork = std::vector<uint32_t>;
		//	std::vector<SMergedSubnerwork> mg_sub_ntwk_indices;
		//	std::vector<int> sub_ntwk_visit_idx;
		//	sub_ntwk_visit_idx.resize(sub_networks.size());
		//	for (int sub_ntwk_idx = 0; sub_ntwk_idx < sub_networks.size(); sub_ntwk_idx++)
		//	{
		//		if (sub_ntwk_visit_idx[sub_ntwk_idx] != 1)
		//		{
		//	
		//			sub_ntwk_visit_idx[sub_ntwk_idx] = 1;
		//		}
		//	}

		// 2.generate subnetwork group by link the subnetwork in a same network
		std::vector<SSubNetwork> merged_sub_ntwk;
		for (int sub_ntwk_idx = 0; sub_ntwk_idx < sub_networks.size(); sub_ntwk_idx++)
		{
			if (sub_networks[sub_ntwk_idx].ipt_sub_networks.size() == 0)
			{
				merged_sub_ntwk.push_back(sub_networks[sub_ntwk_idx]);
			}
		}

		for (int sub_ntwk_idx = 0; sub_ntwk_idx < merged_sub_ntwk.size(); sub_ntwk_idx++)
		{
			std::map<int32_t, int32_t> map_ntwk2edge_offset;
			SSubNetwork& merged_subntwk = merged_sub_ntwk[sub_ntwk_idx];
			if (merged_subntwk.opt_sub_networks.size() != 0)
			{
				for (auto opt_ntwk_iter : merged_subntwk.opt_sub_networks)
				{
					if (map_ntwk2edge_offset.find(opt_ntwk_iter) == map_ntwk2edge_offset.end())
					{
						map_ntwk2edge_offset[opt_ntwk_iter] = merged_subntwk.sub_edges.size();
						const SSubNetwork& other_sub_ntwk = sub_networks[opt_ntwk_iter];
						for (int vtx_idx = 0; vtx_idx < other_sub_ntwk.sub_vetices.size(); vtx_idx++)
						{
							merged_subntwk.sub_vetices.emplace_back(other_sub_ntwk.sub_vetices[vtx_idx]);
						}
						for (int edge_idx = 0; edge_idx < other_sub_ntwk.sub_edges.size(); edge_idx++)
						{
							merged_subntwk.sub_edges.emplace_back(other_sub_ntwk.sub_edges[edge_idx]);
						}
						mergeSubnetworkGroup(sub_networks, merged_sub_ntwk[sub_ntwk_idx], other_sub_ntwk, map_ntwk2edge_offset);
					}
				}
			}
			
			// todo: remove map ntwk to edge offset: map -> set
			generateSubnetworkGroup(merged_sub_ntwk[sub_ntwk_idx]);
		}

		// 3.find the best subnetwork group
		// todo:

		// 4.if ratio over 50%
		// todo:

		// 5.add the vertices into the global network (add the new vertex that is not in the subnet work group)
		//for (auto& iter : hash_tree)
		//{
		//	iter.idx_in_vtx_arr = addUniqueVertex(SShaderVertex{ iter.weight,1,iter.weight ,0xFFFFFFFF,0xFFFFFFFF ,0xFFFFFFFF ,iter.hash_value,iter.debug_string.c_str() });
		//}


		// 首先要确定加入哪个图
		//	1.根据每个节点的权重排序
		//	2.从权重最高的节点开始遍历
		//		根据HashValue，查找该HashValue的所有UniqueIDs
		//		对所有UniqueIDs的图，判断两张图的相似性
		// 
		// 
		// 然后合并两个图
		//	从根小图节点（input node）开始DFS遍历（正向遍历！！）
		//	如果该小图中的节点在大图中不存在，则新加一个节点，并新加一条（出边），连接到新加的节点
		//  如果该小图中的节点在大图中存在，判断该节点的（出边）是否存在（即该节点的前一个节点是否是本次迭代中新加的），不存在，则添加一条边

		//
		//if (aa)
		//{
		//	case a -> b - a ->c
		//	different hash node in the graph has the same hash value
		//}
		//else
		{
			// just merge them together
		}

		//生成变量名，参考此篇算法
		//https://www.zhihu.com/question/32098665/answer/54625344

#if TANGRAM_VIS_GRAPH
		CGraphVis graph_vis;
		std::vector<int32_t> vtx_node_idx;
		vtx_node_idx.resize(shader_vertices.size());
		for (int idx = 0; idx < shader_vertices.size(); idx++)
		{
			SShaderVertex& shader_vtx = shader_vertices[idx];
			vtx_node_idx[idx] = graph_vis.addNode(SNodeDesc{ shader_vtx.debug_string });
		}

		for (int edge_idx = 0; edge_idx < shader_vtx_edges.size(); edge_idx++)
		{
			std::vector<uint32_t>& opt_vtx_uids = shader_vtx_edges[edge_idx].opt_vtx_unique_ids;
			for (int opt_vtx = 0; opt_vtx < opt_vtx_uids.size(); opt_vtx++)
			{
				int32_t from_vtx_idx = edge_idx;
				int32_t to_vtx_idx = unique_id_idx_map[opt_vtx_uids[opt_vtx]];

				graph_vis.addEdge(vtx_node_idx[from_vtx_idx], vtx_node_idx[to_vtx_idx]);
			}
		}

		graph_vis.generate();
		
#endif
	}
}


void CGlobalShaderNetwork::mergeSubnetworkGroup(const std::vector<SSubNetwork>& sub_ntwks, SSubNetwork& merged_subntwk,const SSubNetwork& subntwk_to_process, std::map<int32_t, int32_t>& map_ntwk2edge_offset)
{
	if (subntwk_to_process.opt_sub_networks.size() != 0)
	{
		for (auto opt_ntwk_iter : subntwk_to_process.opt_sub_networks)
		{
			if (map_ntwk2edge_offset.find(opt_ntwk_iter) == map_ntwk2edge_offset.end())
			{
				map_ntwk2edge_offset[opt_ntwk_iter] = merged_subntwk.sub_edges.size();
				const SSubNetwork& other_sub_ntwk = sub_ntwks[opt_ntwk_iter];
				for (int vtx_idx = 0; vtx_idx < other_sub_ntwk.sub_vetices.size(); vtx_idx++)
				{
					merged_subntwk.sub_vetices.emplace_back(other_sub_ntwk.sub_vetices[vtx_idx]);
				}
				for (int edge_idx = 0; edge_idx < other_sub_ntwk.sub_edges.size(); edge_idx++)
				{
					merged_subntwk.sub_edges.emplace_back(other_sub_ntwk.sub_edges[edge_idx]);
				}
			}
		}
	}
}

// @param: map_vtx2offset map vertex to 
void CGlobalShaderNetwork::generateSubnetworkGroup(SSubNetwork& subntwk_to_process)
{
	if (subntwk_to_process.opt_sub_networks.size() != 0)
	{
		for (int vtx_idx = 0; vtx_idx < subntwk_to_process.sub_vetices.size(); vtx_idx++)
		{
			SSubNetwork::SSubNetworkVertex& vtx = subntwk_to_process.sub_vetices[vtx_idx];
			if (vtx.linked_sub_network != -1)
			{
				assert(vtx.sub_edge_idx == INVALIDA_EDGE_DIX);
				vtx.sub_edge_idx = subntwk_to_process.sub_edges.size();
				subntwk_to_process.sub_edges.push_back(SSubNetwork::SSubNetworkVtexIptEdge());
				subntwk_to_process.sub_edges[vtx.sub_edge_idx].ipt_vtx_unique_ids.push_back(int32_t(vtx.linked_other_vtx_uid));
			}
		}
	}
}

inline uint32_t CGlobalShaderNetwork::addUniqueVertex(const SShaderVertex& shader_vtx)
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

inline void CGlobalShaderNetwork::addUniqueEdge(uint32_t edge_idx, uint32_t unique_id, bool is_add_ipt_edge)
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

inline void CGlobalShaderNetwork::addVertex(const SShaderVertex& shader_vtx)
{
	//hash_to_unique_ids assert_t(false);
	if (unique_id_idx_map.find(shader_vtx.unique_id) == unique_id_idx_map.end())
	{
		shader_vertices.push_back(shader_vtx);
	}
}

inline void CGlobalShaderNetwork::addVertexEdge(uint32_t src_vtx_idx, uint32_t dst_vtx_idx)
{

}

void CGlobalShaderNetwork::addHashValueMap(XXH64_hash_t hash_value, uint32_t unique_id)
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
* @param	vtx_ntwk_map: map shader vetices array index to 
*/
//LinkType CGlobalShaderNetwork::subNetworkIterInputNode(const std::vector<CHashNode>& hash_tree, const CHashNode& ipt_node, const uint32_t ipt_vtx_unique_id, SSubNetwork& sub_network, std::unordered_map<uint32_t, SIteratedVtxInfor>& vtx_ntwk_map, int cur_sub_ntwk_idx, std::vector<SSubNetwork>& sub_ntwks)
//{
//	const uint32_t input_vtx_idx = unique_id_idx_map.find(ipt_vtx_unique_id)->second;
//	const SShaderVertex& input_shader_vtx = shader_vertices[input_vtx_idx];
//
//	// if the node is already iterated, link current vertex to the exsited sub_network
//	const auto& vtx_ntwk_iter = vtx_ntwk_map.find(input_vtx_idx);
//	if ((input_shader_vtx.hash_value == ipt_node.hash_value) /*todo: fix me, different hash node may has the same hash value*/ && (ipt_node.input_hash_nodes.size() > 0 /*non linker object*/) && (vtx_ntwk_iter != vtx_ntwk_map.end()) && (vtx_ntwk_iter->second.other_ntwk_idx != cur_sub_ntwk_idx)/**/)
//	{
//		int other_sub_ntwk = vtx_ntwk_iter->second.other_ntwk_idx;
//		sub_ntwks[other_sub_ntwk].ipt_sub_networks.insert(cur_sub_ntwk_idx);
//		sub_network.opt_sub_networks.insert(other_sub_ntwk);
//		SSubNetwork::SSubNetworkVertex ipt_network_vtx = SSubNetwork::SSubNetworkVertex{ input_vtx_idx ,INVALIDA_EDGE_DIX ,other_sub_ntwk, /*vtx_ntwk_iter->second.other_ntwk_vtx_array_idx,*/ int32_t(ipt_vtx_unique_id)};
//		sub_network.sub_vetices.push_back(ipt_network_vtx);
//		return ELinkType::LT_LINKER_SUBNETWORK;
//	}
//
//	if (input_shader_vtx.hash_value == ipt_node.hash_value && ipt_node.input_hash_nodes.size() > 0 /*non linker object*/) //todo: fix me
//	{
//		// iterate the input shader edges of the shader vertex in the shader network
//		const SShaderVertexEdges& ipt_shader_edges = shader_vtx_edges[input_shader_vtx.vtx_edge_idx];
//
//		// todo: output edges
//		SSubNetwork::SSubNetworkVtexIptEdge valid_shader_edges;
//		ELinkType edge_link_types = ELinkType::LT_NONE;
//		for (int next_ipt_edge_idx = 0; next_ipt_edge_idx < ipt_shader_edges.ipt_vtx_unique_ids.size(); next_ipt_edge_idx++)
//		{
//			const uint32_t ipt_vtx_unique_id = ipt_shader_edges.ipt_vtx_unique_ids[next_ipt_edge_idx];
//			for (int ipt_hash_node_idx = 0; ipt_hash_node_idx < ipt_node.input_hash_nodes.size(); ipt_hash_node_idx++)
//			{
//				uint32_t ipt_hash_node_mapped_idx = ipt_node.input_hash_nodes[ipt_hash_node_idx];
//				ELinkType link_type = subNetworkIterInputNode(hash_tree, hash_tree[ipt_hash_node_mapped_idx], ipt_vtx_unique_id,  sub_network, vtx_ntwk_map, cur_sub_ntwk_idx, sub_ntwks);
//				edge_link_types = ELinkType(edge_link_types | link_type);
//				//if (link_type & LT_VILID_LINK)
//				if (link_type & LT_NON_LINKER_NODE)
//				{
//					valid_shader_edges.ipt_vtx_unique_ids.push_back(ipt_vtx_unique_id);
//				}
//			}
//		}
//
//		if (valid_shader_edges.ipt_vtx_unique_ids.size() > 0) // Current node is linked with the non-linker node
//		{
//			SSubNetwork::SSubNetworkVertex ipt_network_vtx = SSubNetwork::SSubNetworkVertex{ input_vtx_idx ,static_cast<uint32_t>(sub_network.sub_edges.size()) };
//			sub_network.sub_edges.push_back(valid_shader_edges);
//			sub_network.sub_vetices.push_back(ipt_network_vtx);
//			vtx_ntwk_map.insert(std::pair<uint32_t, SIteratedVtxInfor>(input_vtx_idx, SIteratedVtxInfor{ cur_sub_ntwk_idx/*, int32_t(sub_network.sub_vetices.size() - 1)*/ }));
//		}
//		else  if(edge_link_types & LT_VILID_LINK)// Current node is linked with the linker node
//		{
//			SSubNetwork::SSubNetworkVertex ipt_network_vtx = SSubNetwork::SSubNetworkVertex{ input_vtx_idx ,INVALIDA_EDGE_DIX };
//			sub_network.sub_vetices.push_back(ipt_network_vtx);
//			vtx_ntwk_map.insert(std::pair<uint32_t, SIteratedVtxInfor>(input_vtx_idx, SIteratedVtxInfor{ cur_sub_ntwk_idx/*, int32_t(sub_network.sub_vetices.size() - 1)*/}));
//		}
//		return ELinkType::LT_NON_LINKER_NODE;
//	}
//	else if(input_shader_vtx.hash_value == ipt_node.hash_value && ipt_node.input_hash_nodes.size() == 0 ) // This node is a linker node
//	{
//		// Don't consider linker nodes for graph ? Remove these codes?
//		//SSubNetwork::SSubNetworkVertex ipt_network_vtx = SSubNetwork::SSubNetworkVertex{ input_vtx_idx ,INVALIDA_EDGE_DIX };
//		//sub_network.sub_vetices.push_back(ipt_network_vtx);
//		//vtx_ntwk_map.insert(std::pair<uint32_t, uint32_t>(input_vtx_idx, cur_sub_ntwk_idx));
//		return ELinkType::LT_LINKER_NODE;
//	}
//	return ELinkType::LT_INVILID_LINK;
//}


//void initShaderNetwork()
//{
//	//if (!global_shader_network)
//	//{
//	//	global_shader_network = new CGlobalShaderNetwork();
//	//}
//}
//
//void addAstHashTree(std::vector<CHashNode>& hash_tree)
//{
//	//global_shader_network->addAstHashTree(hash_tree);
//}
//
//void finalizeShaderNetwork()
//{
//	//if (global_shader_network)
//	//{
//	//	delete global_shader_network;
//	//}
//}
