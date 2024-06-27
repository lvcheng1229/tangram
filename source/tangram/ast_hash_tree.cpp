#include "ast_hash_tree.h"

static int global_seed = 42;

// find and store the symbols in the scope in order
void CScopeSymbolNameTraverser::visitSymbol(TIntermSymbol* node)
{
	const TString& node_string = node->getName();
	XXH64_hash_t hash_value = XXH64(node_string.data(), node_string.size(), global_seed);
	auto iter = symbol_index.find(hash_value);
	if (iter == symbol_index.end())
	{
		symbol_index[hash_value] = symbol_idx;
		symbol_idx++;
	}
}

TString CASTHashTreeBuilder::getTypeText(const TType& type, bool getQualifiers, bool getSymbolName, bool getPrecision)
{
	TString type_string;

	const auto appendStr = [&](const char* s) { type_string.append(s); };
	const auto appendUint = [&](unsigned int u) { type_string.append(std::to_string(u).c_str()); };
	const auto appendInt = [&](int i) { type_string.append(std::to_string(i).c_str()); };

	TBasicType basic_type = type.getBasicType();
	bool is_vec = type.isVector();
	bool is_blk = (basic_type == EbtBlock);
	bool is_mat = type.isMatrix();

	const TQualifier& qualifier = type.getQualifier();
	if (getQualifiers)
	{
		if (qualifier.hasLayout())
		{
			appendStr("layout(");
			if (qualifier.hasAnyLocation())
			{
				appendStr(" location=");
				appendUint(qualifier.layoutLocation);
			}
			if (qualifier.hasPacking())
			{
				appendStr(TQualifier::getLayoutPackingString(qualifier.layoutPacking));
			}
			appendStr(")");
		}

		bool should_out_storage_qualifier = true;
		if (type.getQualifier().storage == EvqTemporary ||
			type.getQualifier().storage == EvqGlobal)
		{
			should_out_storage_qualifier = false;
		}

		if (should_out_storage_qualifier)
		{
			appendStr(type.getStorageQualifierString());
			appendStr(" ");
		}

		{
			appendStr(type.getPrecisionQualifierString());
			appendStr(" ");
		}
	}

	if ((getQualifiers == false) && getPrecision)
	{
		{
			appendStr(type.getPrecisionQualifierString());
			appendStr(" ");
		}
	}

	if (is_vec)
	{
		switch (basic_type)
		{
		case EbtDouble:
			appendStr("d");
			break;
		case EbtInt:
			appendStr("i");
			break;
		case EbtUint:
			appendStr("u");
			break;
		case EbtBool:
			appendStr("b");
			break;
		case EbtFloat:
		default:
			break;
		}
	}

	if (is_mat)
	{
		appendStr("mat");

		int mat_raw_num = type.getMatrixRows();
		int mat_col_num = type.getMatrixCols();

		if (mat_raw_num == mat_col_num)
		{
			appendInt(mat_raw_num);
		}
		else
		{
			appendInt(mat_raw_num);
			appendStr("x");
			appendInt(mat_col_num);
		}
	}

	if (is_vec)
	{
		appendStr("vec");
		appendInt(type.getVectorSize());
	}

	return type_string;
}

// binary hash
// 2_operator_lefthash_righthash

