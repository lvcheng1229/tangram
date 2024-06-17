#include "ast_hash_tree.h"

CGlobalHashNodeMannager::CGlobalHashNodeMannager()
{
	mem_size = HASH_NODE_MEM_BLOCK_SIZE;
	global_hashed_node_mem = malloc(mem_size);
	mem_offset = 0;
}

void* CGlobalHashNodeMannager::allocHashedNodeMem(int size)
{
	if ((mem_offset + size) < mem_size)
	{
		void* ret_mem = ((char*)(global_hashed_node_mem)) + mem_offset;
		mem_offset += size;
		return ret_mem;
	}
	else
	{
		uint32_t new_size = mem_size + HASH_NODE_MEM_BLOCK_SIZE;
		void* new_mem = malloc(new_size);
		if (new_mem != nullptr)
		{
			memcpy(new_mem, global_hashed_node_mem, new_size);
			free(global_hashed_node_mem);
			global_hashed_node_mem = new_mem;
		}
		else
		{
			assert_t(false);
		}

	}
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

	}
	return false;
}

bool TASTHashTraverser::visitBinary(TVisit, TIntermBinary* node)
{
	bool visit = true;

	if (custom_traverser->preVisit)
	{
		visit = custom_traverser->visitBinary(EvPreVisit, node);
	}

	if (visit) {
		custom_traverser->incrementDepth(node);

		if (custom_traverser->rightToLeft) 
		{
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
		visit = it->visitUnary(EvPreVisit, this);

	if (visit) {
		custom_traverser->incrementDepth(this);
		operand->traverse(custom_traverser);
		custom_traverser->decrementDepth();
	}

	if (visit && custom_traverser->postVisit)
		custom_traverser->visitUnary(EvPostVisit, this);

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
