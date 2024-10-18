#include "shader_compressor.h"
#include "tangram.h"
#include "glslang_helper.h"

//todo: uniform buffer padding 目前都是按float为半精度来计算的，需要所有声明之前加入半精度的声明

void SCodeBlockTable::outputCodeBlockTable()
{
#if TANGRAM_DEBUG
	for (int idx = 0; idx < code_blocks.size(); idx++)
	{
		std::cout << "code block index:" << idx << std::endl;

		SCodeBlock& code_block = code_blocks[idx];
		if (code_block.is_ub)
		{
			std::cout << code_block.uniform_buffer<< std::endl;
		}
		else if (code_block.is_main_begin)
		{
			std::cout << "main(){" << std::endl;
		}
		else if (code_block.is_main_end)
		{
			std::cout << "}" << std::endl;
		}
		else
		{
			std::cout << code_block.code << std::endl;
		}
		std::cout << std::endl;
	}
#endif
}

void generatePadding(std::string& result, int offset, int member_offset, int& padding_index)
{
	assert(offset < member_offset);
	int padding_size = member_offset - offset;

	// vec4 padding
	int vec4_size = 2 * 4;
	int vec4_padding_num = padding_size / vec4_size;
	if (vec4_padding_num > 0)
	{
		std::string padding_string = "vec4 padding" + std::to_string(padding_index);
		if (vec4_padding_num > 1)
		{
			padding_string += "[" + std::to_string(vec4_padding_num) + "]";
		}
		padding_string += ";\n";
		result.append(padding_string);
		padding_index++;
	}

	padding_size = padding_size - vec4_padding_num * vec4_size;

	int float_size = 2;
	int float_padding_num = padding_size / float_size;
	for (int flt_pad_idx = 0; flt_pad_idx < float_padding_num; flt_pad_idx++)
	{
		std::string padding_string = "float padding" + std::to_string(padding_index) + ";\n";
		result.append(padding_string);
		padding_index++;
	}

	assert((padding_size - float_size * float_padding_num) == 0);

}

std::string generateUniformBuffer(std::vector<SUniformBufferMemberDesc*>& ub_mem_descs, TTanGramIntermSymbol* ub_node, const std::string& struct_name)
{
	const TType& type = ub_node->getType();
	std::string ub_string;
	const TQualifier& qualifier = type.getQualifier();
	if (qualifier.hasLayout())
	{
		ub_string.append("layout(");
		if (qualifier.hasPacking())
		{
			ub_string.append(TQualifier::getLayoutPackingString(qualifier.layoutPacking));
		}
		ub_string.append(")");
	}

	ub_string.append(type.getStorageQualifierString());
	ub_string.append(" ");
	ub_string.append(struct_name);
	ub_string.append("{\n");

	int padding_index = 0;
	int offset = 0;
	for (int desc_idx = 0; desc_idx < ub_mem_descs.size(); desc_idx++)
	{
		if (desc_idx > 0 && ub_mem_descs[desc_idx]->struct_member_offset == ub_mem_descs[desc_idx - 1]->struct_member_offset)
		{
			continue;
		}

		SUniformBufferMemberDesc* ub_mem_desc = ub_mem_descs[desc_idx];
		if (offset < ub_mem_desc->struct_member_offset)
		{
			generatePadding(ub_string, offset, ub_mem_desc->struct_member_offset, padding_index); 
			offset = ub_mem_desc->struct_member_offset;
		}
		
		{
			const TTypeList* structure = type.getStruct();
			TType* struct_mem_type = (*structure)[ub_mem_desc->struct_index].type;
			ub_string.append(getTypeText(*struct_mem_type, false, false));
			ub_string.append(" ");
			std::string new_member_name;
			getGlobalVariableNameManager()->getNewStructMemberName(ub_mem_desc->struct_instance_hash, ub_mem_desc->struct_member_hash, new_member_name);
			ub_string.append(new_member_name);
			ub_string.append(";\n");
			offset += ub_mem_desc->struct_member_size;
		}
	}
	
	if (offset < ub_mem_descs[0]->struct_size)
	{
		generatePadding(ub_string, offset, ub_mem_descs[0]->struct_size, padding_index);
	}


	ub_string.append("}");
	std::string struct_inst_name;
	getGlobalVariableNameManager()->getNewStructInstanceName(ub_mem_descs[0]->struct_instance_hash, struct_inst_name);
	ub_string.append(struct_inst_name);
	ub_string.append(";");
	return ub_string;
}

