#include "ast_hash_tree.h"

static int global_seed = 42;



bool TASTHashTraverser::visitBinary(TVisit, TIntermBinary* node)
{
	bool visit = true;

	XXH64_hash_t child_hash_values[2];

	if (custom_traverser->preVisit)
	{
		visit = custom_traverser->visitBinary(EvPreVisit, node);
	}

	if (visit) {
		custom_traverser->incrementDepth(node);

		if (custom_traverser->rightToLeft) 
		{
			assert_t(false);

			if (node->getRight())
			{
				node->getRight()->traverse(custom_traverser);
			}

			if (custom_traverser->inVisit)
			{
				visit = custom_traverser->visitBinary(EvInVisit, node);
			}

			if (visit && node->getLeft())
			{
				node->getLeft()->traverse(custom_traverser);
			}
		}
		else 
		{
			if (node->getLeft())
			{
				node->getLeft()->traverse(custom_traverser);
			}
				
			if (custom_traverser->inVisit)
			{
				visit = custom_traverser->visitBinary(EvInVisit, node);
			}

			if (visit && node->getRight())
			{
				node->getRight()->traverse(custom_traverser);
			}
		}

		custom_traverser->decrementDepth();
	}

	if (visit && custom_traverser->postVisit)
	{
		custom_traverser->visitBinary(EvPostVisit, node);
	}
		

	return true;
}

bool TASTHashTraverser::visitUnary(TVisit, TIntermUnary* node)
{
	bool visit = true;

	if (custom_traverser->preVisit)
	{
		visit = custom_traverser->visitUnary(EvPreVisit, node);
	}

	if (visit) 
	{
		custom_traverser->incrementDepth(node);
		node->getOperand()->traverse(custom_traverser);
		custom_traverser->decrementDepth();
	}

	if (visit && custom_traverser->postVisit)
	{
		custom_traverser->visitUnary(EvPostVisit, node);
	}

	return true;
}

bool TASTHashTraverser::visitAggregate(TVisit, TIntermAggregate* node)
{
	bool visit = true;

	if (custom_traverser->preVisit)
	{
		visit = custom_traverser->visitAggregate(EvPreVisit, node);
	}

	if (visit)
	{
		custom_traverser->incrementDepth(node);
		if (custom_traverser->rightToLeft)
		{
			for (TIntermSequence::reverse_iterator sit = node->getSequence().rbegin(); sit != node->getSequence().rend(); sit++)
			{
				(*sit)->traverse(custom_traverser);
				if (visit && inVisit) 
				{
					if (*sit != node->getSequence().front())
					{
						visit = custom_traverser->visitAggregate(EvInVisit, node);
					}
				}
			}
		}
		else
		{
			for (TIntermSequence::iterator sit = node->getSequence().begin(); sit != node->getSequence().end(); sit++) 
			{
				(*sit)->traverse(custom_traverser);

				if (visit && custom_traverser->inVisit) 
				{
					if (*sit != node->getSequence().back())
					{
						visit = custom_traverser->visitAggregate(EvInVisit, node);
					}
				}
			}
		}
		custom_traverser->decrementDepth();
	}

	if (visit && custom_traverser->postVisit)
	{
		custom_traverser->visitAggregate(EvPostVisit, node);
	}
		
	return true;
}

bool TASTHashTraverser::visitSelection(TVisit, TIntermSelection* node)
{
	bool visit = true;

	if (custom_traverser->preVisit)
	{
		visit = custom_traverser->visitSelection(EvPreVisit, node);
	}

	if (visit) 
	{
		custom_traverser->incrementDepth(node);
		if (custom_traverser->rightToLeft) 
		{
			if (node->getFalseBlock())
			{
				node->getFalseBlock()->traverse(custom_traverser);
			}
				
			if (node->getTrueBlock())
			{
				node->getTrueBlock()->traverse(custom_traverser);
			}
				
			node->getCondition()->traverse(custom_traverser);
		}
		else 
		{
			node->getCondition()->traverse(custom_traverser);

			if (node->getTrueBlock())
			{
				node->getTrueBlock()->traverse(custom_traverser);
			}
				
			if (node->getFalseBlock())
			{
				node->getFalseBlock()->traverse(custom_traverser);
			}
		}
		custom_traverser->decrementDepth();
	}

	if (visit && custom_traverser->postVisit)
	{
		custom_traverser->visitSelection(EvPostVisit, node);
	}

	return true;
}

