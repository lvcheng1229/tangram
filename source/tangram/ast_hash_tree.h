#pragma once
#include "ast_tranversar_private.h"
#include "xxhash.h"

// ast symbol hash string layout example
// Type Hash + Path Hash
// Path Hash: BM0M1EM0M1 : begine-> mid 0-> mid 1-> end-> mid 0-> mid 1 -> input symbol list index + output symbol list index view(view it as function)

// uniform buffer struct hash: layout + type + name(layout(std140) uniform pb0View)
// uniform buffer struct member hash: struct hash value + offset
// uniform hash: todo
// other linker object: location + type(layout(location = 5) in vec4)

struct SSymbolHash
{
	inline uint32_t getMemSize() const { return sizeof(XXH64_hash_t) * (1); };

	XXH64_hash_t symbol_hash_value;
};

// ast node hash string layout
// [64bit] current node hash value
// [16bit] child node num
// [64bit] child node 1 xxhash64 value
// [64bit] child node 2 xxhash64 value
// [64bit] ......

struct SNodeHash
{
	inline uint32_t getMemSize() { return sizeof(XXH64_hash_t) * (1 + linked_input_hashed_nodes.size() + input_symbols.size() + output_symbols.size() ); };

	XXH64_hash_t hash_value;

	std::vector<XXH64_hash_t> linked_input_hashed_nodes;
	//std::vector<XXH64_hash_t> linked_output_hashed_nodes;

	std::vector<XXH64_hash_t> input_symbols;
	std::vector<XXH64_hash_t> output_symbols;
};

struct CAbstractFunctionNode
{
public:
private:
	XXH64_hash_t hash_value;

	std::vector<CAbstractFunctionNode*> input_function_nodes;
	std::vector<CAbstractFunctionNode*> inout_function_nodes;
	std::vector<CAbstractFunctionNode*> out_function_nodes;
};

#define HASH_NODE_MEM_BLOCK_SIZE (512 * 1024 * 1024)
class CGlobalHashNodeMannager
{
public:
	CGlobalHashNodeMannager();

	void* allocHashedNodeMem(int size);
	void addSymbolNode(XXH64_hash_t hash, const SSymbolHash& symbol_hash);

	void getInputSymbolHashValues(XXH64_hash_t hash_value, XXH64_hash_t* out_hash_values, int& num);

private:
	std::unordered_map<XXH64_hash_t, SNodeHash*> global_hashed_nodes;
	std::unordered_map<XXH64_hash_t, SSymbolHash*> global_hashed_symbols;

	uint32_t mem_size;
	uint32_t mem_offset;

	void* global_hashed_node_mem;
};

class CHashedNode
{
public:
};

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
	std::set<long long> declared_symbols_id;
	TVector<XXH64_hash_t> path;
	std::unordered_map<long long, XXH64_hash_t> node_id_to_hash;

	std::vector<long long> global_hash_values;
	std::vector<uint32_t> hash_value_path;

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