bool CASTHashTreeBuilder::visitBinary(TVisit visit, TIntermBinary* node)
{
	TOperator node_operator = node->getOp();
	TString hash_string;

	if (visit == EvPostVisit || (visit == EvPreVisit && (node_operator == EOpAssign || node_operator == EOpIndexDirectStruct)))
	{
		hash_string.reserve(2 + 3 + 5 + 5);
		hash_string.append(std::string("2_")); 
		hash_string.append(std::to_string(uint32_t(node_operator)).c_str());
		hash_string.append(std::string("_"));
	}

	switch (node_operator)
	{
	case EOpAssign:
	{
#if TANGRAM_DEBUG
		debug_traverser.visitBinary(EvPreVisit, node);
		debug_traverser.incrementDepth(node);
#endif

		if (visit == EvPreVisit)
		{
			// find all of the symbols in the scope
			builder_context.scopeReset();
			scope_symbol_traverser.reset();
			node->getLeft()->traverse(&scope_symbol_traverser);
			node->getRight()->traverse(&scope_symbol_traverser);

			// output symbols
			if (node->getLeft())
			{
				builder_context.op_assign_visit_output_symbols = true;
				node->getLeft()->traverse(this);
				builder_context.op_assign_visit_output_symbols = false;
			}

#if TANGRAM_DEBUG
			debug_traverser.visitBinary(EvInVisit, node);
#endif

			// input symbols
			if (node->getRight())
			{
				builder_context.op_assign_visit_input_symbols = true;
				node->getRight()->traverse(this);
				builder_context.op_assign_visit_input_symbols = false;
			}

			XXH64_hash_t right_hash_value = hash_value_stack.back();
			hash_value_stack.pop_back();

			XXH64_hash_t left_hash_value = hash_value_stack.back();
			hash_value_stack.pop_back();
			
			hash_string.append(std::to_string(left_hash_value).c_str());
			hash_string.append(std::string("_"));
			hash_string.append(std::to_string(right_hash_value).c_str());
			XXH64_hash_t hash_value = XXH64(hash_string.data(), hash_string.size(), global_seed);

			CHashNode func_hash_node;
			func_hash_node.hash_value = hash_value;
#if TANGRAM_DEBUG
			func_hash_node.debug_string = hash_string;
#endif
			std::vector<XXH64_hash_t>& output_hash_values = builder_context.getOutputHashValues();
			std::vector<XXH64_hash_t>& input_hash_values = builder_context.getInputHashValues();

			for (auto& in_symbol_hash : input_hash_values)
			{
				XXH64_hash_t symbol_last_assign_func_node = 0; 

				if (builder_context.symbol_last_hashnode_map.find(in_symbol_hash) == builder_context.symbol_last_hashnode_map.end()) //linker obeject
				{
					symbol_last_assign_func_node = in_symbol_hash;
				}
				else
				{
					symbol_last_assign_func_node = builder_context.symbol_last_hashnode_map[in_symbol_hash];
				}

				assert_t(hash_value_to_idx.find(symbol_last_assign_func_node) != hash_value_to_idx.end());

				func_hash_node.input_hash_nodes.push_back(hash_value_to_idx[symbol_last_assign_func_node]);
				std::set<uint64_t>& parent_linknode = tree_hash_nodes[hash_value_to_idx[symbol_last_assign_func_node]].out_hash_nodes;
				parent_linknode.insert(tree_hash_nodes.size());
			}

			tree_hash_nodes.push_back(func_hash_node);
			hash_value_to_idx[hash_value] = tree_hash_nodes.size() - 1;
			hash_value_stack.push_back(hash_value);

#if TANGRAM_DEBUG
			debug_traverser.decrementDepth();
			debug_traverser.appendDebugString("[CHashStr:");
			debug_traverser.appendDebugString(hash_string);
			debug_traverser.appendDebugString("]");

			int input_idx = 0;
			for (auto& in_symbol_last_assign_index : func_hash_node.input_hash_nodes)
			{
				debug_traverser.appendDebugString("[InHashStr:");

				CHashNode& hash_node = tree_hash_nodes[in_symbol_last_assign_index];
				hash_node.debug_string;
				debug_traverser.appendDebugString(hash_node.debug_string);
				debug_traverser.appendDebugString("]");
			}

			debug_traverser.visitBinary(EvPostVisit, node);
#endif
			for (auto& out_symbol_hash : output_hash_values)
			{
				builder_context.symbol_last_hashnode_map[out_symbol_hash] = hash_value;
			}
		}
		else
		{
			assert_t(false);
		}
		
		
		return false;// controlled by custom hash tree builder
	}
	case EOpIndexDirectStruct:
	{
		if (visit == EvPreVisit)
		{
			const TTypeList* members = node->getLeft()->getType().getStruct();
			int member_index = node->getRight()->getAsConstantUnion()->getConstArray()[0].getIConst();
			const TString& index_direct_struct_str = (*members)[member_index].type->getFieldName();

			TIntermSymbol* symbol_node = node->getLeft()->getAsSymbolNode();
			const TType& type = symbol_node->getType();
			TString block_hash_string = getTypeText(type);

			TString struct_string = block_hash_string;
			struct_string.append(symbol_node->getName());

			TString member_string = struct_string;
			member_string.append(index_direct_struct_str);

			XXH64_hash_t struct_hash_value = XXH64(struct_string.data(), struct_string.size(), global_seed);
			XXH64_hash_t member_hash_value = XXH64(member_string.data(), member_string.size(), global_seed);
			builder_context.addUniqueHashValue(struct_hash_value);
			builder_context.addUniqueHashValue(member_hash_value);

			hash_string.append(std::to_string(XXH64_hash_t(struct_hash_value)).c_str());
			hash_string.append(std::string("_"));
			hash_string.append(std::to_string(XXH64_hash_t(member_hash_value)).c_str());
			hash_string.append(std::string("_"));

			XXH64_hash_t index_direct_struct_hash = XXH64(hash_string.data(), hash_string.size(), global_seed);
			hash_value_stack.push_back(index_direct_struct_hash);
#if TANGRAM_DEBUG
			debug_traverser.visitBinary(EvPreVisit, node);
			if (node->getLeft()) { node->getLeft()->traverse(&debug_traverser); }
			debug_traverser.visitBinary(EvInVisit, node);
			if (node->getRight()) { node->getRight()->traverse(&debug_traverser); }
			debug_traverser.visitBinary(EvPostVisit, node);
#endif
			return false;
		}
		break;
	}
	default:
	{
#if TANGRAM_DEBUG
		debug_traverser.visitBinary(visit, node);
#endif
		if (visit == EvPostVisit)
		{
			XXH64_hash_t hash_value_right = hash_value_stack.back();
			hash_value_stack.pop_back();

			XXH64_hash_t hash_value_left = hash_value_stack.back();
			hash_value_stack.pop_back();

			hash_string.append(std::to_string(XXH64_hash_t(hash_value_left)).c_str());
			hash_string.append(std::string("_"));
			hash_string.append(std::to_string(XXH64_hash_t(hash_value_right)).c_str());
			hash_string.append(std::string("_"));
			XXH64_hash_t hash_value = XXH64(hash_string.data(), hash_string.size(), global_seed);

			hash_value_stack.push_back(hash_value);
		}
	}
	};

	return true;
}

