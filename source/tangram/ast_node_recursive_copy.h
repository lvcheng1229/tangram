#pragma once

#include <assert.h>
#include <set>
#include <unordered_map>
#include <memory>

#include "glslang_headers.h"
#include "xxhash.h"

using namespace glslang;

enum  ESymbolState
{
	SS_None = 0,
	SS_InputSymbol = (1 << 0),
	SS_OutputSymbol = (1 << 1),
	SS_InoutSymbol = (SS_InputSymbol | SS_OutputSymbol),
};

enum  ESymbolScopeType
{
	SST_None = 0,
	SST_LinkerNode = 1 << 1, 
	SST_AssignUnit = 1 << 2,
	SST_NoAssign = 1 << 3,
};

class TTanGramIntermSymbol : public TIntermSymbol
{
public:
	TTanGramIntermSymbol(long long i, const TString& n, const TType& t) :TIntermSymbol(i, n, t) , asinput_index(-1), asoutut_index(-1) {};

	//std::string generateCode();

	inline void setScopeType(ESymbolScopeType sst) { scope_type = sst; };
	inline ESymbolScopeType getScopeType()const { return scope_type; };

	inline void setSymbolState(ESymbolState st) { symbol_state = st; };
	inline ESymbolState getSymbolState()const { return symbol_state; };

	inline void setAsInputIndex(int32_t index) { asinput_index = index; };
	inline int32_t getAsInputIndex()const { return asinput_index; };

	inline void setAsOutputIndex(int32_t index) { asoutut_index = index; };
	inline int32_t getAsOutputIndex()const { return asoutut_index; };

	inline bool isLinkerNode()const { return scope_type == SST_LinkerNode; }

	inline TString getSymbolName(std::vector<std::string>* ipt_variable_names, std::vector<std::string>* opt_variable_names)
	{
		if (scope_type == SST_LinkerNode)
		{
			assert(asoutut_index == 0);
			return TString((*opt_variable_names)[0].c_str());
		}
		else if (scope_type == SST_AssignUnit)
		{
			assert(symbol_state != SS_InoutSymbol);
			if (symbol_state & SS_InputSymbol)
			{
				return TString((*ipt_variable_names)[asinput_index].c_str());
			}
			else
			{
				assert(asoutut_index == 0);
				return TString((*opt_variable_names)[0].c_str());
			}
		}
		else if (scope_type == SST_NoAssign)
		{
			assert(symbol_state != SS_InoutSymbol);
			if (symbol_state & SS_InputSymbol)
			{
				return TString((*ipt_variable_names)[asinput_index].c_str());
			}
			else
			{
				return TString((*opt_variable_names)[asoutut_index].c_str());
			}
		}
		else
		{
			assert(false); //todo:
		}
		return TString();
	}

private:

	int32_t asinput_index; //symbol as input index
	int32_t asoutut_index; //symbol as output index

	ESymbolScopeType scope_type;
	ESymbolState symbol_state;
};

template<typename T>
struct CastType { using Type = T; };

template<>
struct CastType<TIntermSymbol> { using Type = TTanGramIntermSymbol; };

template<typename T>
CastType<T>::Type* getTanGramNode(T* type) { return static_cast<CastType<T>::Type*>(type); }

class CGlobalAstNodeRecursiveCopy : public glslang::TIntermTraverser
{
public:
	CGlobalAstNodeRecursiveCopy() :glslang::TIntermTraverser(true, true, true) { ast_node_allocator = new TPoolAllocator; }
	~CGlobalAstNodeRecursiveCopy() { delete ast_node_allocator; };

	virtual bool visitBinary(TVisit, TIntermBinary* node);
	virtual bool visitUnary(TVisit, TIntermUnary* node);
	virtual bool visitAggregate(TVisit, TIntermAggregate* node);
	virtual bool visitSelection(TVisit, TIntermSelection* node);
	virtual void visitConstantUnion(TIntermConstantUnion* node);
	virtual void visitSymbol(TIntermSymbol* node);
	virtual bool visitLoop(TVisit, TIntermLoop* node);
	virtual bool visitBranch(TVisit, TIntermBranch* node);
	virtual bool visitSwitch(TVisit, TIntermSwitch* node);

	//void setCopyContext(ESymbolScopeType scope_type)
	//{
	//	scope_context.scope_type = scope_type;
	//}

	TIntermNode* getCopyedNodeAndResetContextNoAssign(std::vector<XXH64_hash_t>& input_hash_nodes, std::vector<XXH64_hash_t>& output_hash_nodes);
	TIntermNode* getCopyedNodeAndResetContextAssignNode(XXH64_hash_t output_hash_node);
	TIntermNode* getCopyedNodeAndResetContextLinkNode();

private:
	TIntermNode* getCopyedNodeAndResetContextImpl();

	inline TIntermNode* getAndPopCopyedNode()
	{
		TIntermNode* copyed_node = node_stack_context.back();
		node_stack_context.pop_back();
		return copyed_node;
	}

	// clone->deep copy
	inline void shadowCopyIntermTyped(TIntermTyped* dst, const TIntermTyped* const src)
	{
		dst->setType(*src->getType().clone());
	}

	inline void shadowCopyIntermOperator(TIntermOperator* dst, const TIntermOperator* const src)
	{
		shadowCopyIntermTyped(dst, src);
		dst->setOperationPrecision(src->getOperationPrecision());
	}

	template<typename T, typename... C>
	T* allocateAndConstructIntermOperator(T* src_node, C... args)
	{
		char* node_mem = reinterpret_cast<char*>(global_allocator.allocate(sizeof(T)));
		T* new_node = new(node_mem)T(args...);
		shadowCopyIntermOperator(new_node, src_node);
		return new_node;
	}

	inline void setGlobalASTPool()
	{
		previous_allocator = &GetThreadPoolAllocator();
		SetThreadPoolAllocator(ast_node_allocator);
	}

	inline void resetGlobalASTPool()
	{
		SetThreadPoolAllocator(previous_allocator);
	}

	//struct SCopeContext
	//{
	//	ESymbolScopeType scope_type;
	//
	//	inline void reset()
	//	{
	//		scope_type = ESymbolScopeType::SST_None;
	//	}
	//};
	//SCopeContext scope_context;

	TPoolAllocator* previous_allocator;
	TPoolAllocator* ast_node_allocator;

	std::vector<TIntermNode*> node_stack_context;
	std::allocator<char> global_allocator;

	std::vector<TTanGramIntermSymbol*> symbol_nodes;

	long long symbol_index;
	std::unordered_map<long long, long long> symbol_id2index; //½« Symbol µÄ ID Ó³Éäµ½ symbol index
};