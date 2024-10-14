#include "ast_tranversar.h"
#include <algorithm>

bool TInvalidShaderTraverser::visitBinary(TVisit visit, TIntermBinary* node)
{
	TOperator node_operator = node->getOp();
	switch (node_operator)
	{
	case EOpAssign:
	{
		const TType& type = node->getType();
		if (type.isArray())
		{
			const TArraySizes* array_sizes = type.getArraySizes();
			for (int i = 0; i < (int)array_sizes->getNumDims(); ++i)
			{
				int size = array_sizes->getDimSize(i);
				if ((array_sizes->getNumDims() == 1) && (size == 1))
				{
					is_valid_shader = false;
				}
			}
		};
	}
	};
	return true;
}

void TInvalidShaderTraverser::visitSymbol(TIntermSymbol* node)
{
	if (!node->getConstArray().empty())
	{
		is_valid_shader = false;
	}
}

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
					symbol_min_line[symbol_id] = node->getLoc().line;
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
		symbol_min_line[symbol_id] = node->getLoc().line;
	}
}

bool TSubScopeTraverser::visitBinary(TVisit visit, TIntermBinary* node)
{
	updateMaxLine(node);

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
				
				//if (iter == declared_symbols_id->end())
				//{
				//	subscope_min_line = (std::min)(subscope_min_line, node->getLoc().line);
				//}

				if ((iter == declared_symbols_id->end()) || (only_record_undeclared_symbol == false))
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
	updateMaxLine(node);
	incrementDepth(node);
	node->getCondition()->traverse(this);
	if (node->getTrueBlock()){node->getTrueBlock()->traverse(this);}
	if (node->getFalseBlock()) { node->getFalseBlock()->traverse(this); }
	decrementDepth();
	return true;
}
void TSubScopeTraverser::visitSymbol(TIntermSymbol* node)
{
	updateMaxLine(node);

	long long symbol_id = node->getId();
	auto iter = declared_symbols_id->find(symbol_id);

	//if (iter == declared_symbols_id->end())
	//{
	//	subscope_min_line = (std::min)(subscope_min_line, node->getLoc().line);
	//}

	if ((iter == declared_symbols_id->end()) || (only_record_undeclared_symbol == false))
	{
		if (subscope_symbols.find(symbol_id) == subscope_symbols.end())
		{
			subscope_symbols[symbol_id] = node;
		}
	}
}

bool TSubScopeTraverser::visitLoop(TVisit, TIntermLoop* node)
{
	updateMaxLine(node);

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
	updateMaxLine(node);

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
