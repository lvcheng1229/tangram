#pragma once

#include <assert.h>
#include <set>
#include <unordered_map>
#include <memory>
#include "glslang_headers.h"

using namespace glslang;

class CGlobalAstNodeRecursiveCopy : public glslang::TIntermTraverser
{
public:
	CGlobalAstNodeRecursiveCopy() { ast_node_allocator = new TPoolAllocator; }
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

	inline TIntermNode* getAndPopCopyedNode()
	{
		TIntermNode* copyed_node = node_stack_context.back();
		node_stack_context.pop_back();
		return copyed_node;
	}
private:

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

	TPoolAllocator* previous_allocator;
	TPoolAllocator* ast_node_allocator;

	std::vector<TIntermNode*> node_stack_context;
	std::allocator<char> global_allocator;


};