#pragma once
#include "ast_tranversar_private.h"
#include "xxhash.h"

static constexpr int global_seed = 42;

// record the symbol name in a seperate structure
struct CHashNode
{
	CHashNode() :
		weight(1.0),
		length(1.0),
		count(1.0),
		hash_value(0),
		combined_hash_value(0) {}

	float weight;

	// weight = length * count
	float length;
	float count;

#if TANGRAM_DEBUG
	TString debug_string;
#endif

	XXH64_hash_t hash_value;
	XXH64_hash_t combined_hash_value; // combined with in out hash value

	std::vector<uint64_t> input_hash_nodes; //input hash node indices
	//std::vector<uint64_t> inout_hash_nodes; //inout hash node indices
	//std::vector<uint64_t> out_hash_nodes;  //output hash node indices
	std::set<uint64_t> out_hash_nodes;
};

class CGlobalHashNode
{
public:
	std::vector<CHashNode> global_hash_nodes;
};

class CScopeSymbolNameTraverser : public TIntermTraverser
{
public:
	CScopeSymbolNameTraverser() :
		TIntermTraverser(true, true, true, false){ }

	inline void reset() { symbol_idx = 0; symbol_index.clear(); }

	virtual void visitSymbol(TIntermSymbol* node);
	inline uint32_t getSymbolIndex(XXH32_hash_t symbol_hash) { return symbol_index.find(symbol_hash)->second; };
private:
	int symbol_idx = 0;
	std::unordered_map<XXH32_hash_t, uint32_t>symbol_index;
};

//  for each ast
//		generate tree hash nodes
//		combine it to graph

//symbol hash vec3_symbolname

// global hash graph node is linked by topology hash
// hash tree node is linked by node hash

class CASTHashTreeBuilder : public TIntermTraverser
{
public:
	CASTHashTreeBuilder() :
		subscope_tranverser(&declared_symbols_id),
		TIntermTraverser(true, true, true, false) //
	{ }
	
	void preTranverse(TIntermediate* intermediate);

	//对于所有的赋值表达式，搜索其所有的输入参数，按出现顺序排序，记录in inout和out
	//对于main函数,加个define来转换
	
	// hash layout:
	// hash operator
	// hash left node
	// hash right node
	virtual bool visitBinary(TVisit, TIntermBinary* node);

	virtual bool visitUnary(TVisit, TIntermUnary* node);
	virtual bool visitAggregate(TVisit, TIntermAggregate* node);
	virtual bool visitSelection(TVisit, TIntermSelection* node);
	virtual void visitConstantUnion(TIntermConstantUnion* node);
	virtual void visitSymbol(TIntermSymbol* node);
	virtual bool visitLoop(TVisit, TIntermLoop* node);
	virtual bool visitBranch(TVisit, TIntermBranch* node);
	virtual bool visitSwitch(TVisit, TIntermSwitch* node);

private:
	bool constUnionBegin(const TIntermConstantUnion* const_untion, TBasicType basic_type, TString& inoutStr);
	void constUnionEnd(const TIntermConstantUnion* const_untion, TString& inoutStr);

	TString getArraySize(const TType& type);
	TString generateConstantUnionStr(const TIntermConstantUnion* node, const TConstUnionArray& constUnion);

	TString getTypeText(const TType& type, bool getQualifiers = true, bool getSymbolName = false, bool getPrecision = true);
	std::vector<CHashNode> tree_hash_nodes;
	std::unordered_map<XXH64_hash_t, uint32_t> hash_value_to_idx;

	// make sure the code block is not nest
	class CBuilderContext
	{
	public:
		// map from symbol to hashnode
		std::unordered_map<XXH64_hash_t, XXH64_hash_t>symbol_last_hashnode_map;

		// iterate linker objects
		bool is_iterate_linker_objects = false;
		bool is_second_level_function = false;

		struct SOpAssignContext
		{
			bool visit_output_symbols = false;
			bool visit_input_symbols = false;
		};
		SOpAssignContext op_assign_context;


		struct SNoAssignContext
		{
			// 0 input symbol
			// 1 output symbol
			// 2 inout symbol
			std::unordered_map<XXH32_hash_t, uint32_t>symbol_inout_hashmap;
		};
		SNoAssignContext no_assign_context;

		inline void scopeReset()
		{
			no_assign_context.symbol_inout_hashmap.clear();
			output_hash_value.clear();
			input_hash_value.clear();
		}

		inline void addUniqueHashValue(XXH64_hash_t hash, const TString& symbol_name)
		{
			// op assign scope
			{
				if (op_assign_context.visit_input_symbols)
				{
					for (uint32_t idx = 0; idx < input_hash_value.size(); idx++)
					{
						if (input_hash_value[idx] == hash)
						{
							return;
						}
					}

					input_hash_value.push_back(hash);
				}

				if (op_assign_context.visit_output_symbols)
				{
					assert_t(op_assign_context.visit_input_symbols == false);
					for (uint32_t idx = 0; idx < output_hash_value.size(); idx++)
					{
						if (output_hash_value[idx] == hash)
						{
							return;
						}
					}

					output_hash_value.push_back(hash);
				}
			}

			// non-assign scope
			if ((!op_assign_context.visit_input_symbols) && (!op_assign_context.visit_output_symbols) && (no_assign_context.symbol_inout_hashmap.size() != 0))
			{
				XXH32_hash_t symbol_name_inout_hash = XXH32(symbol_name.c_str(), symbol_name.length(), global_seed);
				auto iter = no_assign_context.symbol_inout_hashmap.find(symbol_name_inout_hash);
				if (iter != no_assign_context.symbol_inout_hashmap.end())
				{
					uint32_t symbol_inout_state = no_assign_context.symbol_inout_hashmap[symbol_name_inout_hash];
					if (symbol_inout_state == 2 || symbol_inout_state == 1) // output symbols
					{
						for (uint32_t idx = 0; idx < output_hash_value.size(); idx++)
						{
							if (output_hash_value[idx] == hash)
							{
								return;
							}
						}
						output_hash_value.push_back(hash);
					}
					else if (symbol_inout_state == 2 || symbol_inout_state == 0) // input symbols
					{
						for (uint32_t idx = 0; idx < input_hash_value.size(); idx++)
						{
							if (input_hash_value[idx] == hash)
							{
								return;
							}
						}
						input_hash_value.push_back(hash);
					}
				}
			}
		}

		inline std::vector<XXH64_hash_t>& getOutputHashValues() { return output_hash_value; }
		inline std::vector<XXH64_hash_t>& getInputHashValues() { return input_hash_value; }

	private:
		// input symbol hash value and output symbol hash value
		std::vector<XXH64_hash_t>output_hash_value;
		std::vector<XXH64_hash_t>input_hash_value;
	};

	CBuilderContext builder_context;

	std::vector<XXH64_hash_t> hash_value_stack;

	std::set<long long> declared_symbols_id;


	TSubScopeTraverser subscope_tranverser;
	TSymbolScopeTraverser symbol_scope_traverser;

	CScopeSymbolNameTraverser scope_symbol_traverser;
#if TANGRAM_DEBUG
	TAstToGLTraverser debug_traverser;
#endif
};