bool CASTHashTreeBuilder::visitUnary(TVisit visit, TIntermUnary* node)
{
#if TANGRAM_DEBUG
	debug_traverser.visitUnary(visit, node);
	if (visit == EvPreVisit) { debug_traverser.incrementDepth(node); }
	if (visit == EvPostVisit) { debug_traverser.decrementDepth(); }
#endif
	return true;
}

bool CASTHashTreeBuilder::visitAggregate(TVisit visit, TIntermAggregate* node)
{
#if TANGRAM_DEBUG
	debug_traverser.visitAggregate(visit, node);
	if (visit == EvPreVisit) { debug_traverser.incrementDepth(node); }
	if (visit == EvPostVisit) { debug_traverser.decrementDepth(); }
#endif
	return true;
}

bool CASTHashTreeBuilder::visitSelection(TVisit, TIntermSelection* node)
{
	assert_t(false);
	return true;
}

bool CASTHashTreeBuilder::constUnionBegin(const TIntermConstantUnion* const_untion, TBasicType basic_type, TString& inoutStr)
{
	bool is_subvector_scalar = false;
	if (const_untion)
	{
		int array_size = const_untion->getConstArray().size();
		if (array_size > 1)
		{
			switch (basic_type)
			{
			case EbtDouble:
				inoutStr.append("d");
				break;
			case EbtInt:
				inoutStr.append("i");
				break;
			case EbtUint:
				inoutStr.append("u");
				break;
			case EbtBool:
				inoutStr.append("b");
				break;
			case EbtFloat:
			default:
				break;
			};
			inoutStr.append("vec");
			inoutStr.append(std::to_string(array_size).c_str());
			inoutStr.append("(");
			is_subvector_scalar = true;
		}
	}
	return is_subvector_scalar;
}

void CASTHashTreeBuilder::constUnionEnd(const TIntermConstantUnion* const_untion, TString& inoutStr)
{
	if (const_untion)
	{
		int array_size = const_untion->getConstArray().size();
		if (array_size > 1)
		{
			inoutStr.append(")");
		}
	}
}



