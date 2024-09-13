#include "global_graph_builder.h"
#include "graphviz.h"
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

#define DOT_FILE_TO_PNG 1

static CGlobalGraphsBuilder* global_graph_builder = nullptr;

using namespace tangram;
using namespace boost;

void CGlobalGraphsBuilder::addHashDependencyGraph(std::vector<CHashNode>& hash_dependency_graphs)
{
	CGraph builded_graph;
	VertexNameMap vname_map_simple1 = get(boost::vertex_name, builded_graph);

	for (auto& iter : hash_dependency_graphs)
	{
		iter.graph_vtx_idx = add_vertex(builded_graph);
		put(vname_map_simple1, iter.graph_vtx_idx, SShaderCodeVertex{ iter.hash_value,iter.debug_string.c_str()});
	}

	for (const auto& iter : hash_dependency_graphs)
	{
		uint32_t from_vtx_id = iter.graph_vtx_idx;
		for (const auto& ipt_id : iter.out_hash_nodes)
		{
			const CHashNode& hash_node = hash_dependency_graphs[ipt_id];
			const uint32_t to_vtx_id = hash_node.graph_vtx_idx;
			add_edge(from_vtx_id, to_vtx_id, builded_graph);
		}
	}


	// bug test
	//graph_traits<CGraph>::vertex_iterator vi, vi_end, next;
	//tie(vi, vi_end) = vertices(builded_graph);
	//std::vector<size_t> removed_vertices;
	//for (next = vi; vi != vi_end; vi = next) 
	//{
	//	++next;
	//	size_t property_map_idx = *vi;
	//
	//	auto vtx_adj_vertices = adjacent_vertices(property_map_idx, builded_graph);
	//
	//	//non ajacent vertices
	//	if (vtx_adj_vertices.first == vtx_adj_vertices.second)
	//	{
	//		removed_vertices.push_back(*vi);
	//	}
	//}
	//
	//for (auto& t : removed_vertices)
	//{
	//	remove_vertex(t, builded_graph);
	//}

	CGraph processed_graph;
	{
		//std::map<size_t, size_t>map_src_dst;

		VertexNameMap new_map_simple1 = get(boost::vertex_name, processed_graph);

		VertexIndexMap vertex_index_map = get(vertex_index, builded_graph);
		VertexNameMap vertex_name_map = get(vertex_name, builded_graph);

		//auto out_vetices = boost::vertices(builded_graph);
		//for (auto vp = out_vetices; vp.first != vp.second; ++vp.first)
		//{
		//	size_t property_map_idx = *vp.first;
		//
		//	auto vtx_adj_vertices = adjacent_vertices(property_map_idx, builded_graph);
		//	if (vtx_adj_vertices.first != vtx_adj_vertices.second)
		//	{
		//		//size_t  vtx_idx = vertex_index_map[property_map_idx];
		//		SShaderCodeVertex& shader_code_vtx = vertex_name_map[property_map_idx];
		//
		//		size_t dst = add_vertex(processed_graph);
		//		if (dst == 0)
		//		{
		//			std::cout << "aa";
		//		}
		//		put(new_map_simple1, dst, shader_code_vtx);
		//		map_src_dst[property_map_idx] = dst;
		//	}
		//}
		
		std::map<size_t, size_t> map_src_dst2;// = map_src_dst;

		auto es = boost::edges(builded_graph);
		for (auto iterator = es.first; iterator != es.second; iterator++)
		{
			size_t src_property_map_idx = source(*iterator, builded_graph);
			size_t dst_property_map_idx = target(*iterator, builded_graph);

			SShaderCodeVertex& src_shader_code_vtx = vertex_name_map[src_property_map_idx];
			SShaderCodeVertex& dst_shader_code_vtx = vertex_name_map[dst_property_map_idx];

			auto iter_from = map_src_dst2.find(src_property_map_idx);
			auto iter_dst = map_src_dst2.find(dst_property_map_idx);

			if (iter_from == map_src_dst2.end())
			{
				size_t dst = add_vertex(processed_graph);
				put(new_map_simple1, dst, src_shader_code_vtx);
				map_src_dst2[src_property_map_idx] = dst;
			}

			if (iter_dst == map_src_dst2.end())
			{
				size_t dst = add_vertex(processed_graph);
				put(new_map_simple1, dst, dst_shader_code_vtx);
				map_src_dst2[dst_property_map_idx] = dst;
			}
		}

		for (auto iterator = es.first; iterator != es.second; iterator++)
		{
			size_t src_property_map_idx = source(*iterator, builded_graph);
			size_t dst_property_map_idx = target(*iterator, builded_graph);
			const size_t  src_vtx_idx = vertex_index_map[src_property_map_idx];
			const size_t  dst_vtx_idx = vertex_index_map[dst_property_map_idx];
			auto iter_from = map_src_dst2.find(src_vtx_idx);
			auto iter_dst = map_src_dst2.find(dst_vtx_idx);
			if (iter_from != map_src_dst2.end() && iter_dst != map_src_dst2.end())
			{
				add_edge(iter_from->second, iter_dst->second, processed_graph);

				//debug
				SShaderCodeVertex& shader_code_vtx = new_map_simple1[iter_dst->second];
				if (shader_code_vtx.debug_string == "layout(std140)uniform  ViewView_PreExposure")
				{
					std::cout << shader_code_vtx.debug_string;
				}
			}
			else
			{
				std::cout << "aa";
			}
		}
	}

	//print_graph(builded_graph);

	unmerged_graphs.push_back(processed_graph);

#if TANGRAM_DEBUG
	visGraph(&processed_graph);
#endif
}

