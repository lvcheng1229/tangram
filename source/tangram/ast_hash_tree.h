#pragma once
#include "ast_tranversar_private.h"
#include "xxhash.h"

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
	std::vector<uint64_t> inout_hash_nodes; //inout hash node indices
	std::vector<uint64_t> out_hash_nodes;  //output hash node indices
};


class CGlobalHashNode
{
public:
	std::vector<CHashNode> global_hash_nodes;
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
		TIntermTraverser(true, false, false, false) //
	{ }
	
	//对于所有的赋值表达式，搜索其所有的输入参数，按出现顺序排序，记录in inout和out
	//对于main函数,加个define来转换
	
	// hash layout:
	// hash operator
	// hash left node
	// hash right node
	virtual bool visitBinary(TVisit, TIntermBinary* node);

	//virtual bool visitUnary(TVisit, TIntermUnary* node);
	//virtual bool visitAggregate(TVisit, TIntermAggregate* node);
	//virtual bool visitSelection(TVisit, TIntermSelection* node);
	//virtual void visitConstantUnion(TIntermConstantUnion* node);
	virtual void visitSymbol(TIntermSymbol* node);
	//virtual bool visitLoop(TVisit, TIntermLoop* node);
	//virtual bool visitBranch(TVisit, TIntermBranch* node);
	//virtual bool visitSwitch(TVisit, TIntermSwitch* node);


private:
	TString getTypeText(const TType& type, bool getQualifiers = true, bool getSymbolName = false, bool getPrecision = true);
	std::vector<CHashNode> tree_hash_nodes;
	std::unordered_map<XXH64_hash_t, uint32_t> hash_value_to_idx;

	struct CBuilderContext
	{
		std::vector<XXH64_hash_t>input_hash_nodes;
		std::vector<XXH64_hash_t>output_hash_nodes;
		std::vector<XXH64_hash_t>inout_hash_nodes;

		std::unordered_map<XXH64_hash_t, XXH64_hash_t>symbol_last_hashnode_map;
	};

	CBuilderContext builder_context;

	std::vector<XXH64_hash_t> hash_value_stack;

	std::set<long long> declared_symbols_id;

};


class TASTHashTraverser : public TIntermTraverser {
public:
	TASTHashTraverser(CASTHashTreeBuilder* input_traverser) :
		custom_traverser(input_traverser),
		TIntermTraverser(true, false, false, false) //
	{ }

	virtual bool visitBinary(TVisit, TIntermBinary* node);
	virtual bool visitUnary(TVisit, TIntermUnary* node);
	virtual bool visitAggregate(TVisit, TIntermAggregate* node);
	virtual bool visitSelection(TVisit, TIntermSelection* node);
	virtual void visitConstantUnion(TIntermConstantUnion* node);
	virtual void visitSymbol(TIntermSymbol* node);
	virtual bool visitLoop(TVisit, TIntermLoop* node);
	virtual bool visitBranch(TVisit, TIntermBranch* node);
	virtual bool visitSwitch(TVisit, TIntermSwitch* node);

public:
	CASTHashTreeBuilder* custom_traverser;
};




