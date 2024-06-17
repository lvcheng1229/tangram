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
	inline uint32_t getMemSize() { return sizeof(XXH64_hash_t) * (1); };

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
	inline uint32_t getMemSize() { return sizeof(XXH64_hash_t) * (1 + linked_input_hashed_nodes.size() + linked_output_hashed_nodes.size() + input_symbols.size() + output_symbols.size() ); };

	XXH64_hash_t hash_value;

	std::vector<XXH64_hash_t> linked_input_hashed_nodes;
	std::vector<XXH64_hash_t> linked_output_hashed_nodes;

	std::vector<SSymbolHash> input_symbols;
	std::vector<SSymbolHash> output_symbols;
};

#define HASH_NODE_MEM_BLOCK_SIZE (512 * 1024 * 1024)
class CGlobalHashNodeMannager
{
public:
	CGlobalHashNodeMannager();

	void* allocHashedNodeMem(int size);
	
private:
	uint32_t mem_size;
	uint32_t mem_offset;

	void* global_hashed_node_mem;

	std::unordered_map<XXH64_hash_t, SNodeHash*> global_hashed_nodes;
};

class CHashedNode
{
public:
};

class CASTHashTreeBuilder
{
public:
};


class TASTHashTraverser : public TIntermTraverser {
public:
	TASTHashTraverser(TIntermTraverser* input_traverser) :
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
	TIntermTraverser* custom_traverser;
};


