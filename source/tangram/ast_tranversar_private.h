#pragma once
#include <assert.h>
#include <set>
#include <unordered_map>

#include "glslang_headers.h"
using namespace glslang;

#define assert_t(...) assert(__VA_ARGS__);

#define NODE_VISIT_FUNC(PRE,IN,POST)\
{if (visit == EvPreVisit){\
PRE;\
}else if (visit == EvInVisit){\
IN;\
}else if (visit == EvPostVisit){\
POST;\
}break;}\

class TSymbolScopeTraverser : public TIntermTraverser {
public:
    TSymbolScopeTraverser() :
        TIntermTraverser(true, true, true, false) //
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

    inline std::unordered_map<long long, int>* getSymbolMaxLine() { return &symbol_max_line; };
private:
    std::unordered_map<long long, int> symbol_max_line;
};

class TSubScopeTraverser : public TIntermTraverser {
public:
    TSubScopeTraverser(const std::set<long long>* input_declared_symbols_id)
        :declared_symbols_id(input_declared_symbols_id),
        subscope_max_line(0),
        TIntermTraverser(true, true, true, false) {};

    virtual bool visitBinary(TVisit, TIntermBinary* node);
    virtual bool visitUnary(TVisit, TIntermUnary* node);
    virtual bool visitAggregate(TVisit, TIntermAggregate* node);
    virtual bool visitSelection(TVisit, TIntermSelection* node);
    virtual void visitConstantUnion(TIntermConstantUnion* node);
    virtual void visitSymbol(TIntermSymbol* node);
    virtual bool visitLoop(TVisit, TIntermLoop* node);
    virtual bool visitBranch(TVisit, TIntermBranch* node);
    virtual bool visitSwitch(TVisit, TIntermSwitch* node);

    inline void resetSubScopeMaxLine() { subscope_max_line = 0; };
    inline std::unordered_map<int, TIntermSymbol*>& getSubScopeSymbols() { return subscope_symbols; }
    inline int getSubScopeMaxLine() { return subscope_max_line; };

private:
    const std::set<long long>* declared_symbols_id;
    int subscope_max_line;
    std::unordered_map<int, TIntermSymbol*> subscope_symbols;
};

class TAstToGLTraverser : public TIntermTraverser {
public:
    TAstToGLTraverser() :
        subscope_tranverser(&declared_symbols_id),
        TIntermTraverser(true, true, true, false) //
    { }

    void preTranverse(TIntermediate* intermediate);
    inline const std::string& getCodeBuffer() { return code_buffer; };

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
    void declareSubScopeSymbol();
    void outputConstantUnion(const TIntermConstantUnion* node, const TConstUnionArray& constUnion);
    TString getTypeText(const TType& type, bool getQualifiers = true, bool getSymbolName = false, bool getPrecision = true);
    TString getArraySize(const TType& type);

protected:
    bool enable_line_feed_optimize = false;
    bool enable_white_space_optimize = true;
    bool ignore_medium_presion_out = true;

    struct SParserContext
    {
        bool is_vector_swizzle = false;
        bool is_vector_times_scalar = false;
    };

    SParserContext parser_context;

    std::string code_buffer;
    std::set<long long> declared_symbols_id;

    TSymbolScopeTraverser symbol_scope_traverser;
    TSubScopeTraverser subscope_tranverser;
};

