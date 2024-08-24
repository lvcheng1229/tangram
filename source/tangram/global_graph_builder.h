#pragma once
#include <vector>
#include <string>
#include <unordered_map>

#include "xxhash.h"
#include "common.h"
#include "ast_hash_tree.h"

#include <boost/lexical_cast.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/mcgregor_common_subgraphs.hpp>
#include <boost/property_map/shared_array_property_map.hpp>

struct SShaderCodeVertex
{
	XXH64_hash_t hash_value;
#if TANGRAM_DEBUG
	std::string debug_string;
#endif

	inline bool operator==(const SShaderCodeVertex& other) { return hash_value == other.hash_value; }
};

class CGlobalGraphsBuilder
{
public:
	void addHashDependencyGraph(std::vector<CHashNode>& hash_dependency_graphs);
	void mergeGraphs();

private:
	typedef boost::adjacency_list< 
		boost::listS, boost::vecS, boost::directedS,
		boost::property< boost::vertex_name_t, SShaderCodeVertex,
		boost::property< boost::vertex_index_t, unsigned int > >,
		boost::property< boost::edge_name_t, unsigned int > >
		Graph;

	typedef boost::property_map< Graph, boost::vertex_name_t >::type VertexNameMap;

	std::vector<Graph> unmerged_graphs;

	Graph merged_graphs;
#if TANGRAM_DEBUG
	int graphviz_debug_idx;
#endif
};