#include "ast_node_recursive_copy.h"

bool CGlobalAstNodeRecursiveCopy::visitBinary(TVisit visit, TIntermBinary* node)
{
	if (visit == EvPreVisit)
	{

	}
	else if (visit == EvInVisit)
	{

	}
	else if (visit == EvPostVisit)
	{
		TIntermNode* left_node = node_stack_context.back();
		node_stack_context.pop_back();

		TIntermNode* right_node = node_stack_context.back();
		node_stack_context.pop_back();

		char* node_mem = reinterpret_cast<char*>(global_allocator.allocate(sizeof(TIntermBinary)));
		TIntermBinary* binary_node = new(node_mem)TIntermBinary(node->getOp());
		binary_node->setLeft(static_cast<TIntermTyped*>(left_node));
		binary_node->setRight(static_cast<TIntermTyped*>(right_node));
	}

	return true;
}