#if TANGRAM_DEBUG
void CGlobalGraphsBuilder::visGraph(CGraph* graph)
{
	graphviz_debug_idx++;

	std::string out_dot_file = "digraph G{";

	VertexIndexMap vertex_index_map = get(vertex_index, *graph);
	VertexNameMap vertex_name_map = get(vertex_name, *graph);

	auto out_vetices = boost::vertices(*graph);
	for (auto vp = out_vetices; vp.first != vp.second; ++vp.first)
	{
		size_t property_map_idx = *vp.first;
		
		auto vtx_adj_vertices = adjacent_vertices(property_map_idx, *graph);

		//if (vtx_adj_vertices.first != vtx_adj_vertices.second)
		{
			size_t  vtx_idx = vertex_index_map[property_map_idx];
			SShaderCodeVertex& shader_code_vtx = vertex_name_map[property_map_idx];

			out_dot_file += std::format("Node_{0} [label=\"{1}\"]\n", vtx_idx, shader_code_vtx.debug_string);
		}
	}

	auto es = boost::edges(*graph);
	for (auto iterator = es.first; iterator != es.second; iterator++)
	{
		size_t src_property_map_idx = source(*iterator, *graph);
		size_t dst_property_map_idx = target(*iterator, *graph);
		size_t  src_vtx_idx = vertex_index_map[src_property_map_idx];
		size_t  dst_vtx_idx = vertex_index_map[dst_property_map_idx];
		out_dot_file += std::format("Node_{0} -> Node_{1}\n", src_vtx_idx, dst_vtx_idx);
	}

	out_dot_file += "}";

	std::string dot_file_path = intermediatePath() + std::to_string(graphviz_debug_idx) + ".dot";
	std::ofstream output_dotfile(dot_file_path);

	output_dotfile << out_dot_file;

	output_dotfile.close();

#if DOT_FILE_TO_PNG
	std::string dot_to_png_cmd = "dot -Tpng " + dot_file_path + " -o " + intermediatePath() + std::to_string(graphviz_debug_idx) + ".png";
	system(dot_to_png_cmd.c_str());
#endif
}
#endif