TString CASTHashTreeBuilder::generateConstantUnionStr(const TIntermConstantUnion* node, const TConstUnionArray& constUnion)
{
	TString constUnionStr;

	int size = node->getType().computeNumComponents();

	bool is_construct_vector = false;
	bool is_construct_matrix = false;
	bool is_subvector_scalar = false;
	bool is_vector_swizzle = false;

	if (getParentNode()->getAsAggregate())
	{
		TIntermAggregate* parent_node = getParentNode()->getAsAggregate();
		TOperator node_operator = parent_node->getOp();
		switch (node_operator)
		{
		case EOpMin:
		case EOpMax:
		case EOpClamp:
		case EOpMix:
		case EOpStep:
		case EOpDistance:
		case EOpDot:
		case EOpCross:

		case EOpLessThan:
		case EOpGreaterThan:
		case EOpLessThanEqual:
		case EOpGreaterThanEqual:
		case EOpVectorEqual:
		case EOpVectorNotEqual:

		case EOpMod:
		case EOpModf:
		case EOpPow:

		case EOpConstructMat2x2:
		case EOpConstructMat2x3:
		case EOpConstructMat2x4:
		case EOpConstructMat3x2:
		case EOpConstructMat3x3:
		case EOpConstructMat3x4:
		case EOpConstructMat4x2:
		case EOpConstructMat4x3:
		case EOpConstructMat4x4:

		case  EOpTexture:
		case  EOpTextureProj:
		case  EOpTextureLod:
		case  EOpTextureOffset:
		case  EOpTextureFetch:
		case  EOpTextureFetchOffset:
		case  EOpTextureProjOffset:
		case  EOpTextureLodOffset:
		case  EOpTextureProjLod:
		case  EOpTextureProjLodOffset:
		case  EOpTextureGrad:
		case  EOpTextureGradOffset:
		case  EOpTextureProjGrad:
		case  EOpTextureProjGradOffset:
		case  EOpTextureGather:
		case  EOpTextureGatherOffset:
		case  EOpTextureGatherOffsets:
		case  EOpTextureClamp:
		case  EOpTextureOffsetClamp:
		case  EOpTextureGradClamp:
		case  EOpTextureGradOffsetClamp:
		case  EOpTextureGatherLod:
		case  EOpTextureGatherLodOffset:
		case  EOpTextureGatherLodOffsets:

		case EOpAny:
		case EOpAll:
		{
			is_construct_vector = true;
			break;
		}
		default:
		{
			break;
		}
		}
	}

	if (getParentNode()->getAsBinaryNode())
	{
		TIntermBinary* parent_node = getParentNode()->getAsBinaryNode();
		TOperator node_operator = parent_node->getOp();
		switch (node_operator)
		{
		case EOpAssign:

		case  EOpAdd:
		case  EOpSub:
		case  EOpMul:
		case  EOpDiv:
		case  EOpMod:
		case  EOpRightShift:
		case  EOpLeftShift:
		case  EOpAnd:
		case  EOpInclusiveOr:
		case  EOpExclusiveOr:
		case  EOpEqual:
		case  EOpNotEqual:
		case  EOpVectorEqual:
		case  EOpVectorNotEqual:
		case  EOpLessThan:
		case  EOpGreaterThan:
		case  EOpLessThanEqual:
		case  EOpGreaterThanEqual:
		case  EOpComma:

		case  EOpVectorTimesScalar:
		case  EOpVectorTimesMatrix:

		case  EOpLogicalOr:
		case  EOpLogicalXor:
		case  EOpLogicalAnd:
		{
			is_construct_vector = true;
			break;
		}

		case  EOpMatrixTimesVector:
		case  EOpMatrixTimesScalar:
		{
			is_construct_vector = true;
			if (node->getMatrixCols() > 1 && node->getMatrixRows() > 1)
			{
				is_construct_matrix = true;
			}
			break;
		}
		case EOpVectorSwizzle:
		{
			is_vector_swizzle = true;
			break;
		}

		default:
		{
			break;
		}
		}
	}

	if (is_construct_matrix)
	{
		int mat_row = node->getMatrixRows();
		int mat_col = node->getMatrixCols();

		int array_idx = 0;
		for (int idx_row = 0; idx_row < mat_row; idx_row++)
		{
			switch (node->getBasicType())
			{
			case EbtDouble:
				constUnionStr.append("d");
				break;
			case EbtInt:
				constUnionStr.append("i");
				break;
			case EbtUint:
				constUnionStr.append("u");
				break;
			case EbtBool:
				constUnionStr.append("b");
				break;
			case EbtFloat:
			default:
				break;
			};

			constUnionStr.append("vec");
			constUnionStr.append(std::to_string(mat_col).c_str());
			constUnionStr.append("(");
			for (int idx_col = 0; idx_col < mat_col; idx_col++)
			{
				TBasicType const_type = constUnion[array_idx].getType();
				switch (const_type)
				{
				case EbtDouble:
				{
					constUnionStr.append(OutputDouble(constUnion[array_idx].getDConst()));
					break;
				}
				default:
				{
					assert_t(false);
					break;
				}
				}

				if (idx_col != (mat_col - 1))
				{
					constUnionStr.append(",");
				}
			}
			constUnionStr.append(")");
			if (idx_row != (mat_row - 1))
			{
				constUnionStr.append(",");
			}
			array_idx++;
		}
		return constUnionStr;
	}

	if (is_construct_vector)
	{
		is_subvector_scalar = constUnionBegin(node, node->getBasicType(), constUnionStr);
	}

	bool is_all_components_same = true;
	for (int i = 1; i < size; i++)
	{
		TBasicType const_type = constUnion[i].getType();
		switch (const_type)
		{
		case EbtInt: {if (constUnion[i].getIConst() != constUnion[i - 1].getIConst()) { is_all_components_same = false; } break; }
		case EbtDouble: {if (constUnion[i].getDConst() != constUnion[i - 1].getDConst()) { is_all_components_same = false; } break; }
		case EbtUint: {if (constUnion[i].getUConst() != constUnion[i - 1].getUConst()) { is_all_components_same = false; } break; }
		default:
		{
			assert_t(false);
			break;
		}
		};
	};

	if (is_all_components_same)
	{
		size = 1;
	}

	for (int i = 0; i < size; i++)
	{
		TBasicType const_type = constUnion[i].getType();
		switch (const_type)
		{
		case EbtInt:
		{
			if (is_vector_swizzle)
			{
				constUnionStr.insert(constUnionStr.end(), unionConvertToChar(constUnion[i].getIConst()));
			}
			else
			{
				constUnionStr.append(std::to_string(constUnion[i].getIConst()).c_str());
			}
			break;
		}
		case EbtDouble:
		{
			if (constUnion[i].getDConst() < 0)
			{
				//todo: fix me
				constUnionStr.append("(");
			}
			constUnionStr.append(OutputDouble(constUnion[i].getDConst()));
			if (constUnion[i].getDConst() < 0)
			{
				constUnionStr.append(")");
			}
			break;
		}
		case EbtUint:
		{
			constUnionStr.append(std::to_string(constUnion[i].getUConst()).c_str());
			constUnionStr.append("u");
			break;
		}
		case EbtBool:
		{
			if (constUnion[i].getBConst()) { constUnionStr.append("true"); }
			else { constUnionStr.append("false"); }
			break;
		}

		default:
		{
			assert_t(false);
			break;
		}
		}

		if (is_subvector_scalar && (i != (size - 1)))
		{
			constUnionStr.append(",");
		}
	}

	if (is_construct_vector)
	{
		constUnionEnd(node, constUnionStr);
	}

	return constUnionStr;
}

