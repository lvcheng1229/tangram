#include "ast_node_recursive_copy.h"

//	#define GET_AND_SET_MEMBER(Member)\
//	new_node->set##Member(node->get##Member());\


bool CGlobalAstNodeRecursiveCopy::visitBinary(TVisit visit, TIntermBinary* node)
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
		node_stack_context.push_back(binary_node);

		resetGlobalASTPool();
	}

	return true;
}

bool CGlobalAstNodeRecursiveCopy::visitUnary(TVisit visit, TIntermUnary* node)
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

bool CGlobalAstNodeRecursiveCopy::visitAggregate(TVisit visit, TIntermAggregate* node)
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
		new_node->setPragmaTable(node->getPragmaTable());
		new_node->setSpirvInstruction(node->getSpirvInstruction());
		new_node->setLinkType(node->getLinkType());
		node_stack_context.push_back(new_node);

		resetGlobalASTPool();
	}
	return true;
}

bool CGlobalAstNodeRecursiveCopy::visitSelection(TVisit visit, TIntermSelection* node)
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

void CGlobalAstNodeRecursiveCopy::visitConstantUnion(TIntermConstantUnion* node)
{
	setGlobalASTPool();
	char* node_mem = reinterpret_cast<char*>(global_allocator.allocate(sizeof(TIntermConstantUnion)));
	TIntermConstantUnion* new_node = new(node_mem)TIntermConstantUnion(node->getConstArray(), *node->getType().clone());
	if (node->isLiteral()) { new_node->setLiteral(); };
	node_stack_context.push_back(new_node);
	resetGlobalASTPool();
}

void CGlobalAstNodeRecursiveCopy::visitSymbol(TIntermSymbol* node)
{
	setGlobalASTPool();
	char* node_mem = reinterpret_cast<char*>(global_allocator.allocate(sizeof(TIntermSymbol)));
	TIntermSymbol* symbol_node = new(node_mem)TIntermSymbol(node->getId(), node->getName(), *node->getType().clone());
	symbol_node->setFlattenSubset(node->getFlattenSubset());
	symbol_node->setConstArray(node->getConstArray());
	assert(node->getConstSubtree() == nullptr);
	node_stack_context.push_back(symbol_node);
	resetGlobalASTPool();
}

bool CGlobalAstNodeRecursiveCopy::visitLoop(TVisit visit, TIntermLoop* node)
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

bool CGlobalAstNodeRecursiveCopy::visitBranch(TVisit visit, TIntermBranch* node)
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

bool CGlobalAstNodeRecursiveCopy::visitSwitch(TVisit visit, TIntermSwitch* node)
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
