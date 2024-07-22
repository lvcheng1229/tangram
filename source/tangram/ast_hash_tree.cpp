#include "ast_hash_tree.h"
#include "shader_network.h"


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

TString CASTHashTreeBuilder::getTypeText(const TType& type, bool getQualifiers, bool getSymbolName, bool getPrecision, bool getLayoutLocation)
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
			if (qualifier.hasAnyLocation() && getLayoutLocation)
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

	if ((!is_vec) && (!is_blk) && (!is_mat))
	{
		appendStr(type.getBasicTypeString().c_str());
	}

	return type_string;
}

// binary hash
// 2_operator_lefthash_righthash

void CASTHashTreeBuilder::preTranverse(TIntermediate* intermediate)
{
	intermediate->getTreeRoot()->traverse(&symbol_scope_traverser);
	debug_traverser.preTranverse(intermediate);
}

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
		if (builder_context.is_second_level_function == true)
		{
#if TANGRAM_DEBUG
			debug_traverser.visitBinary(visit, node);
			increAndDecrePath(visit, node);
#endif

			if (visit == EvPreVisit)
			{
				builder_context.no_assign_context.visit_assigned_symbols = true;
			}
			else if (visit == EvInVisit)
			{
				builder_context.no_assign_context.visit_assigned_symbols = false;
			}
		}
		else if (builder_context.is_second_level_function == false)
		{
			if (visit == EvPreVisit)
			{
#if TANGRAM_DEBUG
				debug_traverser.visitBinary(EvPreVisit, node);
				debug_traverser.incrementDepth(node);
#endif
				// find all of the symbols in the scope
				builder_context.scopeReset();
				scope_symbol_traverser.reset();
				hash_value_stack_max_depth = 0;

				node->getLeft()->traverse(&scope_symbol_traverser);
				node->getRight()->traverse(&scope_symbol_traverser);

				builder_context.is_second_level_function = true;

				// output symbols
				if (node->getLeft())
				{
					builder_context.op_assign_context.visit_output_symbols = true;
					
					node->getLeft()->traverse(this);
					builder_context.op_assign_context.visit_output_symbols = false;
					
				}

#if TANGRAM_DEBUG
				debug_traverser.visitBinary(EvInVisit, node);
#endif

				// input symbols
				if (node->getRight())
				{
					builder_context.op_assign_context.visit_input_symbols = true;
					builder_context.no_assign_context.visit_input_symbols = true;
					node->getRight()->traverse(this);
					builder_context.op_assign_context.visit_input_symbols = false;
					builder_context.no_assign_context.visit_input_symbols = false;
				}

				builder_context.is_second_level_function = false;

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
				func_hash_node.weight = hash_value_stack_max_depth;
#if TANGRAM_DEBUG
				func_hash_node.debug_string = hash_string;
#endif
				getAndUpdateInputHashNodes(func_hash_node);

				{
					tree_hash_nodes.push_back(func_hash_node);
					hash_value_to_idx[hash_value] = tree_hash_nodes.size() - 1;
					hash_value_stack.push_back(hash_value);
				}


#if TANGRAM_DEBUG
				outputDebugString(func_hash_node);
				const std::string max_depth_str = std::string("[Max Depth:") + std::to_string(hash_value_stack_max_depth) + std::string("]");
				debug_traverser.appendDebugString(max_depth_str.c_str());
				debug_traverser.decrementDepth();
				debug_traverser.visitBinary(EvPostVisit, node);
#endif
				updateLastAssignHashmap(func_hash_node);
			}
			else
			{
				assert_t(false);
			}

			return false;// controlled by custom hash tree builder
		}
		break;
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
			
			builder_context.addUniqueHashValue(member_hash_value, symbol_node->getName());

			hash_string.append(std::to_string(XXH64_hash_t(struct_hash_value)).c_str());
			hash_string.append(std::string("_"));
			hash_string.append(std::to_string(XXH64_hash_t(member_hash_value)).c_str());
			hash_string.append(std::string("_"));

			XXH64_hash_t index_direct_struct_hash = XXH64(hash_string.data(), hash_string.size(), global_seed);
			hash_value_stack.push_back(index_direct_struct_hash);
			hash_value_stack_max_depth += 3;