void CASTHashTreeBuilder::visitConstantUnion(TIntermConstantUnion* node)
{
#if TANGRAM_DEBUG
	debug_traverser.visitConstantUnion(node);
#endif
	TString constUnionStr = generateConstantUnionStr(node, node->getConstArray());
	XXH64_hash_t hash_value = XXH64(constUnionStr.data(), constUnionStr.size(), global_seed);
	hash_value_stack.push_back(hash_value);
}

// symbol hash
// struct linker object:
//		struct layout
//		struct name
// other linker obecjt
//		layout
// symbol hash:
//		symbol type hash
//		symbol index hash

void CASTHashTreeBuilder::visitSymbol(TIntermSymbol* node)
{
#if TANGRAM_DEBUG
	debug_traverser.visitSymbol(node);
#endif
	bool is_declared = false;

	{
		long long symbol_id = node->getId();
		auto iter = declared_symbols_id.find(symbol_id);
		if (iter == declared_symbols_id.end())
		{
			declared_symbols_id.insert(symbol_id);
		}
		else
		{
			is_declared = true;
		}
	}

	const TString& symbol_name = node->getName();
	if ((symbol_name[0] == 'g') && (symbol_name[1] == 'l') && (symbol_name[2] == '_'))
	{
		XXH64_hash_t hash_value = XXH64(symbol_name.data(), symbol_name.size(), global_seed);
		hash_value_stack.push_back(hash_value);
	}

	if (is_declared == false)
	{

		const TType& type = node->getType();
		TString hash_string = getTypeText(type);

		TBasicType basic_type = type.getBasicType();

		if (basic_type == EbtBlock)// uniform buffer linker object
		{
			hash_string.append(node->getName());
			assert_t(type.isStruct() && type.getStruct());

			{
				XXH64_hash_t hash_value = XXH64(hash_string.data(), hash_string.size(), global_seed);
				CHashNode linker_node;
				linker_node.hash_value = hash_value;
#if TANGRAM_DEBUG
				linker_node.debug_string = hash_string;
#endif
				tree_hash_nodes.push_back(linker_node);
				hash_value_to_idx[hash_value] = tree_hash_nodes.size() - 1;
			}

			const TTypeList* structure = type.getStruct();
			for (size_t i = 0; i < structure->size(); ++i)
			{
				TString mem_name = hash_string;
				TType* struct_mem_type = (*structure)[i].type;
				mem_name.append(struct_mem_type->getFieldName().c_str());
				XXH64_hash_t hash_value = XXH64(mem_name.data(), mem_name.size(), global_seed);

				CHashNode linker_node;
				linker_node.hash_value = hash_value;
#if TANGRAM_DEBUG
				linker_node.debug_string = mem_name;
#endif
				tree_hash_nodes.push_back(linker_node);
				hash_value_to_idx[hash_value] = tree_hash_nodes.size() - 1;
			}
		}
		else if (type.getQualifier().hasLayout() && (basic_type != EbtBlock))//other linker object
		{
			XXH64_hash_t hash_value = XXH64(hash_string.data(), hash_string.size(), global_seed);
			CHashNode linker_node;
			linker_node.hash_value = hash_value;
#if TANGRAM_DEBUG
			linker_node.debug_string = hash_string;
#endif
			tree_hash_nodes.push_back(linker_node);
			hash_value_to_idx[hash_value] = tree_hash_nodes.size() - 1;
		}
		else //temp variable
		{
			assert_t(builder_context.op_assign_visit_output_symbols == true);

			XXH64_hash_t name_hash = XXH64(node->getName().data(), node->getName().size(), global_seed);
			
			// in scope hash: symbol type + symbol index
			{
				uint32_t symbol_index = scope_symbol_traverser.getSymbolIndex(name_hash);
				hash_string.append(TString("_"));
				hash_string.append(std::to_string(symbol_index));
				XXH64_hash_t in_scope_hash_value = XXH64(hash_string.data(), hash_string.size(), global_seed);
				hash_value_stack.push_back(in_scope_hash_value);
			}

			// out scope hash:  symbol name
			{
				XXH64_hash_t out_scope_hash = name_hash;

				CHashNode symbol_hash_node;
				symbol_hash_node.hash_value = out_scope_hash;
#if TANGRAM_DEBUG
				symbol_hash_node.debug_string = hash_string;
#endif
				tree_hash_nodes.push_back(symbol_hash_node);
				hash_value_to_idx[out_scope_hash] = tree_hash_nodes.size() - 1;
				builder_context.addUniqueHashValue(out_scope_hash);
			}
		}
	}
	else
	{
		const TType& type = node->getType();
		TString hash_string = getTypeText(type);

		if (!type.getQualifier().hasLayout())
		{
			const XXH64_hash_t name_hash = XXH64(node->getName().data(), node->getName().size(), global_seed);
			uint32_t symbol_index = scope_symbol_traverser.getSymbolIndex(name_hash);

			hash_string.append(TString("_"));
			hash_string.append(std::to_string(symbol_index));

			const XXH64_hash_t out_scope_hash = name_hash;
			builder_context.addUniqueHashValue(out_scope_hash);
		}
		
		XXH64_hash_t in_scope_hash_value = XXH64(hash_string.data(), hash_string.size(), global_seed);
		hash_value_stack.push_back(in_scope_hash_value);
		
		if (type.getQualifier().hasLayout())
		{
			const XXH64_hash_t out_scope_hash = in_scope_hash_value;
			builder_context.addUniqueHashValue(out_scope_hash);
		}
	}

	if (!node->getConstArray().empty() && (is_declared == false))
	{
		assert_t(false);
	}
	else if (node->getConstSubtree())
	{
		assert_t(false);
	}
}

