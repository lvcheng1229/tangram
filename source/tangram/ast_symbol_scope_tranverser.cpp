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


bool TSubScopeTraverser::visitSelection(TVisit, TIntermSelection* node)
{
	subscope_max_line = (std::max)(subscope_max_line, node->getLoc().line);
	incrementDepth(node);
	node->getCondition()->traverse(this);
	if (node->getTrueBlock()){node->getTrueBlock()->traverse(this);}
	if (node->getFalseBlock()) { node->getFalseBlock()->traverse(this); }
	decrementDepth();
	return true;
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
	if (!node->testFirst()) { assert_t(false); }
	if (node->getUnroll()) { assert_t(false); }
	if (node->getLoopDependency()) { assert_t(false); }

	incrementDepth(node);
	if (node->getTest()) { node->getTest()->traverse(this); }
	if (node->getTerminal()) { node->getTerminal()->traverse(this); };

	// loop body
	if (node->getBody()) { node->getBody()->traverse(this); }
	decrementDepth();
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

bool TLoopHeaderTraverser::visitBinary(TVisit visit, TIntermBinary* node)
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
				auto iter = declared_symbols_id->find(symbol_id);
				if (iter == declared_symbols_id->end())
				{
					if (loop_header_symbols.find(symbol_id) == loop_header_symbols.end())
					{
						loop_header_symbols[symbol_id] = symbol_node;
					}
				}
			}
		}
		break;
	}
	}
	return true;
}

void TLoopHeaderTraverser::visitSymbol(TIntermSymbol* node)
{
	long long symbol_id = node->getId();
	auto iter = declared_symbols_id->find(symbol_id);
	if (iter == declared_symbols_id->end())
	{
		if (loop_header_symbols.find(symbol_id) == loop_header_symbols.end())
		{
			loop_header_symbols[symbol_id] = node;
		}
	}
}

bool TLoopHeaderTraverser::visitLoop(TVisit, TIntermLoop* node)
{
	if (node->getTest()) { node->getTest()->traverse(this); }
	if (node->getTerminal()) { node->getTerminal()->traverse(this); };
	return true;
}
