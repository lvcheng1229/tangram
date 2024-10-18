#include "ast_node_deepcopy.h"
#include "glslang_helper.h"

//todo: AssignSymbol中出现自增自建的情况

//	#define GET_AND_SET_MEMBER(Member)\
//	new_node->set##Member(node->get##Member());\

static bool sortSymbolNodes(TIntermSymbol* symbol_a, TIntermSymbol* symbol_b)
{
	return symbol_a->getId() < symbol_b->getId();
}

TIntermNode* CGlobalAstNodeDeepCopy::getCopyedNodeAndResetContext(std::unordered_map<XXH32_hash_t, ESymbolState>& symbol_state_map, ESymbolScopeType scopeType, std::set<XXH32_hash_t>& linker_node_map)
{
	std::sort(symbol_nodes.begin(), symbol_nodes.end(), sortSymbolNodes);

	long long input_index = 0;
	long long output_index = 0;

	std::unordered_map<XXH32_hash_t, int32_t>input_symbol_index_map;

	for (int symbol_node_index = 0; symbol_node_index < symbol_nodes.size(); symbol_node_index++)
	{
		// 首先遍历所有input nodes，看看其是否是input，如果是，判断这个symbol是否之前出现过
		ESymbolState symbol_state = ESymbolState::SS_None;

		TTanGramIntermSymbol* symbol_node = symbol_nodes[symbol_node_index];
		symbol_node->setScopeType(scopeType);

		XXH32_hash_t name_hash = XXH32(symbol_node->getName().data(), symbol_node->getName().size(), global_seed);

		// input node index
		auto iter = symbol_state_map.find(name_hash);
		if (iter != symbol_state_map.end() && ((iter->second & ESymbolState::SS_InputSymbol) != 0))
		{
			auto input_index_iter = input_symbol_index_map.find(name_hash);
			if (input_index_iter != input_symbol_index_map.end())
			{
				symbol_node->setAsInputIndex(input_index_iter->second);
			}
			else
			{
				symbol_node->setAsInputIndex(input_index);
				input_symbol_index_map[name_hash] = input_index;
				input_index++;
			}

			symbol_state = ESymbolState(symbol_state | ESymbolState::SS_InputSymbol);
		}

		// output node index
		if (iter != symbol_state_map.end() && ((iter->second & ESymbolState::SS_OutputSymbol) != 0))
		{
			symbol_node->setAsOutputIndex(output_index);
			output_index++;
			symbol_state = ESymbolState(symbol_state | ESymbolState::SS_OutputSymbol);
		}

		symbol_node->addSymbolState(symbol_state);

		if (linker_node_map.find(name_hash) != linker_node_map.end())
		{
			symbol_node->addSymbolState(ESymbolState::SS_LinkerSymbol);
		}
		if (scopeType == SST_AssignUnit)
		{
			assert(symbol_state != SS_InoutSymbol);
		}
	}

	

	return getCopyedNodeAndResetContextImpl();
}

//TIntermNode* CGlobalAstNodeDeepCopy::getCopyedNodeAndResetContextAssignNode(XXH32_hash_t output_hash_node)
//{
//	long long input_index = 0;
//	std::unordered_map<XXH32_hash_t, int32_t>input_symbol_index_map;
//
//	for (int symbol_node_index = 0; symbol_node_index < symbol_nodes.size(); symbol_node_index++)
//	{
//		TTanGramIntermSymbol* symbol_node = symbol_nodes[symbol_node_index];
//		if (symbol_node->getName() == "out_var_SV_Target0")
//		{
//			int debug_avr = 1;
//
//		}
//		symbol_node->setScopeType(ESymbolScopeType::SST_AssignUnit);
//		XXH32_hash_t name_hash = XXH32(symbol_node->getName().data(), symbol_node->getName().size(), global_seed);
//		
//		if (name_hash != output_hash_node)
//		{
//			auto input_index_iter = input_symbol_index_map.find(name_hash);
//			if (input_index_iter != input_symbol_index_map.end())
//			{
//				if (symbol_nodes.size() == 4 && input_index_iter->second == 2)
//				{
//					int debug_avr = 1;
//				}
//				symbol_node->setAsInputIndex(input_index_iter->second);
//			}
//			else
//			{
//				if (symbol_nodes.size() == 4 && input_index == 2)
//				{
//					int debug_avr = 1;
//				}
//				symbol_node->setAsInputIndex(input_index);
//				input_symbol_index_map[name_hash] = input_index;
//				input_index++;
//			}
//
//			symbol_node->setSymbolState(ESymbolState::SS_InputSymbol);
//		}
//		else
//		{
//			symbol_node->setAsOutputIndex(0);
//			symbol_node->setSymbolState(ESymbolState::SS_OutputSymbol);
//		}
//	}
//
//	return getCopyedNodeAndResetContextImpl();
//}

