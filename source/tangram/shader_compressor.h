#pragma once
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

class CShaderCompressor
{
public:
	CShaderCompressor(std::map<SGraphVertexDesc, std::vector<SGraphEdgeDesc>>& vertex_input_edges, STopologicalOrderVetices& topological_order_vertices, CGraph& graph)
		:vertex_input_edges(vertex_input_edges)
		, topological_order_vertices(topological_order_vertices)
		, graph(graph)
	{
		previousAllocator = &GetThreadPoolAllocator();
		builtInPoolAllocator = new TPoolAllocator;

		SetThreadPoolAllocator(builtInPoolAllocator);
		glsl_converter = new TAstToGLTraverser();

		vtx_name_map = get(boost::vertex_name, graph);
		block_index = 0;
	}

	~CShaderCompressor()
	{
		delete builtInPoolAllocator;
		SetThreadPoolAllocator(previousAllocator);
	}
	void partition();

private:

	void floodSearch(SGraphVertexDesc vtx_desc, std::size_t pre_node_block_hash);

	const std::map<SGraphVertexDesc, std::vector<SGraphEdgeDesc>>& vertex_input_edges;
	STopologicalOrderVetices& topological_order_vertices;
	CGraph& graph;

	VertexNameMap vtx_name_map;
	int block_index;
	std::set<SGraphVertexDesc> node_found;
	
	TPoolAllocator* previousAllocator;
	TPoolAllocator* builtInPoolAllocator;

	TAstToGLTraverser* glsl_converter;
};