#if TANGRAM_DEBUG
			debug_traverser.visitBinary(EvPreVisit, node);
			if (visit == EvPreVisit) { debug_traverser.incrementDepth(node); }
			if (node->getLeft()) { node->getLeft()->traverse(&debug_traverser); }
			debug_traverser.visitBinary(EvInVisit, node);
			if (node->getRight()) { node->getRight()->traverse(&debug_traverser); }
			if (visit == EvPostVisit) { debug_traverser.decrementDepth(); }
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
		increAndDecrePath(visit, node);
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
			hash_value_stack_max_depth++;
		}
	}
	};

	return true;
}

bool CASTHashTreeBuilder::visitUnary(TVisit visit, TIntermUnary* node)
{
#if TANGRAM_DEBUG
	debug_traverser.visitUnary(visit, node);
	increAndDecrePath(visit, node);
#endif

	TOperator node_operator = node->getOp();
	if (visit == EvPreVisit)
	{
		switch (node_operator)
		{
		case EOpPostIncrement:
		case EOpPostDecrement:
		case EOpPreIncrement:
		case EOpPreDecrement:
			builder_context.no_assign_context.visit_assigned_symbols = true;
			break;
		}
	}

	if (visit == EvPostVisit)
	{
		switch (node_operator)
		{
		case EOpPostIncrement:
		case EOpPostDecrement:
		case EOpPreIncrement:
		case EOpPreDecrement:
			builder_context.no_assign_context.visit_assigned_symbols = false;
			break;
		}
	}
	
	if (visit == EvPostVisit)
	{
		TString hash_string;
		hash_string.reserve(10 + 1 * 8);
		hash_string.append(std::string("1_"));
		hash_string.append(std::to_string(uint32_t(node_operator)).c_str());
		hash_string.append(std::string("_"));

		XXH64_hash_t sub_hash_value = hash_value_stack.back();
		hash_value_stack.pop_back();

		hash_string.append(std::to_string(XXH64_hash_t(sub_hash_value)).c_str());
		hash_string.append(std::string("_"));

		XXH64_hash_t hash_value = XXH64(hash_string.data(), hash_string.size(), global_seed);
		hash_value_stack.push_back(hash_value);
		hash_value_stack_max_depth++;
	}

	return true;
}

bool CASTHashTreeBuilder::visitAggregate(TVisit visit, TIntermAggregate* node)
{
#if TANGRAM_DEBUG
	
	debug_traverser.visitAggregate(visit, node);
	increAndDecrePath(visit, node);
#endif
	TOperator node_operator = node->getOp();
	switch (node_operator)
	{
	case EOpLinkerObjects:
	{
		if (visit == EvPreVisit)
		{
			builder_context.is_iterate_linker_objects = true;
		}
		else if (visit == EvPostVisit)
		{
			builder_context.is_iterate_linker_objects = false;
		}
		break;
	}
	default:
	{

	}
	}

	if (visit == EvPostVisit && node_operator != EOpLinkerObjects && node_operator != EOpNull)
	{
		TString hash_string;
		hash_string.reserve(10 + node->getSequence().size() * 8);
		hash_string.append(std::to_string(node->getSequence().size()).c_str());
		hash_string.append(std::string("_"));
		hash_string.append(std::to_string(uint32_t(node_operator)).c_str());
		hash_string.append(std::string("_"));

		for (TIntermSequence::iterator sit = node->getSequence().begin(); sit != node->getSequence().end(); sit++)
		{
			XXH64_hash_t sub_hash_value = hash_value_stack.back();
			hash_value_stack.pop_back();

			hash_string.append(std::to_string(XXH64_hash_t(sub_hash_value)).c_str());
			hash_string.append(std::string("_"));
		}

		XXH64_hash_t hash_value = XXH64(hash_string.data(), hash_string.size(), global_seed);
		hash_value_stack.push_back(hash_value);
		hash_value_stack_max_depth++;
	}

	return true;
}

