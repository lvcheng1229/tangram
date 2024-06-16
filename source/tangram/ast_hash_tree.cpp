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

		TASTHashTraverser hash_tree_tranverser;
		intermediate->getTreeRoot()->traverse(&hash_tree_tranverser);
	}
	return false;
}