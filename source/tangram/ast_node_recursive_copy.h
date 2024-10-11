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
	//virtual bool visitBinary(TVisit, TIntermBinary* node);
	//virtual bool visitUnary(TVisit, TIntermUnary* node);
	//virtual bool visitAggregate(TVisit, TIntermAggregate* node);
	//virtual bool visitSelection(TVisit, TIntermSelection* node);
	//virtual void visitConstantUnion(TIntermConstantUnion* node);
	//virtual void visitSymbol(TIntermSymbol* node);
	//virtual bool visitLoop(TVisit, TIntermLoop* node);
	//virtual bool visitBranch(TVisit, TIntermBranch* node);
	//virtual bool visitSwitch(TVisit, TIntermSwitch* node);

	TIntermNode* binaryCopy(TIntermBinary* node);
private:
	
	TIntermNode* copy_result;

	std::allocator<char> global_allocator;
};