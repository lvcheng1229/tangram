#pragma once
#include "ast_tranversar.h"
#include "xxhash.h"

// 第 i个输入symbol所对应的上个节点的第j个输出
struct CHashEdge
{
	std::map<uint32_t, uint32_t> variable_source_index;
};

// higher uint16 source verttex index
// lower uint16 target vertex index
// edge map
// 
// 高 16 bit表示 输入顶点的全局索引，低16bit表示输出顶点的全局索引
using CHashVaiableMap = std::map <uint32_t, CHashEdge>;


// record the symbol name in a seperate structure
struct CHashNode
{
	CHashNode() :
		weight(1.0),
		hash_value(0),
		combined_hash_value(0),
		should_rename(true){}

	float weight;

#if TANGRAM_DEBUG
	TString debug_string;
#endif

	XXH64_hash_t hash_value;
	XXH64_hash_t combined_hash_value; // combined with in out hash value

	TIntermNode* interm_node;

	size_t graph_vtx_idx; 

	// 如果不能重命名，需要记录symbol的名字 (如 sampler texture和 uniform，对于unform buffer 要记录struct的名字), 因为 glsl 通过名字来设置索引
	bool should_rename; // 部分变量不可重命名比如 sample2D ps1,uniform 变量，因为这些变量依靠名字来索引
	bool is_ub_member;
	TString symbol_name; //如果link symbol node不可重命名，则需要记录其名字

	// 对于uniform buffer特殊处理
	//bool is_uniform_buffer;
	//uint16_t ub_offset;
	//uint16_t ub_member_size;

	std::vector<uint64_t> input_hash_nodes; //input hash node indices
	std::set<uint64_t> out_hash_nodes;

	// for variable rename
	std::vector<XXH64_hash_t> ordered_input_symbols_hash;
	std::unordered_map<XXH64_hash_t, uint32_t> ipt_symbol_name_order_map; // 表示某Hash值的Symbol是当前节点的第几个输入

	// 表示某 Hash值的 Symbol 是当前节点的第几个输出，用于下一个节点构建 ipt_symbol_name_order_map
	std::unordered_map<XXH64_hash_t, uint32_t> opt_symbol_name_order_map;

	//std::vector<uint64_t> inout_hash_nodes; //inout hash node indices
	//std::vector<uint64_t> out_hash_nodes;  //output hash node indices
};

class CScopeSymbolNameTraverser : public TIntermTraverser
{
public:
	CScopeSymbolNameTraverser() :
		TIntermTraverser(true, true, true, false){ }

	inline void reset() { symbol_idx = 0; symbol_index.clear(); }

	virtual void visitSymbol(TIntermSymbol* node);
	inline uint32_t getSymbolIndex(XXH32_hash_t symbol_hash) { return symbol_index.find(symbol_hash)->second; };
	
	// 返回symbol map， symbol map是所有符号的索引，包括输入符号和输出符号
	inline std::unordered_map<XXH32_hash_t, uint32_t>& getSymbolMap() { return symbol_index; };

	inline std::vector<XXH32_hash_t>& getSymbolHashOrdered() { return symbol_hash_ordered; };
private:
	int symbol_idx = 0;
	std::vector<XXH32_hash_t> symbol_hash_ordered;
	std::unordered_map<XXH32_hash_t, uint32_t>symbol_index;
};


//symbol hash vec3_symbolname

// global hash graph node is linked by topology hash
// hash tree node is linked by node hash

class CASTHashTreeBuilder : public TIntermTraverser
{
public:
	CASTHashTreeBuilder() :
		subscope_tranverser(&declared_symbols_id),
		TIntermTraverser(true, true, true, false) //
	{ 
		subscope_tranverser.enableVisitAllSymbols();
	}
	
	void preTranverse(TIntermediate* intermediate);

	virtual bool visitBinary(TVisit, TIntermBinary* node);
	virtual bool visitUnary(TVisit, TIntermUnary* node);
	virtual bool visitAggregate(TVisit, TIntermAggregate* node);
	virtual bool visitSelection(TVisit, TIntermSelection* node);
	virtual void visitConstantUnion(TIntermConstantUnion* node);
	virtual void visitSymbol(TIntermSymbol* node);
	virtual bool visitLoop(TVisit, TIntermLoop* node);
	virtual bool visitBranch(TVisit, TIntermBranch* node);
	virtual bool visitSwitch(TVisit, TIntermSwitch* node);

