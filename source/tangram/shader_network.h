#pragma once
//#include <vector>
//#include <string>
//#include <unordered_map>
//
//#include "xxhash.h"
//#include "common.h"
//#include "ast_hash_tree.h"
//
//#define TANGRAM_VIS_GRAPH (TANGRAM_DEBUG && 1)
//#define INVALIDA_EDGE_DIX (0xFFFFFFFF)
//
//struct SShaderVertex
//{
//	float total_weight; // total weight = vertex count * vertex weight
//	float vertex_count; // 
//	float vertex_weight;
//
//	uint32_t unique_id; // Unique vertex ID. Different vertices may have the same hash value. They are distinguished by the unique_id
//	
//	uint32_t idx_in_vtx_arr;
//	uint32_t vtx_edge_idx;
//
//	XXH64_hash_t hash_value;
//
//#if TANGRAM_DEBUG
//	std::string debug_string;
//#endif
//};
//
//// The input edges of the vertex
//// The output edeges of the vertex
//struct SShaderVertexEdges
//{
//	std::vector<uint32_t> ipt_vtx_unique_ids;
//	std::vector<uint32_t> opt_vtx_unique_ids;
//};
//
//struct SSubNetwork
//{
//	struct SSubNetworkVtexIptEdge
//	{
//		std::vector<uint32_t> ipt_vtx_unique_ids;
//	};
//
//	struct SSubNetworkVertex
//	{
//		uint32_t global_vtx_idx;
//		uint32_t sub_edge_idx = INVALIDA_EDGE_DIX;
//
//		int32_t linked_sub_network = -1; // link information
//		int32_t linked_other_vtx_uid = -1;
//	};
//	std::vector<SSubNetworkVertex> sub_vetices;
//	std::vector<SSubNetworkVtexIptEdge> sub_edges;
//	std::set<uint32_t> ipt_sub_networks; // link information
//	std::set<uint32_t> opt_sub_networks; // link information
//	//int sub_ntwk_idx = -1; // link information
//};
//
//enum ELinkType
//{
//	LT_NONE = 0,
//	LT_LINKER_NODE = 1,
//	LT_NON_LINKER_NODE = 1 << 1,
//	LT_LINKER_SUBNETWORK = 1 << 2,
//	LT_INVILID_LINK = 1 << 3,
//	LT_VILID_LINK = (LT_LINKER_NODE | LT_NON_LINKER_NODE | LT_LINKER_SUBNETWORK),
//};
//
//// 层级
//// SSubNetwork 组成了 CShaderNetwork
//// CShaderNetwork 组成了 CGlobalShaderNetwork
//
//class CShadderNetwork
//{
//
//};
//
//class CGlobalShaderNetwork
//{
//public:
//	void addAstHashTree(std::vector<CHashNode>& hash_tree);
//	inline int getShaderVertexNum()const { return shader_vertices.size(); };
//
//	inline uint32_t addUniqueVertex(const SShaderVertex& shader_vtx);
//	inline void addUniqueEdge(uint32_t edge_idx, uint32_t unique_id, bool is_add_ipt_edge);
//
//	inline void addVertex(const SShaderVertex& shader_vtx);
//	inline void addVertexEdge(uint32_t src_vtx_idx, uint32_t dst_vtx_idx);
//
//private:
//
//	struct SIteratedVtxInfor
//	{
//		int32_t other_ntwk_idx;
//		//int32_t other_ntwk_vtx_array_idx;
//	};
//
//	void addHashValueMap(XXH64_hash_t hash_value, uint32_t unique_id);
//
//	void mergeSubnetworkGroup(const std::vector<SSubNetwork>& sub_ntwks, SSubNetwork& merged_subntwk, const SSubNetwork& subntwk_to_process, std::map<int32_t, int32_t>& map_ntwk2edge_offset);
//	void generateSubnetworkGroup(SSubNetwork& subntwk_to_process);
//	
//	
//	ELinkType subNetworkIterInputNode(const std::vector<CHashNode>& hash_tree, const CHashNode& ipt_node, const uint32_t ipt_vtx_unique_id, SSubNetwork& sub_network, std::unordered_map<uint32_t, SIteratedVtxInfor>& vtx_ntwk_map, int cur_sub_ntwk_idx, std::vector<SSubNetwork>& sub_ntwks);
//
//	uint32_t global_unique_id;
//
//	using SUniqueIDArray = std::vector<uint32_t>;
//	std::unordered_map<XXH64_hash_t, SUniqueIDArray> hash_to_unique_ids; // map hash value to unique id array
//
//	std::unordered_map<uint32_t, uint32_t>unique_id_idx_map; // map unique id to shader_vertices index
//	
//	// shader_vertices and shader_vtx_edges are the same in size
//	std::vector<SShaderVertex>shader_vertices;
//	std::vector<SShaderVertexEdges> shader_vtx_edges;
//};
//
//void initShaderNetwork();
//void addAstHashTree(std::vector<CHashNode>& hash_tree);
//void finalizeShaderNetwork();