TIntermNode* CGlobalAstNodeDeepCopy::getCopyedNodeAndResetContextLinkNode()
{
	assert(symbol_nodes.size() == 1);
	TTanGramIntermSymbol* symbol_node = symbol_nodes[0];
	symbol_node->setScopeType(ESymbolScopeType::SST_LinkerNode);
	symbol_node->setAsOutputIndex(0);
	symbol_node->addSymbolState(ESymbolState::SS_OutputSymbol);
	return getCopyedNodeAndResetContextImpl();
}

TIntermNode* CGlobalAstNodeDeepCopy::getCopyedNodeAndResetContextImpl()
{
	TIntermNode* copyed_node = node_stack_context.back();
	node_stack_context.pop_back();
	symbol_id2index.clear();
	symbol_index = 0;
	assert(node_stack_context.size() == 0);
	symbol_nodes.clear();
	is_visit_link_node = false;
	//scope_context.reset();
	return copyed_node;
}

bool CGlobalAstNodeDeepCopy::visitBinary(TVisit visit, TIntermBinary* node)
{
	if (visit == EvPostVisit)
	{
		setGlobalASTPool();

		// stack left -> first
		TIntermTyped* right_node = (node->getRight() != nullptr) ? getAndPopCopyedNode()->getAsTyped() : nullptr;
		TIntermTyped* left_node = (node->getLeft() != nullptr) ? getAndPopCopyedNode()->getAsTyped() : nullptr;

		TIntermBinary* binary_node = allocateAndConstructIntermOperator(node, node->getOp());
		binary_node->setLeft(left_node);
		binary_node->setRight(right_node);

		TOperator node_operator = node->getOp();
		if (node_operator == EOpIndexDirectStruct)
		{
			const TType& type = node->getLeft()->getType();
			TBasicType basic_type = type.getBasicType();

			assert(basic_type == EbtBlock);
			int member_index = node->getRight()->getAsConstantUnion()->getConstArray()[0].getIConst();

			const TTypeList* structure = type.getStruct();

			const TString struct_string = getTypeText_HashTree(type);
			XXH32_hash_t struct_inst_hash = XXH32(struct_string.data(), struct_string.size(), global_seed);

			int16_t struct_size = 0;
			for (size_t i = 0; i < structure->size(); ++i)
			{
				TType* struct_mem_type = (*structure)[i].type;
				struct_size += getTypeSize(*struct_mem_type);
			}

			int member_offset = 0;
			for (size_t i = 0; i < structure->size(); ++i)
			{
				TType* struct_mem_type = (*structure)[i].type;
				int16_t member_size = getTypeSize(*struct_mem_type);
				if (member_index == i)
				{
					const TString& member_str = (*structure)[member_index].type->getFieldName();

					char* ub_memory = reinterpret_cast<char*>(global_allocator.allocate(sizeof(SUniformBufferMemberDesc)));
					SUniformBufferMemberDesc* ub_mem_desc = new(ub_memory)SUniformBufferMemberDesc();
					ub_mem_desc->struct_instance_hash = struct_inst_hash;
					ub_mem_desc->struct_member_hash = XXH32(member_str.data(), member_str.size(), global_seed);
					ub_mem_desc->struct_member_size = member_size;
					ub_mem_desc->struct_member_offset = member_offset;
					ub_mem_desc->struct_size = struct_size;
					ub_mem_desc->struct_index = member_index;
					getTanGramNode(left_node->getAsSymbolNode())->setUBMemberDesc(ub_mem_desc);
					break;
				}
				member_offset += member_size;
			}
		}

		node_stack_context.push_back(binary_node);

		resetGlobalASTPool();
	}

	return true;
}