XXH64_hash_t CASTHashTreeBuilder::generateSelectionHashValue(TString& hash_string, TIntermSelection* node)
{
	bool is_ternnary = false;
	TIntermNode* true_block = node->getTrueBlock();
	TIntermNode* false_block = node->getFalseBlock();

	int operator_num = 0;
	if (true_block != nullptr)
	{
		operator_num++;
	}

	if (false_block != nullptr)
	{
		operator_num++;
	}

	hash_string.append(std::to_string(operator_num).c_str());
	hash_string.append(std::string("_Selection_"));

	if ((true_block != nullptr) && (false_block != nullptr))
	{
		int true_block_line = true_block->getLoc().line;
		int false_block_line = false_block->getLoc().line;
		if (abs(true_block_line - false_block_line) <= 1)
		{
			is_ternnary = true;
		}
	}

	if (is_ternnary)
	{
		hash_string.append(std::string("Ternnary_"));
	}
	else
	{
		hash_string.append(std::string("If_"));
	}

	if (false_block)
	{
		XXH64_hash_t false_blk_hash = hash_value_stack.back();
		hash_value_stack.pop_back();
		hash_string.append(std::to_string(XXH64_hash_t(false_blk_hash)).c_str());
		hash_string.append(std::string("_"));
	}

	if (true_block)
	{
		XXH64_hash_t true_blk_hash = hash_value_stack.back();
		hash_value_stack.pop_back();
		hash_string.append(std::to_string(XXH64_hash_t(true_blk_hash)).c_str());
		hash_string.append(std::string("_"));
	}

	XXH64_hash_t cond_blk_hash = hash_value_stack.back();
	hash_value_stack.pop_back();

	hash_string.append(std::to_string(XXH64_hash_t(cond_blk_hash)).c_str());
	XXH64_hash_t hash_value = XXH64(hash_string.data(), hash_string.size(), global_seed);
	return hash_value;
}

