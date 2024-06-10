#include "ast_tranversar_private.h"
#include <algorithm>

bool TSymbolScopeTraverser::visitBinary(TVisit visit, TIntermBinary* node)
{
	TOperator node_operator = node->getOp();
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
				auto iter = symbol_max_line.find(symbol_id);
				if (iter != symbol_max_line.end())
				{
					int symbol_max_line = iter->second;
					if (node->getLoc().line > symbol_max_line)
					{
						iter->second = node->getLoc().line;
					}
				}
				else
				{
					symbol_max_line[symbol_id] = node->getLoc().line;
				}
			}
		}
	}
	};
	return true;
}

bool TSymbolScopeTraverser::visitUnary(TVisit, TIntermUnary* node)
{
	return true;
}

bool TSymbolScopeTraverser::visitAggregate(TVisit, TIntermAggregate* node)
{
	return true;
}

bool TSymbolScopeTraverser::visitSelection(TVisit, TIntermSelection* node)
{
	return true;
}

void TSymbolScopeTraverser::visitConstantUnion(TIntermConstantUnion* node)
{
}

void TSymbolScopeTraverser::visitSymbol(TIntermSymbol* node)
{
	long long symbol_id = node->getId();
	auto iter = symbol_max_line.find(symbol_id);
	if (iter != symbol_max_line.end())
	{
		int symbol_max_line = iter->second;
		if (node->getLoc().line > symbol_max_line)
		{
			iter->second = node->getLoc().line;
		}
	}
	else
	{
		symbol_max_line[symbol_id] = node->getLoc().line;
	}
}

bool TSymbolScopeTraverser::visitLoop(TVisit, TIntermLoop* node)
{
	return false;
}

bool TSymbolScopeTraverser::visitBranch(TVisit, TIntermBranch* node)
{
	return false;
}

bool TSymbolScopeTraverser::visitSwitch(TVisit, TIntermSwitch* node)
{
	return false;
}






bool TSubScopeTraverser::visitBinary(TVisit visit, TIntermBinary* node)
{
	subscope_max_line = (std::max)(subscope_max_line, node->getLoc().line);

	TOperator node_operator = node->getOp();
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
				auto iter = declared_symbols_id->find(symbol_id);
				if (iter == declared_symbols_id->end())
				{
					if (subscope_symbols.find(symbol_id) == subscope_symbols.end())
					{
						subscope_symbols[symbol_id] = symbol_node;
					}
				}
			}
		}
	}
	};
	return true;
}

bool TSubScopeTraverser::visitUnary(TVisit, TIntermUnary* node)
{
	subscope_max_line = (std::max)(subscope_max_line, node->getLoc().line);
	return true;
}

bool TSubScopeTraverser::visitAggregate(TVisit, TIntermAggregate* node)
{
	subscope_max_line = (std::max)(subscope_max_line, node->getLoc().line);
	return true;
}

bool TSubScopeTraverser::visitSelection(TVisit, TIntermSelection* node)
{
	subscope_max_line = (std::max)(subscope_max_line, node->getLoc().line);
	return true;
}

void TSubScopeTraverser::visitConstantUnion(TIntermConstantUnion* node)
{
	subscope_max_line = (std::max)(subscope_max_line, node->getLoc().line);
}

void TSubScopeTraverser::visitSymbol(TIntermSymbol* node)
{
	subscope_max_line = (std::max)(subscope_max_line, node->getLoc().line);

	long long symbol_id = node->getId();
	auto iter = declared_symbols_id->find(symbol_id);
	if (iter == declared_symbols_id->end())
	{
		if (subscope_symbols.find(symbol_id) == subscope_symbols.end())
		{
			subscope_symbols[symbol_id] = node;
		}
	}
}

bool TSubScopeTraverser::visitLoop(TVisit, TIntermLoop* node)
{
	subscope_max_line = (std::max)(subscope_max_line, node->getLoc().line);
	return true;
}

bool TSubScopeTraverser::visitBranch(TVisit, TIntermBranch* node)
{
	subscope_max_line = (std::max)(subscope_max_line, node->getLoc().line);
	return true;
}

bool TSubScopeTraverser::visitSwitch(TVisit, TIntermSwitch* node)
{
	subscope_max_line = (std::max)(subscope_max_line, node->getLoc().line);

	incrementDepth(node);
	node->getCondition()->traverse(this);
	node->getBody()->traverse(this);
	decrementDepth();
	return true;
}