class CMergeGraphDFSVisitor : public boost::default_dfs_visitor
{
public:
	CMergeGraphDFSVisitor(CGraph ipt_new_graph)
		:new_graph(ipt_new_graph) {}


	void examine_edge(SShaderCodeEdge e, CGraph g)const
	{

	}

private:
	CGraph new_graph;
};

struct SOutputMaxComSubGraph
{
public:
	typedef typename boost::graph_traits< CGraph >::vertices_size_type VertexSizeFirst;

	SOutputMaxComSubGraph(const CGraph& graph1) : m_graph1(graph1) {}

	template < typename CorrespondenceMapFirstToSecond, typename CorrespondenceMapSecondToFirst >
	bool operator()(CorrespondenceMapFirstToSecond correspondence_map_1_to_2, CorrespondenceMapSecondToFirst correspondence_map_2_to_1, VertexSizeFirst subgraph_size)
	{
		typedef shared_array_property_map< bool, VertexIndexMap > MembershipMap;

		MembershipMap membership_map1(num_vertices(m_graph1), get(vertex_index, m_graph1));

		fill_membership_map< CGraph >(m_graph1, correspondence_map_1_to_2, membership_map1);

		// Generate filtered graphs using membership map
		typedef typename membership_filtered_graph_traits< CGraph, MembershipMap >::graph_type MembershipFilteredGraph;

		MembershipFilteredGraph subgraph1 = make_membership_filtered_graph(m_graph1, membership_map1);

		// Print the graph out to the console
		std::cout << "Found common subgraph (size " << subgraph_size << ")"
			<< std::endl;
		print_graph(subgraph1);
		std::cout << std::endl;

		// Explore the entire space
		return (true);
	}

private:
	//std::set<>*;
	//std::map<>*;

	const CGraph& m_graph1;
	VertexSizeFirst m_max_subgraph_size;
};


CGraph CGlobalGraphsBuilder::mergeGraph(CGraph* graph_a,CGraph* graph_b)
{
	VertexNameMap vtx_name_map_a = get(boost::vertex_name, *graph_a);
	VertexNameMap vtx_name_map_b = get(boost::vertex_name, *graph_b);

	SOutputMaxComSubGraph output_maxcom_graph(*graph_a); 
	mcgregor_common_subgraphs_maximum_unique(*graph_a, *graph_b, true, output_maxcom_graph, vertices_equivalent(make_property_map_equivalent(vtx_name_map_a, vtx_name_map_b)));
	
	//CGraph new_graph = *graph_a;
	//CMergeGraphDFSVisitor merge_graph_dfs_visitor(new_graph);
	//depth_first_search(*graph_a, visitor(merge_graph_dfs_visitor));

	//以A图为基础，DFS遍历B图
	//对于B图中所遍历到的任何一个顶点
	//	如果这个顶点是最大公共子图的节点之一
	//		不需要再次加入
	//	如果不是公共节点
	//		增加改节点，并将改节点的入边加入到图中
	//		遍历所有出边
	//			如果出边对应出顶点输入最大公共子图的节点之一
	//				增加一条边，连接这个顶点



	return CGraph();
}