bool CASTHashTreeBuilder::visitSelection(TVisit visit, TIntermSelection* node)
{
	if (builder_context.is_second_level_function == false)
	{
		subscope_tranverser.resetSubScopeMinMaxLine();
		subscope_tranverser.visitSelection(EvPreVisit, node);

#if TANGRAM_DEBUG
		if (visit == EvPreVisit)
		{
			bool ret_result = debug_traverser.visitSelection(EvPreVisit, node);
			assert_t(ret_result == false);
		}
		debug_traverser.visit_state.DisableAllVisitState();
#endif

		builder_context.is_second_level_function = true;

		// find all of the symbols in the scope
		builder_context.scopeReset();
		scope_symbol_traverser.reset();
		hash_value_stack_max_depth = 0;
		node->getCondition()->traverse(&scope_symbol_traverser);
		if (node->getTrueBlock()) { node->getTrueBlock()->traverse(&scope_symbol_traverser); }
		if (node->getFalseBlock()) { node->getFalseBlock()->traverse(&scope_symbol_traverser); }

		int scope_min_line = node->getLoc().line;

		// generate symbol inout map
		generateSymbolInoutMap(scope_min_line);

		node->getCondition()->traverse(this);
		if (node->getTrueBlock()) { node->getTrueBlock()->traverse(this); }
		if (node->getFalseBlock()) { node->getFalseBlock()->traverse(this); }

		builder_context.is_second_level_function = false;

		TString hash_string;
		XXH64_hash_t node_hash_value = generateSelectionHashValue(hash_string, node);
		generateHashNode(hash_string, node_hash_value);
		return false;
	}
	else
	{
#if TANGRAM_DEBUG
		if (visit == EvPreVisit)
		{
			bool ret_result = debug_traverser.visitSelection(visit, node);
			assert_t(ret_result == false);
		}
#endif

		if (visit == EvPostVisit)
		{
			TString hash_string;
			XXH64_hash_t hash_value = generateSelectionHashValue(hash_string, node);
			hash_value_stack.push_back(hash_value);
			hash_value_stack_max_depth++;
		}
	}
	
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

TString CASTHashTreeBuilder::getArraySize(const TType& type)
{
	TString array_size_str;
	if (type.isArray())
	{
		const TArraySizes* array_sizes = type.getArraySizes();
		for (int i = 0; i < (int)array_sizes->getNumDims(); ++i)
		{
			int size = array_sizes->getDimSize(i);
			array_size_str.append("[");
			array_size_str.append(std::to_string(array_sizes->getDimSize(i)).c_str());
			array_size_str.append("]");
		}
	}
	return array_size_str;
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

void CASTHashTreeBuilder::visitSymbol(TIntermSymbol* node)
{
#if TANGRAM_DEBUG
	debug_traverser.visitSymbol(node);
#endif
	bool is_declared = false;

#if TANGRAM_DEBUG
	if (node->getName() == "_69")
	{
		int debugVar = 1;
	}
#endif

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
		hash_value_stack_max_depth++;
	}

	if (is_declared == false)
	{
		const TType& type = node->getType();
		TString hash_string = getTypeText(type);

		TBasicType basic_type = type.getBasicType();

		if (builder_context.is_iterate_linker_objects)
		{
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
					debug_traverser.appendDebugString("[CHashStr:");
					debug_traverser.appendDebugString(hash_string);
					debug_traverser.appendDebugString("]");
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
					debug_traverser.appendDebugString("\n[CHashStr:");
					debug_traverser.appendDebugString(mem_name);
					debug_traverser.appendDebugString("]");
#endif
					tree_hash_nodes.push_back(linker_node);
					hash_value_to_idx[hash_value] = tree_hash_nodes.size() - 1;
				}
			}
			else if (basic_type == EbtSampler) //sample linker object
			{
				hash_string.append(node->getName());
				XXH64_hash_t hash_value = XXH64(hash_string.data(), hash_string.size(), global_seed);
				CHashNode linker_node;
				linker_node.hash_value = hash_value;
#if TANGRAM_DEBUG
				linker_node.debug_string = hash_string;
				debug_traverser.appendDebugString("[CHashStr:");
				debug_traverser.appendDebugString(hash_string);
				debug_traverser.appendDebugString("]");
#endif
				tree_hash_nodes.push_back(linker_node);
				hash_value_to_idx[hash_value] = tree_hash_nodes.size() - 1;
			}
			else if (type.getQualifier().hasLayout() && (basic_type != EbtBlock))//other linker object
			{
				XXH64_hash_t hash_value = XXH64(hash_string.data(), hash_string.size(), global_seed);
				CHashNode linker_node;
				linker_node.hash_value = hash_value;
#if TANGRAM_DEBUG
				linker_node.debug_string = hash_string;
				debug_traverser.appendDebugString("[CHashStr:");
				debug_traverser.appendDebugString(hash_string);
				debug_traverser.appendDebugString("]");
#endif
				tree_hash_nodes.push_back(linker_node);
				hash_value_to_idx[hash_value] = tree_hash_nodes.size() - 1;
			}
			else if (type.getQualifier().isUniform())
			{
				hash_string.append(node->getName());
				hash_string.append(getArraySize(type));
				XXH64_hash_t hash_value = XXH64(hash_string.data(), hash_string.size(), global_seed);

				CHashNode linker_node;
				linker_node.hash_value = hash_value;
#if TANGRAM_DEBUG
				linker_node.debug_string = hash_string;
				debug_traverser.appendDebugString("[CHashStr:");
				debug_traverser.appendDebugString(hash_string);
				debug_traverser.appendDebugString("]");
#endif
				tree_hash_nodes.push_back(linker_node);
				hash_value_to_idx[hash_value] = tree_hash_nodes.size() - 1;
			}
			else if(type.getQualifier().storage == TStorageQualifier::EvqGlobal)
			{
				// out scope hash:  symbol name
				{
					const std::string global_variable("global_variable");

					XXH64_hash_t out_scope_hash = XXH64(global_variable.c_str(), global_variable.length(), global_seed);

					CHashNode symbol_hash_node;
					symbol_hash_node.hash_value = out_scope_hash;
#if TANGRAM_DEBUG
					symbol_hash_node.debug_string = global_variable;
#endif
					tree_hash_nodes.push_back(symbol_hash_node);
					hash_value_to_idx[out_scope_hash] = tree_hash_nodes.size() - 1;
					builder_context.addUniqueHashValue(out_scope_hash, node->getName());
				}

#if TANGRAM_DEBUG
				printf("Global Symbol Name: %s\n", node->getName().c_str());
#endif
			}
			else
			{
				assert_t(false);
			}
		}
		else
		{
			assert_t(builder_context.op_assign_context.visit_output_symbols == true || builder_context.is_second_level_function == true);

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
				builder_context.addUniqueHashValue(out_scope_hash, node->getName());
			}
		}
	}
	else
	{
		const TType& type = node->getType();
		TString hash_string = getTypeText(type);
		hash_value_stack_max_depth++;

		if (type.getBasicType() == EbtSampler)
		{
			hash_string.append(node->getName());
			XXH64_hash_t in_scope_hash_value = XXH64(hash_string.data(), hash_string.size(), global_seed);
			hash_value_stack.push_back(in_scope_hash_value);
			const XXH64_hash_t out_scope_hash = in_scope_hash_value;
			builder_context.addUniqueHashValue(out_scope_hash, node->getName());
		}
		else if (type.getQualifier().hasLayout()) // layout( location=1)in highp vec4 -> layout() in highp vec4
		{
			XXH64_hash_t out_scope_hash = XXH64(hash_string.data(), hash_string.size(), global_seed);
			builder_context.addUniqueHashValue(out_scope_hash, node->getName());

			TString hash_string_wihtout_location = getTypeText(type, true, true, true, false);
			XXH64_hash_t in_scope_hash_value = XXH64(hash_string_wihtout_location.data(), hash_string_wihtout_location.size(), global_seed);
			hash_value_stack.push_back(in_scope_hash_value);
		}
		else if (type.getQualifier().isUniform())
		{
			hash_string.append(node->getName());
			hash_string.append(getArraySize(type));
			XXH64_hash_t in_scope_hash_value = XXH64(hash_string.data(), hash_string.size(), global_seed);
			hash_value_stack.push_back(in_scope_hash_value);
			const XXH64_hash_t out_scope_hash = in_scope_hash_value;
			builder_context.addUniqueHashValue(out_scope_hash, node->getName());
		}
		else if (type.getQualifier().storage == TStorageQualifier::EvqGlobal)
		{
			const std::string global_variable("global_variable");

			XXH64_hash_t name_hash = XXH64(global_variable.c_str(), global_variable.length(), global_seed);

			// in scope hash
			hash_value_stack.push_back(name_hash);

			// out scope hash
			builder_context.addUniqueHashValue(name_hash, node->getName());
		}
		else
		{
			const XXH64_hash_t name_hash = XXH64(node->getName().data(), node->getName().size(), global_seed);
			uint32_t symbol_index = scope_symbol_traverser.getSymbolIndex(name_hash);

			hash_string.append(TString("_"));
			hash_string.append(std::to_string(symbol_index));
			XXH64_hash_t in_scope_hash_value = XXH64(hash_string.data(), hash_string.size(), global_seed);
			hash_value_stack.push_back(in_scope_hash_value);

			const XXH64_hash_t out_scope_hash = name_hash;
			builder_context.addUniqueHashValue(out_scope_hash, node->getName());
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

XXH64_hash_t CASTHashTreeBuilder::generateLoopHashValue(TString& hash_string, TIntermLoop* node)
{
	int operator_num = 0;
	if (node->getTest())
	{
		operator_num++;
	}
	
	if (node->getTerminal())
	{
		operator_num++;
	}

	if (node->getBody())
	{
		operator_num++;
	}

	hash_string.append(std::to_string(operator_num).c_str());
	hash_string.append(std::string("_Selection_"));

	bool is_do_while = (!node->testFirst());

	if (is_do_while == false)
	{
		hash_string.append(std::string("For_"));
		
		if (node->getBody())
		{
			XXH64_hash_t sub_hash_value = hash_value_stack.back();
			hash_value_stack.pop_back();
			hash_string.append(std::to_string(XXH64_hash_t(sub_hash_value)).c_str());
			hash_string.append(std::string("_"));
		}

		if (node->getTerminal())
		{
			XXH64_hash_t sub_hash_value = hash_value_stack.back();
			hash_value_stack.pop_back();
			hash_string.append(std::to_string(XXH64_hash_t(sub_hash_value)).c_str());
			hash_string.append(std::string("_"));
		}

		if (node->getTest())
		{
			XXH64_hash_t sub_hash_value = hash_value_stack.back();
			hash_value_stack.pop_back();
			hash_string.append(std::to_string(XXH64_hash_t(sub_hash_value)).c_str());
			hash_string.append(std::string("_"));
		}

		XXH64_hash_t hash_value = XXH64(hash_string.data(), hash_string.size(), global_seed);
		return hash_value;
	}
	else
	{
		hash_string.append(std::string("DoWhile_"));

		if (node->getTest())
		{
			XXH64_hash_t sub_hash_value = hash_value_stack.back();
			hash_value_stack.pop_back();
			hash_string.append(std::to_string(XXH64_hash_t(sub_hash_value)).c_str());
			hash_string.append(std::string("_"));
		}

		if (node->getBody())
		{
			XXH64_hash_t sub_hash_value = hash_value_stack.back();
			hash_value_stack.pop_back();
			hash_string.append(std::to_string(XXH64_hash_t(sub_hash_value)).c_str());
			hash_string.append(std::string("_"));
		}

		XXH64_hash_t hash_value = XXH64(hash_string.data(), hash_string.size(), global_seed);
		return hash_value;
	}

	assert_t(false);
	return XXH64_hash_t();
}

bool CASTHashTreeBuilder::visitLoop(TVisit visit, TIntermLoop* node)
{
	if (builder_context.is_second_level_function == false)
	{
		subscope_tranverser.resetSubScopeMinMaxLine();
		subscope_tranverser.visitLoop(EvPreVisit, node);

#if TANGRAM_DEBUG
		if (visit == EvPreVisit)
		{
			bool ret_result = debug_traverser.visitLoop(EvPreVisit, node);
			assert_t(ret_result == false);
		}
		debug_traverser.visit_state.DisableAllVisitState();
#endif

		builder_context.is_second_level_function = true;

		bool is_do_while = (!node->testFirst());

		// find all of the symbols in the scope
		builder_context.scopeReset();
		scope_symbol_traverser.reset();

		if (is_do_while == false)
		{
			if (node->getTest())		{ node->getTest()->traverse(&scope_symbol_traverser); }
			if (node->getTerminal()) { node->getTerminal()->traverse(&scope_symbol_traverser); };
			if (node->getBody())	{ node->getBody()->traverse(&scope_symbol_traverser); }
		}
		else
		{
			if (node->getBody()) { node->getBody()->traverse(&scope_symbol_traverser); }
			if (node->getTest()) { node->getTest()->traverse(&scope_symbol_traverser); }
		}

		int scope_min_line = node->getLoc().line;

		// generate symbol inout map
		generateSymbolInoutMap(scope_min_line);

		if (is_do_while == false)
		{
			if (node->getTest()) { node->getTest()->traverse(this); }
			if (node->getTerminal()) { node->getTerminal()->traverse(this); };
			if (node->getBody()) { node->getBody()->traverse(this); }
		}
		else
		{
			if (node->getBody()) { node->getBody()->traverse(this); }
			if (node->getTest()) { node->getTest()->traverse(this); }
		}

		builder_context.is_second_level_function = false;

		TString hash_string;
		XXH64_hash_t node_hash_value = generateLoopHashValue(hash_string, node);
		generateHashNode(hash_string, node_hash_value);
		return false;
	}
	else
	{
#if TANGRAM_DEBUG
		if (visit == EvPreVisit)
		{
			debug_traverser.visitLoop(visit, node);
		}
#endif
		if (visit == EvPostVisit)
		{
			TString hash_string;
			XXH64_hash_t node_hash_value = generateLoopHashValue(hash_string, node);
			hash_value_stack.push_back(node_hash_value);
			hash_value_stack_max_depth++;
		}
	}
	
	return true;
}

XXH64_hash_t CASTHashTreeBuilder::generateBranchHashValue(TString& hash_string, TIntermBranch* node)
{
	TOperator node_operator = node->getFlowOp();
	hash_string.reserve(2 + 1 + 1);
	hash_string.append(std::string("2_"));
	hash_string.append(std::to_string(uint32_t(node_operator)).c_str());
	hash_string.append(std::string("_"));
	if (node->getExpression())
	{
		XXH64_hash_t expr_hash_value = hash_value_stack.back();
		hash_value_stack.pop_back();

		hash_string.append(std::to_string(XXH64_hash_t(expr_hash_value)).c_str());
	}

	XXH64_hash_t hash_value = XXH64(hash_string.data(), hash_string.size(), global_seed);
	return hash_value;
}

bool CASTHashTreeBuilder::visitBranch(TVisit visit, TIntermBranch* node)
{
	TOperator node_operator = node->getFlowOp();
	if (builder_context.is_second_level_function == false)
	{
		assert_t(false);
		return false;
	}
	else
	{
#if TANGRAM_DEBUG
		debug_traverser.visitBranch(visit, node);
		increAndDecrePath(visit, node);
#endif
		if (visit == EvPostVisit)
		{
			TString hash_string;
			XXH64_hash_t hash_value = generateBranchHashValue(hash_string, node);
			hash_value_stack.push_back(hash_value);
			hash_value_stack_max_depth++;
		}
	}
	
	return true;
}

XXH64_hash_t CASTHashTreeBuilder::generateSwitchHashValue(TString& hash_string)
{
	XXH64_hash_t body_hash_value = hash_value_stack.back();
	hash_value_stack.pop_back();

	XXH64_hash_t condition_hash_value = hash_value_stack.back();
	hash_value_stack.pop_back();
	
	hash_string.reserve(10 + 2 * 8);
	hash_string.append(std::string("2_Switch"));
	hash_string.append(std::string("_"));
	hash_string.append(std::to_string(XXH64_hash_t(condition_hash_value)).c_str());
	hash_string.append(std::string("_"));
	hash_string.append(std::to_string(XXH64_hash_t(body_hash_value)).c_str());

	XXH64_hash_t node_hash_value = XXH64(hash_string.data(), hash_string.size(), global_seed);
	return node_hash_value;
}

bool CASTHashTreeBuilder::visitSwitch(TVisit visit, TIntermSwitch* node)
{
	if (builder_context.is_second_level_function == false)
	{
		subscope_tranverser.resetSubScopeMinMaxLine();
		subscope_tranverser.visitSwitch(EvPreVisit, node);

#if TANGRAM_DEBUG
		if (visit == EvPreVisit)
		{
			bool ret_result = debug_traverser.visitSwitch(EvPreVisit, node);
			assert_t(ret_result == false);
		}
		debug_traverser.visit_state.DisableAllVisitState();
#endif

		builder_context.is_second_level_function = true;
		
		// find all of the symbols in the scope
		builder_context.scopeReset();
		scope_symbol_traverser.reset();
		node->getCondition()->traverse(&scope_symbol_traverser);
		node->getBody()->traverse(&scope_symbol_traverser);

		int scope_min_line = node->getLoc().line;

		// generate symbol inout map
		generateSymbolInoutMap(scope_min_line);

		node->getCondition()->traverse(this);
		node->getBody()->traverse(this);
		builder_context.is_second_level_function = false;

		TString hash_string;
		XXH64_hash_t node_hash_value = generateSwitchHashValue(hash_string);
		generateHashNode(hash_string, node_hash_value);
		return false;
	}
	else
	{
#if TANGRAM_DEBUG
		if (visit == EvPreVisit)
		{
			bool ret_result = debug_traverser.visitSwitch(visit, node);
			assert_t(ret_result == false);
		}
#endif
		if (visit == EvPostVisit)
		{
			TString hash_string;
			XXH64_hash_t hash_value = generateSwitchHashValue(hash_string);
			hash_value_stack.push_back(hash_value);
			hash_value_stack_max_depth++;
		}
		return true;
	}
	
	return true;
}

void CASTHashTreeBuilder::generateHashNode(const TString& hash_string, XXH64_hash_t node_hash_value)
{
	CHashNode func_hash_node;
	func_hash_node.hash_value = node_hash_value;
	func_hash_node.weight = hash_value_stack_max_depth;
#if TANGRAM_DEBUG
	func_hash_node.debug_string = hash_string;
#endif
	// get input hash values
	getAndUpdateInputHashNodes(func_hash_node);

	{
		tree_hash_nodes.push_back(func_hash_node);
		hash_value_to_idx[node_hash_value] = tree_hash_nodes.size() - 1;
		hash_value_stack.push_back(node_hash_value);
	}

#if TANGRAM_DEBUG
	outputDebugString(func_hash_node);
	const std::string max_depth_str = std::string("[Max Depth:") + std::to_string(hash_value_stack_max_depth) + std::string("]");
	debug_traverser.appendDebugString(max_depth_str.c_str());
	debug_traverser.appendDebugString("\n");
	debug_traverser.visit_state.EnableAllVisitState();
#endif

	updateLastAssignHashmap(func_hash_node);
}

void CASTHashTreeBuilder::generateSymbolInoutMap(int scope_min_line)
{
	int scope_max_line = subscope_tranverser.getSubScopeMaxLine();

	auto symbols_max_line_map = symbol_scope_traverser.getSymbolMaxLine();
	auto symbols_min_line_map = symbol_scope_traverser.getSymbolMinLine();

	for (auto& iter : subscope_tranverser.getSubScopeSymbols())
	{
		TIntermSymbol* symbol_node = iter.second;

		auto symbol_max_map_iter = symbols_max_line_map->find(symbol_node->getId());
		assert_t(symbol_max_map_iter != symbols_max_line_map->end());
		int symbol_max_line = symbol_max_map_iter->second;

		auto symbol_min_map_iter = symbols_min_line_map->find(symbol_node->getId());
		assert_t(symbol_min_map_iter != symbols_min_line_map->end());
		int symbol_min_line = symbol_min_map_iter->second;

		XXH32_hash_t symbol_name_inout_hash = XXH32(symbol_node->getName().c_str(), symbol_node->getName().length(), global_seed);

		if ((symbol_min_line < scope_min_line) && (symbol_max_line > scope_max_line)) // inout symbol
		{
			builder_context.no_assign_context.symbol_inout_hashmap[symbol_name_inout_hash] = 2;
		}
		else if (symbol_min_line < scope_min_line) // input symbols
		{
			builder_context.no_assign_context.symbol_inout_hashmap[symbol_name_inout_hash] = 0;
		}
		else if (symbol_max_line > scope_max_line) // output symbols
		{
			builder_context.no_assign_context.symbol_inout_hashmap[symbol_name_inout_hash] = 1;
		}
	}
}

void CASTHashTreeBuilder::getAndUpdateInputHashNodes(CHashNode& func_hash_node)
{
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
}

void CASTHashTreeBuilder::outputDebugString(const CHashNode& func_hash_node)
{
#if TANGRAM_DEBUG
	debug_traverser.appendDebugString("\t[CHashStr:");
	debug_traverser.appendDebugString(func_hash_node.debug_string);
	debug_traverser.appendDebugString("]");

	int input_idx = 0;
	for (auto& in_symbol_last_assign_index : func_hash_node.input_hash_nodes)
	{
		debug_traverser.appendDebugString("\t[InHashStr:");

		CHashNode& hash_node = tree_hash_nodes[in_symbol_last_assign_index];
		hash_node.debug_string;
		debug_traverser.appendDebugString(hash_node.debug_string);
		debug_traverser.appendDebugString("]");
	}
#endif
}

void CASTHashTreeBuilder::updateLastAssignHashmap(const CHashNode& func_hash_node)
{
	std::vector<XXH64_hash_t>& output_hash_values = builder_context.getOutputHashValues();
	for (auto& out_symbol_hash : output_hash_values)
	{
		builder_context.symbol_last_hashnode_map[out_symbol_hash] = func_hash_node.hash_value;
	}
}

void CASTHashTreeBuilder::increAndDecrePath(TVisit visit, TIntermNode* current)
{
	if (visit == EvPreVisit) 
	{ 
		debug_traverser.incrementDepth(current);
	}

	if (visit == EvPostVisit) 
	{ 
		debug_traverser.decrementDepth();
	}
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
			bool is_surceee = shader.parse(GetDefaultResources(), defaultVersion, isForwardCompatible, messages);
			//assert_t(is_surceee);
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
		hash_tree_builder.preTranverse(intermediate);
		intermediate->getTreeRoot()->traverse(&hash_tree_builder);
		addAstHashTree(hash_tree_builder.getTreeHashNodes());
	}
 	return false;
}