	std::vector<CHashNode>& getTreeHashNodes() { return tree_hash_nodes; };

private:
	bool constUnionBegin(const TIntermConstantUnion* const_untion, TBasicType basic_type, TString& inoutStr);
	void constUnionEnd(const TIntermConstantUnion* const_untion, TString& inoutStr);

	TString getArraySize(const TType& type);
	TString generateConstantUnionStr(const TIntermConstantUnion* node, const TConstUnionArray& constUnion);
	
	XXH64_hash_t generateSelectionHashValue(TString& out_hash_string, TIntermSelection* node);
	XXH64_hash_t generateLoopHashValue(TString& out_hash_string, TIntermLoop* node);
	XXH64_hash_t generateBranchHashValue(TString& out_hash_string, TIntermBranch* node);
	XXH64_hash_t generateSwitchHashValue(TString& out_hash_string);

	void generateHashNode(const TString& hash_string, XXH64_hash_t node_hash_value, TIntermNode* node);
	void generateSymbolInoutMap(int scope_min_line);
	void getAndUpdateInputHashNodes(CHashNode& func_hash_node);
	void outputDebugString(const CHashNode& func_hash_node);
	void updateLastAssignHashmap(const CHashNode& func_hash_node);

	void increAndDecrePath(TVisit visit, TIntermNode* current);

	TString getTypeText(const TType& type, bool getQualifiers = true, bool getSymbolName = false, bool getPrecision = true, bool getLayoutLocation = true);
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
			// 0 pre scope symbol
			// 1 post scope symbol
			// 2 pre-post scope symbol
			std::unordered_map<XXH32_hash_t, uint32_t>symbol_inout_hashmap;

			bool visit_assigned_symbols = false;
			bool visit_input_symbols = false;
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

			// non-assign scope example: loop / switch
			if ((!op_assign_context.visit_input_symbols) && (!op_assign_context.visit_output_symbols) && (no_assign_context.symbol_inout_hashmap.size() != 0))
			{
				XXH32_hash_t symbol_name_inout_hash = XXH32(symbol_name.c_str(), symbol_name.length(), global_seed);
				auto iter = no_assign_context.symbol_inout_hashmap.find(symbol_name_inout_hash);
				if (iter != no_assign_context.symbol_inout_hashmap.end())
				{
					uint32_t symbol_inout_state = no_assign_context.symbol_inout_hashmap[symbol_name_inout_hash];
					if ((symbol_inout_state == 2 || symbol_inout_state == 1) && no_assign_context.visit_assigned_symbols) // output( state = 1 ) symbols or inout symbols( state = 2 )
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
					
					if (symbol_inout_state == 2 || symbol_inout_state == 0) // input symbols( state = 0) or inout symbols( state = 2 )
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

		// 获得所有输出Symbol的HashValue，用于更新某个Symbol上一个被赋值是在哪个节点
		inline std::vector<XXH64_hash_t>& getOutputHashValues() { return output_hash_value; }
		inline std::vector<XXH64_hash_t>& getInputHashValues() { return input_hash_value; }

		// 用于变量重命名，标记某个symbol是第几个输出，第几个输入
		inline void buildInputOutputSymbolIndexMap(CHashNode& hash_node)
		{
			hash_node.ordered_input_symbols_hash = input_hash_value;
			for (int idx = 0; idx < input_hash_value.size(); idx++)
			{
				hash_node.ipt_symbol_name_order_map[input_hash_value[idx]] = idx;
			}

			for (int idx = 0; idx < output_hash_value.size(); idx++)
			{
				hash_node.opt_symbol_name_order_map[output_hash_value[idx]] = idx;
			}
		}

	private:
		// input symbol hash value and output symbol hash value
		std::vector<XXH64_hash_t>output_hash_value;
		std::vector<XXH64_hash_t>input_hash_value;
	};

	CBuilderContext builder_context;

	int hash_value_stack_max_depth = 0;
	std::vector<XXH64_hash_t> hash_value_stack;

	std::set<long long> declared_symbols_id;

	TSubScopeTraverser subscope_tranverser;
	TSymbolScopeTraverser symbol_scope_traverser;

	CScopeSymbolNameTraverser scope_symbol_traverser;

	CHashVaiableMap hash_varible_map;

#if TANGRAM_DEBUG
	TAstToGLTraverser debug_traverser;
#endif
};




