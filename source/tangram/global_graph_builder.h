#pragma once
#include <vector>
#include <string>
#include <unordered_map>

#include "xxhash.h"
#include "common.h"
#include "ast_hash_tree.h"

#include <boost/graph/adjacency_list.hpp>

//#include <boost/lexical_cast.hpp>
//#include <boost/graph/filtered_graph.hpp>
//#include <boost/graph/graph_utility.hpp>
//#include <boost/graph/iteration_macros.hpp>
//#include <boost/graph/mcgregor_common_subgraphs.hpp>
//#include <boost/property_map/shared_array_property_map.hpp>



struct SShaderCodeVertex
{
	XXH64_hash_t hash_value;
#if TANGRAM_DEBUG
	std::string debug_string;
#endif

	inline bool operator==(const SShaderCodeVertex& other) 
	{ 
		return hash_value == other.hash_value; 
	}
};

// Use boost::listS to store the vertices since execute 'remove vertices' causing 'Iterator and Descriptor Stability/Invalidation' https://www.boost.org/doc/libs/1_63_0/libs/graph/doc/adjacency_list.html
// It's has higer per-vertex space overhead https://www.boost.org/doc/libs/1_63_0/libs/graph/doc/using_adjacency_list.html#sec:choosing-graph-type

typedef boost::adjacency_list<boost::listS, boost::vecS, boost::directedS,
	boost::property< boost::vertex_name_t, SShaderCodeVertex,
	boost::property< boost::vertex_index_t, unsigned int > >,
	boost::property< boost::edge_name_t, unsigned int> >
	CGraph;

typedef boost::property_map< CGraph, boost::vertex_name_t >::type VertexNameMap;
typedef boost::property_map< CGraph, boost::vertex_index_t >::type VertexIndexMap;

struct SMcsResult
{
	std::set<size_t> graph_common_vtx_indices[2];
	size_t subgraph_size;
};

class CGlobalGraphsBuilder
{
public:
	void addHashDependencyGraph(std::vector<CHashNode>& hash_dependency_graphs);
#if TANGRAM_DEBUG
	void visGraph(CGraph* graph);
#endif
	void mergeGraphs();
private:
	void generateSubgraphRemoveMcs(CGraph& origin_graph, const std::set<size_t>& mcs_vtx_indices, const std::map<size_t, size_t>& pre_map_new_to_origin, CGraph& generated_subgraph, std::map<size_t, size_t>& map_new_to_origin);
	void findCommonSubgraphMax(CGraph& grapha, CGraph& graphb, SMcsResult& mcsResult);

	std::vector<CGraph> unmerged_graphs;

	CGraph merged_graphs;

	CGraph mergeGraph(CGraph* graph_a, CGraph* graph_b);

#if TANGRAM_DEBUG
	int graphviz_debug_idx;
#endif
};

void initGlobalShaderGraphBuild();
void addHashedGraphToGlobalGraphBuilder(std::vector<CHashNode>& hash_dependency_graphs);
void buildGlobalShaderGraph();
void releaseGlobalShaderGraph();
