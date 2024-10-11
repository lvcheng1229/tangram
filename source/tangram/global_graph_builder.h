#pragma once
#include <vector>
#include <string>
#include <unordered_map>

#include "xxhash.h"
#include "common.h"
#include "ast_hash_tree.h"
#include "variable_name_manager.h"
#include "ast_node_recursive_copy.h"

#include <boost/graph/adjacency_list.hpp>

//#include <boost/lexical_cast.hpp>
//#include <boost/graph/filtered_graph.hpp>
//#include <boost/graph/graph_utility.hpp>
//#include <boost/graph/iteration_macros.hpp>
//#include <boost/graph/mcgregor_common_subgraphs.hpp>
//#include <boost/property_map/shared_array_property_map.hpp>

struct SShaderCodeVertexInfomation
{
	// variable 1 name, variable 2 name. etc. (in order)
	int input_variable_num; //用于ipt_variable_names的resize
	int output_variable_num;//用于遍历所有输出变量

	std::vector<std::string> ipt_variable_names; // input variables
	std::vector<std::string> opt_variable_names; // output variables

	// 如果第 m 个输出变量是输入输出变量，将该变量映射到n，n是第几个输入变量
	std::map<int, int> inout_variable_in2out;

	// 和上面反过来
	std::map<int, int> inout_variable_out2in;

	// shader vertex sequence

	// 该顶点所有相关联的shader ID
	std::vector<int> related_shaders;
};

template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct SShaderCodeVertex
{
	XXH64_hash_t hash_value;
	SShaderCodeVertexInfomation vtx_info;
	
	// for graph partition
	std::size_t vertex_shader_ids_hash;
	int code_block_index;

#if TANGRAM_DEBUG
	std::string debug_string;
#endif

	inline bool operator==(const SShaderCodeVertex& other) 
	{ 
		return hash_value == other.hash_value; 
	}
};

struct SShaderCodeEdge
{
	// map next shader vertex's input varible index to previous vertex's output variable index
	std::map<uint32_t, uint32_t> variable_map_next_to_pre;

	// 
	std::map<uint32_t, uint32_t> variable_map_pre_to_next;
};

// Use boost::listS to store the vertices since execute 'remove vertices' causing 'Iterator and Descriptor Stability/Invalidation' https://www.boost.org/doc/libs/1_63_0/libs/graph/doc/adjacency_list.html
// It's has higer per-vertex space overhead https://www.boost.org/doc/libs/1_63_0/libs/graph/doc/using_adjacency_list.html#sec:choosing-graph-type

// The vertex_descriptor is only an index when you are using a vector(or similar) as the underlying data structure for your vertices(i.e.boost::vecS).If you use a different underlying data structure, 
// then the vertex descriptors will not necessarily be an index.An example of this would be if you use an std::list / boost::listS - lists do not use an index - based accessing method.Instead, each vertex_descriptor will instead be a pointer to the list item.

typedef boost::adjacency_list<boost::listS, boost::vecS, boost::directedS,
	boost::property< boost::vertex_name_t, SShaderCodeVertex,
	boost::property< boost::vertex_index_t, unsigned int > >,
	boost::property< boost::edge_name_t, SShaderCodeEdge> >
	CGraph;

typedef boost::property_map< CGraph, boost::vertex_name_t >::type VertexNameMap;
typedef boost::property_map< CGraph, boost::vertex_index_t >::type VertexIndexMap;
typedef std::vector< boost::graph_traits<CGraph>::vertex_descriptor > STopologicalOrderVetices;
typedef boost::property_map< CGraph, boost::edge_name_t >::type EdgeNameMap;

typedef boost::graph_traits<CGraph>::vertex_descriptor SGraphVertexDesc;
typedef boost::graph_traits<CGraph>::edge_descriptor SGraphEdgeDesc;

void buildGraphVertexInputEdges(CGraph& graph, std::map<SGraphVertexDesc, std::vector<SGraphEdgeDesc>>& vertex_input_edges);

struct SMcsResult
{
	std::set<size_t> graph_common_vtx_indices[2];
	std::map<size_t,size_t> vtx_desc_map_b2a;
	size_t subgraph_size;
};

class CGlobalGraphsBuilder
{
public:
	void addHashDependencyGraph(std::vector<CHashNode>& hash_dependency_graphs, int shader_id);
#if TANGRAM_DEBUG
	void visGraph(CGraph* graph, bool visualize_symbol_name = false, bool visualize_shader_id = false, bool visualize_graph_partition = false);
#endif
	void mergeGraphs();

private:
	void generateSubgraphRemoveMcs(CGraph& origin_graph, const std::set<size_t>& mcs_vtx_indices, const std::map<size_t, size_t>& pre_map_new_to_origin, CGraph& generated_subgraph, std::map<size_t, size_t>& map_new_to_origin);
	void findCommonSubgraphMax(CGraph& grapha, CGraph& graphb, SMcsResult& mcsResult);

	std::vector<CGraph> unmerged_graphs;

	CGraph merged_graphs;

	void variableRename(CGraph* graph, std::map<SGraphVertexDesc, std::vector<SGraphEdgeDesc>>& vertex_input_edges);
	void graphPartition(CGraph* graph, std::map<SGraphVertexDesc, std::vector<SGraphEdgeDesc>>& vertex_input_edges);

	CGraph mergeGraph(CGraph* graph_a, CGraph* graph_b);
	
	CVariableNameManager variable_name_manager;

#if TANGRAM_DEBUG
	int graphviz_debug_idx;
#endif
};

void initGlobalShaderGraphBuild();
void addHashedGraphToGlobalGraphBuilder(std::vector<CHashNode>& hash_dependency_graphs, int shader_id);
CGlobalAstNodeRecursiveCopy* getGlobalAstNodeRecursiveCopy();
void buildGlobalShaderGraph();
void releaseGlobalShaderGraph();