bool CGlobalAstNodeDeepCopy::visitUnary(TVisit visit, TIntermUnary* node)
{
	if (visit == EvPostVisit)
	{
		setGlobalASTPool();

		TIntermTyped* operand = static_cast<TIntermTyped*>(getAndPopCopyedNode());

		TIntermUnary* unary_node = allocateAndConstructIntermOperator(node, node->getOp());
		unary_node->setOperand(operand);
		unary_node->setSpirvInstruction(node->getSpirvInstruction());
		node_stack_context.push_back(unary_node);

		resetGlobalASTPool();
	}
	return true;
}

bool CGlobalAstNodeDeepCopy::visitAggregate(TVisit visit, TIntermAggregate* node)
{
	if (visit == EvPostVisit)
	{
		setGlobalASTPool();

		TIntermAggregate* new_node = allocateAndConstructIntermOperator(node, node->getOp());

		new_node->getSequence().resize(node->getSequence().size());
		for (int idx = new_node->getSequence().size() - 1; idx >= 0; idx--)
		{
			new_node->getSequence()[idx] = getAndPopCopyedNode();
		}
		new_node->getQualifierList() = node->getQualifierList();
		new_node->setName(node->getName());
		//new_node->setPragmaTable(node->getPragmaTable());
		new_node->setSpirvInstruction(node->getSpirvInstruction());
		new_node->setLinkType(node->getLinkType());
		node_stack_context.push_back(new_node);

		resetGlobalASTPool();
	}
	return true;
}

bool CGlobalAstNodeDeepCopy::visitSelection(TVisit visit, TIntermSelection* node)
{
	if (visit == EvPostVisit)
	{
		setGlobalASTPool();

		TIntermNode* false_node = getAndPopCopyedNode();
		TIntermNode* true_node = (node->getTrueBlock() != nullptr) ? getAndPopCopyedNode() : nullptr;
		TIntermNode* condition_node = (node->getFalseBlock() != nullptr) ? getAndPopCopyedNode() : nullptr;

		char* node_mem = reinterpret_cast<char*>(global_allocator.allocate(sizeof(TIntermSelection)));
		TIntermSelection* new_node = new(node_mem)TIntermSelection(condition_node->getAsTyped(), true_node, false_node, *node->getType().clone());
		if (node->getShortCircuit() == false) { new_node->setNoShortCircuit(); }
		if (node->getDontFlatten()) { new_node->setDontFlatten(); }
		if (node->getFlatten()) { new_node->setFlatten(); }
		node_stack_context.push_back(new_node);

		resetGlobalASTPool();
	}
	return true;
}

void CGlobalAstNodeDeepCopy::visitConstantUnion(TIntermConstantUnion* node)
{
	setGlobalASTPool();
	char* node_mem = reinterpret_cast<char*>(global_allocator.allocate(sizeof(TIntermConstantUnion)));
	TIntermConstantUnion* new_node = new(node_mem)TIntermConstantUnion(TConstUnionArray(node->getConstArray(), 0, node->getConstArray().size()), *node->getType().clone());
	if (node->isLiteral()) { new_node->setLiteral(); };
	node_stack_context.push_back(new_node);
	resetGlobalASTPool();
}