static bool uniformBufferMemberCompare(SUniformBufferMemberDesc* ub_member_a, SUniformBufferMemberDesc* ub_member_b)
{
	return ub_member_a->struct_member_offset < ub_member_b->struct_member_offset;
}

void CShaderCompressor::partition()
{
	TAstToGLTraverser new_glsl_converter(true);
	glsl_converter = &new_glsl_converter;

	std::vector<SGraphVertexDesc> start_vertex_descs;
	for (STopologicalOrderVetices::reverse_iterator vtx_desc_iter = topological_order_vertices.rbegin(); vtx_desc_iter != topological_order_vertices.rend(); ++vtx_desc_iter)
	{
		auto input_edges_iter = vertex_input_edges.find(*vtx_desc_iter);
		if (input_edges_iter == vertex_input_edges.end()) // linker node
		{
			start_vertex_descs.push_back(*vtx_desc_iter);
		}
	}

	// uniform buffer 遍历
	for (int start_vertex_idx = 0; start_vertex_idx < start_vertex_descs.size(); start_vertex_idx++)
	{
		const SGraphVertexDesc& vtx_desc = start_vertex_descs[start_vertex_idx];
		SShaderCodeVertex& shader_vertex = vtx_name_map[vtx_desc];
		if (shader_vertex.is_ub_member)
		{
			if (node_found.find(vtx_desc) == node_found.end())
			{
				node_found.insert(vtx_desc);

				shader_vertex.code_block_index = block_index;
				std::vector<SUniformBufferMemberDesc*> ub_mem_descs;
				TTanGramIntermSymbol* ub_node = getTanGramNode(shader_vertex.interm_node->getAsSymbolNode());
				SUniformBufferMemberDesc* ub_mem_desc = ub_node->getUBMemberDesc();
				ub_mem_descs.push_back(ub_mem_desc);
				for (int same_hash_idx = 0; same_hash_idx < start_vertex_descs.size(); same_hash_idx++)
				{
					if (same_hash_idx != start_vertex_idx)
					{
						const SGraphVertexDesc& same_hash_vtx_desc = start_vertex_descs[same_hash_idx];
						SShaderCodeVertex& same_hash_shader_vertex = vtx_name_map[same_hash_vtx_desc];
						if (same_hash_shader_vertex.is_ub_member)
						{
							SUniformBufferMemberDesc* same_ub_mem_desc = getTanGramNode(same_hash_shader_vertex.interm_node->getAsSymbolNode())->getUBMemberDesc();

							// symbol name里面存储的是ub struct的名字，而不是ub structural instance的名字，因为gl是一高struct的名字来进行绑定的
							if (same_ub_mem_desc->struct_instance_hash == ub_mem_desc->struct_instance_hash && same_hash_shader_vertex.symbol_name == shader_vertex.symbol_name)
							{
								if (node_found.find(same_hash_vtx_desc) == node_found.end())
								{
									node_found.insert(same_hash_vtx_desc);
									same_hash_shader_vertex.code_block_index = block_index;
									ub_mem_descs.push_back(same_ub_mem_desc);
								}
							}
						}
					}
				}


				std::sort(ub_mem_descs.begin(), ub_mem_descs.end(), uniformBufferMemberCompare);

				code_block_table.code_blocks.emplace_back(SCodeBlock());
				code_block_table.code_blocks.back().is_ub = true;
				code_block_table.code_blocks.back().uniform_buffer = generateUniformBuffer(ub_mem_descs, ub_node, shader_vertex.symbol_name);
				block_index++;
			}
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

			// code table gen
			code_block_table.code_blocks.emplace_back(SCodeBlock());
			glsl_converter->setCodeBlockContext(&shader_vertex.vtx_info.ipt_variable_names, &shader_vertex.vtx_info.opt_variable_names);
			shader_vertex.interm_node->traverse(glsl_converter);
			//code_block_table.code_blocks.back().code_units.push_back(SCodeUnit{ glsl_converter->getCodeUnitString() });

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

							// code table gen
							glsl_converter->setCodeBlockContext(&same_hash_shader_vertex.vtx_info.ipt_variable_names, &same_hash_shader_vertex.vtx_info.opt_variable_names);
							same_hash_shader_vertex.interm_node->traverse(glsl_converter);
							

							same_hash_shader_vertex.code_block_index = block_index;
						}
					}
				}
			}
			code_block_table.code_blocks.back().code = glsl_converter->getCodeUnitString();
			block_index++;
		}
	}

	code_block_table.code_blocks.emplace_back(SCodeBlock());
	code_block_table.code_blocks.back().is_main_begin = true;
	block_index++;

	int main_code_block_begin = block_index;

	for (STopologicalOrderVetices::reverse_iterator vtx_desc_iter = topological_order_vertices.rbegin(); vtx_desc_iter != topological_order_vertices.rend(); ++vtx_desc_iter)
	{
		if (node_found.find(*vtx_desc_iter) == node_found.end())
		{
			node_found.insert(*vtx_desc_iter);
			SShaderCodeVertex& shader_vertex = vtx_name_map[*vtx_desc_iter];
			shader_vertex.code_block_index = block_index;

			code_block_table.code_blocks.emplace_back(SCodeBlock());

			// code table gen
			//glsl_converter->setCodeBlockContext(&shader_vertex.vtx_info.ipt_variable_names, &shader_vertex.vtx_info.opt_variable_names);
			//shader_vertex.interm_node->traverse(glsl_converter);
			//code_block_table.code_blocks.emplace_back(SCodeBlock());
			//code_block_table.code_blocks.back().code_units.push_back(SCodeUnit{ glsl_converter->getCodeUnitString() });

			floodSearch(*vtx_desc_iter, shader_vertex.vertex_shader_ids_hash);
			block_index++;
		}
	}

	for (int code_block_gen_index = main_code_block_begin; code_block_gen_index < block_index; code_block_gen_index++)
	{
		int found_num = 0;
		
		
		SCodeBlock& code_block = code_block_table.code_blocks[code_block_gen_index];
		for (STopologicalOrderVetices::reverse_iterator vtx_desc_iter = topological_order_vertices.rbegin(); vtx_desc_iter != topological_order_vertices.rend() && found_num <= code_block.vertex_num; ++vtx_desc_iter)
		{
			SShaderCodeVertex& shader_vertex = vtx_name_map[*vtx_desc_iter];
			if (shader_vertex.code_block_index == code_block_gen_index)
			{
				glsl_converter->setCodeBlockContext(&shader_vertex.vtx_info.ipt_variable_names, &shader_vertex.vtx_info.opt_variable_names);
				shader_vertex.interm_node->traverse(glsl_converter);
				found_num++;
			}
		}
		code_block.code = glsl_converter->getCodeUnitString();
	}

	code_block_table.code_blocks.emplace_back(SCodeBlock());
	code_block_table.code_blocks.back().is_main_end = true;
	block_index++;

#if TANGRAM_DEBUG
	code_block_table.outputCodeBlockTable();
#endif

	glsl_converter = nullptr;
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
					floodSearch(vertex_desc, pre_node_block_hash);
					code_block_table.code_blocks.back().vertex_num++;
					// code table gen
					//glsl_converter->setCodeBlockContext(&next_shader_vertex.vtx_info.ipt_variable_names, &next_shader_vertex.vtx_info.opt_variable_names);
					//next_shader_vertex.interm_node->traverse(glsl_converter);
					//code_block_table.code_blocks.back().code_units.push_back(SCodeUnit{ glsl_converter->getCodeUnitString() });

					next_shader_vertex.code_block_index = block_index;
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
				code_block_table.code_blocks.back().vertex_num++;

				// code table gen
				//glsl_converter->setCodeBlockContext(&next_shader_vertex.vtx_info.ipt_variable_names, &next_shader_vertex.vtx_info.opt_variable_names);
				//next_shader_vertex.interm_node->traverse(glsl_converter);
				//code_block_table.code_blocks.back().code_units.push_back(SCodeUnit{ glsl_converter->getCodeUnitString() });

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