bool CASTHashTreeBuilder::visitLoop(TVisit, TIntermLoop* node)
{
	assert_t(false);
	return true;
}

bool CASTHashTreeBuilder::visitBranch(TVisit, TIntermBranch* node)
{
	assert_t(false);
	return true;
}

bool CASTHashTreeBuilder::visitSwitch(TVisit, TIntermSwitch* node)
{
	assert_t(false);
	return true;
}

void initAstToHashTree()
{
	InitializeProcess();
}

void finalizeAstToHashTree()
{
	FinalizeProcess();
}

bool ast_to_hash_treel(const char* const* shaderStrings, const int* shaderLengths)
{
	int shader_size = *shaderLengths;
	int sharp_pos = 0;
	for (int idx = 0; idx < shader_size; idx++)
	{
		if (((const char*)shaderStrings)[idx] == '#')
		{
			if (((const char*)shaderStrings)[idx + 1] == 'v')
			{
				sharp_pos = idx;
				break;
			}
		}
	}

	int main_size = shader_size - sharp_pos;
	if (main_size <= 0)
	{
		return false;
	}

	const int defaultVersion = 100;
	const EProfile defaultProfile = ENoProfile;
	const bool forceVersionProfile = false;
	const bool isForwardCompatible = false;

	EShMessages messages = EShMsgCascadingErrors;
	messages = static_cast<EShMessages>(messages | EShMsgAST);
	messages = static_cast<EShMessages>(messages | EShMsgHlslLegalization);

	std::string src_code((const char*)shaderStrings + sharp_pos, main_size);
	{
		glslang::TShader shader(EShLangFragment);
		{
			const char* shader_strings = src_code.data();
			const int shader_lengths = static_cast<int>(src_code.size());
			shader.setStringsWithLengths(&shader_strings, &shader_lengths, 1);
			shader.setEntryPoint("main");
			shader.parse(GetDefaultResources(), defaultVersion, isForwardCompatible, messages);
		}

		TIntermediate* intermediate = shader.getIntermediate();
		TInvalidShaderTraverser invalid_traverser;
		intermediate->getTreeRoot()->traverse(&invalid_traverser);
		if (!invalid_traverser.getIsValidShader()) { return false; }

		TIntermAggregate* root_aggregate = intermediate->getTreeRoot()->getAsAggregate();
		if (root_aggregate != nullptr)
		{
			// tranverse linker object at first
			TIntermSequence& sequence = root_aggregate->getSequence();
			TIntermSequence sequnce_reverse;
			for (TIntermSequence::reverse_iterator sit = sequence.rbegin(); sit != sequence.rend(); sit++)
			{
				TString skip_str("compiler_internal_Adjust");
				if ((*sit)->getAsAggregate())
				{
					TString func_name = (*sit)->getAsAggregate()->getName();
					if ((func_name.size() > skip_str.size()) && (func_name.substr(0, skip_str.size()) == skip_str))
					{
						return false;
					}
				}

				sequnce_reverse.push_back(*sit);
			}
			sequence = sequnce_reverse;
		}
		else
		{
			assert_t(false);
		}

		CASTHashTreeBuilder hash_tree_builder;
		intermediate->getTreeRoot()->traverse(&hash_tree_builder);

	}
	return false;
}