void CGlobalAstNodeDeepCopy::visitSymbol(TIntermSymbol* node)
{
	setGlobalASTPool();

	// 将SymbolID 设置成Symbol最先出现时的Index，用于处理一个Symbol出现多次的情况
	auto symbol_map_iter = symbol_id2index.find(node->getId());
	long long new_symbol_index = 0;
	if (symbol_map_iter != symbol_id2index.end())
	{
		new_symbol_index = symbol_map_iter->second;
	}
	else
	{
		new_symbol_index = symbol_index;
		symbol_id2index[node->getId()] = new_symbol_index;
		symbol_index++;
	}

	char* node_mem = reinterpret_cast<char*>(global_allocator.allocate(sizeof(TTanGramIntermSymbol)));
	TTanGramIntermSymbol* symbol_node = new(node_mem)TTanGramIntermSymbol(node->getId(), node->getName(), *node->getType().clone());
	symbol_node->setFlattenSubset(node->getFlattenSubset());
	symbol_node->setConstArray(node->getConstArray());
	symbol_node->switchId(new_symbol_index);

	const TType& type = node->getType();
	if (is_visit_link_node)
	{
		TBasicType basic_type = type.getBasicType();
		if (basic_type == EbtBlock)
		{
			char* ub_memory = reinterpret_cast<char*>(global_allocator.allocate(sizeof(SUniformBufferMemberDesc)));
			SUniformBufferMemberDesc* ub_member_node = new(ub_memory)SUniformBufferMemberDesc();
			symbol_node->setUBMemberDesc(ub_member_node);
		}
	}
	

	assert(node->getConstSubtree() == nullptr);
	node_stack_context.push_back(symbol_node);
	symbol_nodes.push_back(symbol_node);
	resetGlobalASTPool();
}

bool CGlobalAstNodeDeepCopy::visitLoop(TVisit visit, TIntermLoop* node)
{
	if (visit == EvPostVisit)
	{
		setGlobalASTPool();
		TIntermTyped* test_node = (node->getTest() != nullptr) ? getAndPopCopyedNode()->getAsTyped() : nullptr;
		TIntermNode* body_node = (node->getTest() != nullptr) ? getAndPopCopyedNode() : nullptr;
		TIntermTyped* terminal_node = (node->getTest() != nullptr) ? getAndPopCopyedNode()->getAsTyped() : nullptr;

		char* node_mem = reinterpret_cast<char*>(global_allocator.allocate(sizeof(TIntermLoop)));
		TIntermLoop* new_node = new(node_mem)TIntermLoop(body_node, test_node, terminal_node, node->testFirst());
		if (node->getUnroll()) { new_node->setUnroll(); }
		if (node->getDontUnroll()) { new_node->setDontUnroll(); }
		new_node->setLoopDependency(node->getLoopDependency());
		new_node->setMinIterations(node->getMinIterations());
		new_node->setMaxIterations(node->getMaxIterations());
		new_node->setIterationMultiple(node->getIterationMultiple());
		assert(node->getPeelCount() == 0);
		assert(node->getPartialCount() == 0);
		node_stack_context.push_back(new_node);
		resetGlobalASTPool();
	}
	return true;
}

bool CGlobalAstNodeDeepCopy::visitBranch(TVisit visit, TIntermBranch* node)
{
	if (visit == EvPostVisit)
	{
		setGlobalASTPool();
		TIntermTyped* expression_node = (node->getExpression() != nullptr) ? getAndPopCopyedNode()->getAsTyped() : nullptr;

		char* node_mem = reinterpret_cast<char*>(global_allocator.allocate(sizeof(TIntermBranch)));
		TIntermBranch* new_node = new(node_mem)TIntermBranch(node->getFlowOp(), expression_node);
		node_stack_context.push_back(new_node);
		resetGlobalASTPool();
	}
	return true;
}

bool CGlobalAstNodeDeepCopy::visitSwitch(TVisit visit, TIntermSwitch* node)
{
	if (visit == EvPostVisit)
	{
		setGlobalASTPool();
		TIntermAggregate* body = getAndPopCopyedNode()->getAsAggregate();
		TIntermTyped* condition = getAndPopCopyedNode()->getAsTyped();

		char* node_mem = reinterpret_cast<char*>(global_allocator.allocate(sizeof(TIntermSwitch)));
		TIntermSwitch* new_node = new(node_mem)TIntermSwitch(condition, body);
		if (node->getFlatten()) { new_node->setFlatten(); };
		if (node->getDontFlatten()) { new_node->setDontFlatten(); };
		node_stack_context.push_back(new_node);
		resetGlobalASTPool();
	}
	return true;
}