void CGlobalGraphsBuilder::mergeGraphs()
{
	// test graph
	CGraph graph_a;
	CGraph graph_b;

	
	constexpr int debugGraph = 2;
	{
		VertexIndexMap vertex_index_map = get(vertex_index, unmerged_graphs[debugGraph]);
		VertexNameMap vertex_name_map = get(vertex_name, unmerged_graphs[debugGraph]);
		auto out_vetices = boost::vertices(unmerged_graphs[debugGraph]);
		for (auto vp = out_vetices; vp.first != vp.second; ++vp.first)
		{
			size_t property_map_idx = *vp.first;

			//auto vtx_adj_vertices = adjacent_vertices(property_map_idx, unmerged_graphs[1]);
			//if (vtx_adj_vertices.first != vtx_adj_vertices.second)
			
			{
				size_t  vtx_idx = vertex_index_map[property_map_idx];
				SShaderCodeVertex& shader_code_vtx = vertex_name_map[property_map_idx];

				std::cout << "size_t vtx_" << vtx_idx << " = add_vertex(graph_temp); put(vname_map_simple1, vtx_" << vtx_idx << ",SShaderCodeVertex{ " << shader_code_vtx.hash_value << "ULL , " << "\"" << shader_code_vtx.debug_string << "\"});" << std::endl;
			}
		}

		auto es = boost::edges(unmerged_graphs[debugGraph]);
		for (auto iterator = es.first; iterator != es.second; iterator++)
		{
			size_t src_property_map_idx = source(*iterator, unmerged_graphs[debugGraph]);
			size_t dst_property_map_idx = target(*iterator, unmerged_graphs[debugGraph]);
			
			std::cout << "add_edge(vtx_" << src_property_map_idx << ",vtx_" << dst_property_map_idx << ",graph_temp);" << std::endl;
		}
	}

	{
		VertexNameMap vname_map_simple1 = get(boost::vertex_name, graph_a);
		CGraph& graph_temp = graph_a;

		size_t vtx_2 = add_vertex(graph_temp); put(vname_map_simple1, vtx_2, SShaderCodeVertex{ 4667699555926963197ULL , "layout(std140)uniform  MobileBasePassMobileBasePass_bManualEyeAdaption" });
		
		size_t vtx_4 = add_vertex(graph_temp); put(vname_map_simple1, vtx_4, SShaderCodeVertex{ 14027090263269972666ULL , "layout(std140)uniform  MobileBasePassMobileBasePass_DefaultEyeExposure" });
		size_t vtx_5 = add_vertex(graph_temp); put(vname_map_simple1, vtx_5, SShaderCodeVertex{ 421373844136946760ULL , "uniform highp vec4pc2_h[2]" });
		size_t vtx_6 = add_vertex(graph_temp); put(vname_map_simple1, vtx_6, SShaderCodeVertex{ 17971120611723594491ULL , "2_585_17873430062239417437_6846267010982278733" });
		size_t vtx_7 = add_vertex(graph_temp); put(vname_map_simple1, vtx_7, SShaderCodeVertex{ 7746059650824637115ULL , "uniform highp samplerBufferps0" });
		size_t vtx_8 = add_vertex(graph_temp); put(vname_map_simple1, vtx_8, SShaderCodeVertex{ 3473799682743744372ULL , "uniform mediump sampler2Dps1" });
		size_t vtx_9 = add_vertex(graph_temp); put(vname_map_simple1, vtx_9, SShaderCodeVertex{ 13821842053333811058ULL , "uniform mediump sampler2Dps2" });
		size_t vtx_10 = add_vertex(graph_temp); put(vname_map_simple1, vtx_10, SShaderCodeVertex{ 11433205934588975520ULL , "uniform mediump sampler2Dps3" });
		size_t vtx_11 = add_vertex(graph_temp); put(vname_map_simple1, vtx_11, SShaderCodeVertex{ 15231287368500543158ULL , "layout( location=1)in highp vec4" });
		size_t vtx_12 = add_vertex(graph_temp); put(vname_map_simple1, vtx_12, SShaderCodeVertex{ 4529962203236050115ULL , "2_585_1174659732613877816_13819668646401522794" });
		size_t vtx_13 = add_vertex(graph_temp); put(vname_map_simple1, vtx_13, SShaderCodeVertex{ 6661346318500704738ULL , "2_585_8988885434030579837_5810306597718727437" });

		size_t vtx_3 = add_vertex(graph_temp); put(vname_map_simple1, vtx_3, SShaderCodeVertex{ 13085795960708241157ULL , "2_585_13239296054265662906_7792014216068258029" });
		size_t vtx_0 = add_vertex(graph_temp); put(vname_map_simple1, vtx_0, SShaderCodeVertex{ 5799338323037263073ULL , "layout(std140)uniform  ViewView_PreExposure" });
		size_t vtx_1 = add_vertex(graph_temp); put(vname_map_simple1, vtx_1, SShaderCodeVertex{ 8304811195687460811ULL , "2_585_16150756779487078058_3161357532303492359" });
		size_t vtx_14 = add_vertex(graph_temp); put(vname_map_simple1, vtx_14, SShaderCodeVertex{ 1445264608434258223ULL , "2_585_14973197469294367889_8032819896168772964" });
		add_edge(vtx_0, vtx_1, graph_temp);
		add_edge(vtx_1, vtx_14, graph_temp);
			add_edge(vtx_2, vtx_3, graph_temp);
		add_edge(vtx_3, vtx_1, graph_temp);
		add_edge(vtx_3, vtx_14, graph_temp);
			add_edge(vtx_4, vtx_3, graph_temp);
				add_edge(vtx_5, vtx_6, graph_temp);
			add_edge(vtx_6, vtx_3, graph_temp);
			add_edge(vtx_7, vtx_3, graph_temp);
				add_edge(vtx_8, vtx_6, graph_temp);
				add_edge(vtx_9, vtx_6, graph_temp);
				add_edge(vtx_10, vtx_6, graph_temp);
	add_edge(vtx_11, vtx_12, graph_temp);
	add_edge(vtx_11, vtx_13, graph_temp);
				add_edge(vtx_12, vtx_6, graph_temp);
				add_edge(vtx_13, vtx_6, graph_temp);
	}

	{
		VertexNameMap vname_map_simple1 = get(boost::vertex_name, graph_b);
		CGraph& graph_temp = graph_b;
		
		
		size_t vtx_2 = add_vertex(graph_temp); put(vname_map_simple1, vtx_2, SShaderCodeVertex{ 4667699555926963197ULL , "layout(std140)uniform  MobileBasePassMobileBasePass_bManualEyeAdaption" });
		
		size_t vtx_4 = add_vertex(graph_temp); put(vname_map_simple1, vtx_4, SShaderCodeVertex{ 14027090263269972666ULL , "layout(std140)uniform  MobileBasePassMobileBasePass_DefaultEyeExposure" });
		size_t vtx_5 = add_vertex(graph_temp); put(vname_map_simple1, vtx_5, SShaderCodeVertex{ 421373844136946760ULL , "uniform highp vec4pc2_h[2]" });
		size_t vtx_6 = add_vertex(graph_temp); put(vname_map_simple1, vtx_6, SShaderCodeVertex{ 17971120611723594491ULL , "2_585_17873430062239417437_6846267010982278733" });
		size_t vtx_7 = add_vertex(graph_temp); put(vname_map_simple1, vtx_7, SShaderCodeVertex{ 7746059650824637115ULL , "uniform highp samplerBufferps0" });
		size_t vtx_8 = add_vertex(graph_temp); put(vname_map_simple1, vtx_8, SShaderCodeVertex{ 3473799682743744372ULL , "uniform mediump sampler2Dps1" });
		size_t vtx_9 = add_vertex(graph_temp); put(vname_map_simple1, vtx_9, SShaderCodeVertex{ 13821842053333811058ULL , "uniform mediump sampler2Dps2" });
		size_t vtx_10 = add_vertex(graph_temp); put(vname_map_simple1, vtx_10, SShaderCodeVertex{ 11433205934588975520ULL , "uniform mediump sampler2Dps3" });
		size_t vtx_11 = add_vertex(graph_temp); put(vname_map_simple1, vtx_11, SShaderCodeVertex{ 9209500962628141972ULL , "layout( location=2)in highp vec4" });
		size_t vtx_12 = add_vertex(graph_temp); put(vname_map_simple1, vtx_12, SShaderCodeVertex{ 13560859697935552866ULL , "layout( location=3)in highp vec4" });
		size_t vtx_13 = add_vertex(graph_temp); put(vname_map_simple1, vtx_13, SShaderCodeVertex{ 4529962203236050115ULL , "2_585_1174659732613877816_13819668646401522794" });
		size_t vtx_14 = add_vertex(graph_temp); put(vname_map_simple1, vtx_14, SShaderCodeVertex{ 6661346318500704738ULL , "2_585_8988885434030579837_5810306597718727437" });

		size_t vtx_3 = add_vertex(graph_temp); put(vname_map_simple1, vtx_3, SShaderCodeVertex{ 9004810436692339929ULL , "2_585_13239296054265662906_1350948457307568115" });
		size_t vtx_0 = add_vertex(graph_temp); put(vname_map_simple1, vtx_0, SShaderCodeVertex{ 5799338323037263073ULL , "layout(std140)uniform  ViewView_PreExposure" });
		size_t vtx_1 = add_vertex(graph_temp); put(vname_map_simple1, vtx_1, SShaderCodeVertex{ 8304811195687460811ULL , "2_585_16150756779487078058_3161357532303492359" });
		size_t vtx_15 = add_vertex(graph_temp); put(vname_map_simple1, vtx_15, SShaderCodeVertex{ 1445264608434258223ULL , "2_585_14973197469294367889_8032819896168772964" });

		add_edge(vtx_0, vtx_1, graph_temp);
		add_edge(vtx_1, vtx_15, graph_temp);
			add_edge(vtx_2, vtx_3, graph_temp);
		add_edge(vtx_3, vtx_1, graph_temp);
		add_edge(vtx_3, vtx_15, graph_temp);
			add_edge(vtx_4, vtx_3, graph_temp);
				add_edge(vtx_5, vtx_6, graph_temp);
			add_edge(vtx_6, vtx_3, graph_temp);
			add_edge(vtx_7, vtx_3, graph_temp);
				add_edge(vtx_8, vtx_6, graph_temp);
				add_edge(vtx_9, vtx_6, graph_temp);
				add_edge(vtx_10, vtx_6, graph_temp);
			add_edge(vtx_11, vtx_3, graph_temp);
	add_edge(vtx_12, vtx_13, graph_temp);
	add_edge(vtx_12, vtx_14, graph_temp);
				add_edge(vtx_13, vtx_6, graph_temp);
				add_edge(vtx_14, vtx_6, graph_temp);
	}

	VertexNameMap vtx_name_map_a = get(boost::vertex_name, graph_a);
	VertexNameMap vtx_name_map_b = get(boost::vertex_name, graph_b);
	SOutputMaxComSubGraph output_maxcom_graph(graph_a);
	mcgregor_common_subgraphs_maximum_unique(graph_a, graph_b, true, output_maxcom_graph, vertices_equivalent(make_property_map_equivalent(vtx_name_map_a, vtx_name_map_b)));

	//for (int test_index = 0; test_index < 4; test_index += 2)
	int test_index = 1;
	{
		mergeGraph(&unmerged_graphs[test_index], &unmerged_graphs[test_index + 1]);
	}
}

void initGlobalShaderGraphBuild()
{
	if (global_graph_builder == nullptr)
	{
		global_graph_builder = new CGlobalGraphsBuilder();
	}
}

void addHashedGraphToGlobalGraphBuilder(std::vector<CHashNode>& hash_dependency_graphs)
{
	if (global_graph_builder == nullptr)
	{
		assert(false && "global_graph_builder == nullptr");
	}

	global_graph_builder->addHashDependencyGraph(hash_dependency_graphs);
}

void buildGlobalShaderGraph()
{
	global_graph_builder->mergeGraphs();
}

void releaseGlobalShaderGraph()
{
	if (global_graph_builder != nullptr)
	{
		delete global_graph_builder;
	}
}