void TASTHashTraverser::visitConstantUnion(TIntermConstantUnion* node)
{
	custom_traverser->visitConstantUnion(node);
}

void TASTHashTraverser::visitSymbol(TIntermSymbol* node)
{
	custom_traverser->visitSymbol(node);
}

bool TASTHashTraverser::visitLoop(TVisit, TIntermLoop* node)
{
	bool visit = true;

	if (custom_traverser->preVisit)
	{
		visit = custom_traverser->visitLoop(EvPreVisit, node);
	}
		

	if (visit) 
	{
		custom_traverser->incrementDepth(node);

		if (custom_traverser->rightToLeft) 
		{
			if (node->getTerminal())
			{
				node->getTerminal()->traverse(custom_traverser);
			}
				
			if (node->getBody())
			{
				node->getBody()->traverse(custom_traverser);
			}

			if (node->getTest())
			{
				node->getTest()->traverse(custom_traverser);
			}
		}
		else 
		{
			if (node->getTest())
			{
				node->getTest()->traverse(custom_traverser);
			}

			if (node->getBody())
			{
				node->getBody()->traverse(custom_traverser);
			}

			if (node->getTerminal())
			{
				node->getTerminal()->traverse(custom_traverser);
			}
		}

		custom_traverser->decrementDepth();
	}

	if (visit && custom_traverser->postVisit)
	{
		custom_traverser->visitLoop(EvPostVisit, node);
	}

	return true;
}

bool TASTHashTraverser::visitBranch(TVisit, TIntermBranch* node)
{
	bool visit = true;

	if (custom_traverser->preVisit)
	{
		visit = custom_traverser->visitBranch(EvPreVisit, node);
	}

	if (visit && node->getExpression()) 
	{
		custom_traverser->incrementDepth(node);
		node->getExpression()->traverse(custom_traverser);
		custom_traverser->decrementDepth();
	}

	if (visit && custom_traverser->postVisit)
	{
		custom_traverser->visitBranch(EvPostVisit, node);
	}

	return true;
}

bool TASTHashTraverser::visitSwitch(TVisit, TIntermSwitch* node)
{
	bool visit = true;

	if (custom_traverser->preVisit)
		visit = custom_traverser->visitSwitch(EvPreVisit, node);

	if (visit) {
		custom_traverser->incrementDepth(node);
		if (custom_traverser->rightToLeft) 
		{
			node->getBody()->traverse(custom_traverser);
			node->getCondition()->traverse(custom_traverser);
		}
		else {
			node->getCondition()->traverse(custom_traverser);
			node->getBody()->traverse(custom_traverser);
		}
		custom_traverser->decrementDepth();
	}

	if (visit && custom_traverser->postVisit)
	{
		custom_traverser->visitSwitch(EvPostVisit, node);
	}

	return true;
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

	if (type.isStruct() && type.getStruct())
	{
		
	}
	return type_string;
}

bool CASTHashTreeBuilder::visitBinary(TVisit visit, TIntermBinary* node)
{
	TOperator node_operator = node->getOp();
	TString hash_string;

	if (visit == EvPostVisit)
	{
		hash_string.reserve(2 + 3 + 5 + 5);
		hash_string.append(std::string("2_")); 
		hash_string.append(std::to_string(uint32_t(node_operator)).c_str());
		hash_string.append(std::string("_"));
	}

	if (visit == EvPreVisit)
	{
		
	}
	
	switch (node_operator)
	{
	case EOpAssign:
	{
		if (visit == EvPreVisit)
		{
			TIntermSymbol* symbol_node = node->getLeft()->getAsSymbolNode();

			if (symbol_node == nullptr)
			{
				TIntermBinary* binary_node = node->getLeft()->getAsBinaryNode();
				if (binary_node)
				{
					symbol_node = binary_node->getLeft()->getAsSymbolNode();
				}
			}

			if (symbol_node != nullptr)
			{
				long long symbol_id = symbol_node->getId();
				auto iter = declared_symbols_id.find(symbol_id);
				if (iter == declared_symbols_id.end()) { declared_symbols_id.insert(symbol_id); }
			}

			//todo: left symbol may be a expression, e.g. a.x = ;
			{
				TIntermSymbol* symbol_node = node->getLeft()->getAsSymbolNode();
				const TString& symbol_name = symbol_node->getName();
				if ((symbol_name[0] == 'g') && (symbol_name[1] == 'l') && (symbol_name[2] == '_'))
				{
					assert_t(false);
				}

				TString return_symbol_string;
				return_symbol_string = getTypeText(node->getType());
				return_symbol_string.append(TString("_"));
				return_symbol_string.append(symbol_name);
				XXH64_hash_t return_symbol_hash_value = XXH64(return_symbol_string.data(), return_symbol_string.size(), global_seed);
				//builder_context.symbol_last_hashnode_map[return_symbol_hash_value] = func_node_combined_hash;
			}

		}
		else if (visit == EvPostVisit)
		{
			XXH64_hash_t func_node_topology_hash;
			{

			}

			XXH64_hash_t func_node_combined_hash; //combined with input symbol hash and output symbol hash


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
			const TType& type = node->getType();
			TString block_hash_string = getTypeText(type);

			TString struct_string = block_hash_string;
			struct_string.append(symbol_node->getName());

			TString member_string = block_hash_string;
			member_string.append(index_direct_struct_str);

			XXH64_hash_t struct_hash_value = XXH64(struct_string.data(), struct_string.size(), global_seed);
			XXH64_hash_t member_hash_value = XXH64(member_string.data(), member_string.size(), global_seed);

			auto struct_symbol_iter = hash_value_to_idx.find(struct_hash_value);
			auto member_symbol_iter = hash_value_to_idx.find(member_hash_value);

			assert_t(struct_symbol_iter != hash_value_to_idx.end());
			assert_t(member_symbol_iter != hash_value_to_idx.end());

			builder_context.inout_hash_nodes.push_back(struct_symbol_iter->second);
			builder_context.inout_hash_nodes.push_back(member_symbol_iter->second);

			hash_string.append(std::to_string(XXH64_hash_t(struct_hash_value)).c_str());
			hash_string.append(std::string("_"));
			hash_string.append(std::to_string(XXH64_hash_t(member_hash_value)).c_str());
			hash_string.append(std::string("_"));

			XXH64_hash_t index_direct_struct_hash = XXH64(hash_string.data(), hash_string.size(), global_seed);
			hash_value_stack.push_back(index_direct_struct_hash);

			node->setLeft(nullptr);
			node->setRight(nullptr);
		}
		break;
	}
	default:
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
	};

	

	return true;
}

void CASTHashTreeBuilder::visitSymbol(TIntermSymbol* node)
{
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

	if (is_declared == false)
	{
		TString hash_string;

		const TType& type = node->getType();
		TString type_string = getTypeText(type);
		hash_string.append(type_string);

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
		else
		{
			//do nothing
			//visitBinary
		}
	}
	else
	{
		TString hash_string;

		const TType& type = node->getType();
		TString type_string = getTypeText(type);
		hash_string.append(type_string);

		XXH64_hash_t type_hash_value = XXH64(hash_string.data(), hash_string.size(), global_seed);
		hash_value_stack.push_back(type_hash_value);

		if (type.getQualifier().hasLayout())
		{
			XXH64_hash_t hash_value = XXH64(hash_string.data(), hash_string.size(), global_seed);
			builder_context.input_hash_nodes.push_back(hash_value_to_idx[hash_value]);
		}
		else
		{
			hash_string.append(TString("_"));
			hash_string.append(node->getName());
			XXH64_hash_t hash_value = XXH64(hash_string.data(), hash_string.size(), global_seed);
			auto iter = builder_context.symbol_last_hashnode_map.find(hash_value);
			builder_context.input_hash_nodes.push_back(iter->second);
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
		TASTHashTraverser hash_tree_tranverser(&hash_tree_builder);
		intermediate->getTreeRoot()->traverse(&hash_tree_tranverser);

	}
	return false;
}