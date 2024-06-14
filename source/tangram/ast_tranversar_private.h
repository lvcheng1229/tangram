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

class TInvalidShaderTraverser : public TIntermTraverser {
public:
    TInvalidShaderTraverser()
        :is_invalid_shader(true), TIntermTraverser(true, true, true, false) {};

    virtual bool visitBinary(TVisit, TIntermBinary* node);
    virtual void visitSymbol(TIntermSymbol* node);

    inline bool getIsInvalidShader() { return is_invalid_shader; }
private:
    bool is_invalid_shader;
};

class TSymbolScopeTraverser : public TIntermTraverser {
public:
    TSymbolScopeTraverser() : TIntermTraverser(true, true, true, false) {}

    virtual bool visitBinary(TVisit, TIntermBinary* node);
    virtual void visitSymbol(TIntermSymbol* node);

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
    virtual bool visitUnary(TVisit, TIntermUnary* node) { subscope_max_line = (std::max)(subscope_max_line, node->getLoc().line); return true; }
    virtual bool visitAggregate(TVisit, TIntermAggregate* node) { subscope_max_line = (std::max)(subscope_max_line, node->getLoc().line); return true; }
    virtual bool visitSelection(TVisit, TIntermSelection* node);
    virtual void visitConstantUnion(TIntermConstantUnion* node) { subscope_max_line = (std::max)(subscope_max_line, node->getLoc().line); return ; }
    virtual void visitSymbol(TIntermSymbol* node);
    virtual bool visitLoop(TVisit, TIntermLoop* node);
    virtual bool visitBranch(TVisit, TIntermBranch* node) { subscope_max_line = (std::max)(subscope_max_line, node->getLoc().line); return true; }
    virtual bool visitSwitch(TVisit, TIntermSwitch* node);

    inline void resetSubScopeMaxLine() { subscope_max_line = 0; subscope_symbols.clear(); };
    inline std::unordered_map<int, TIntermSymbol*>& getSubScopeSymbols() { return subscope_symbols; }
    inline int getSubScopeMaxLine() { return subscope_max_line; };

private:
    const std::set<long long>* declared_symbols_id;
    int subscope_max_line;
    std::unordered_map<int, TIntermSymbol*> subscope_symbols;
};

class TLoopHeaderTraverser : public TIntermTraverser {
public:
    TLoopHeaderTraverser(const std::set<long long>* input_declared_symbols_id)
        :declared_symbols_id(input_declared_symbols_id),
        TIntermTraverser(true, true, true, false) {};

    virtual bool visitBinary(TVisit, TIntermBinary* node);
    virtual void visitSymbol(TIntermSymbol* node);
    virtual bool visitLoop(TVisit, TIntermLoop* node);
    
    inline void resetTraverser() { loop_header_symbols.clear(); };
    inline std::unordered_map<int, TIntermSymbol*>& getLoopHeaderSymbols() { return loop_header_symbols; }

private:
    const std::set<long long>* declared_symbols_id;
    std::unordered_map<int, TIntermSymbol*> loop_header_symbols;
};


class TAstToGLTraverser : public TIntermTraverser {
public:
    TAstToGLTraverser() :
        subscope_tranverser(&declared_symbols_id),
        loop_header_tranverser(&declared_symbols_id),
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
    void emitTypeConvert(TVisit visit, TIntermUnary* node, const TString& unaryName, const TString& vecName, bool onlyConvertVec = false);

    void constUnionBegin(const TIntermConstantUnion* const_untion, TBasicType basic_type);
    void constUnionEnd(const TIntermConstantUnion* const_untion);

    void declareSubScopeSymbol();
    void outputConstantUnion(const TIntermConstantUnion* node, const TConstUnionArray& constUnion);

    TString getTypeText(const TType& type, bool getQualifiers = true, bool getSymbolName = false, bool getPrecision = true);
    TString getArraySize(const TType& type);

protected:
    bool enable_line_feed_optimize = false;
    bool enable_white_space_optimize = true;
    bool ignore_medium_presion_out = true;
    //bool en

    struct SParserContext
    {
        bool is_vector_swizzle = false;
        bool is_subvector_scalar = false;
        bool is_in_loop_header = false;
        int component_func_visit_index = 0;
    };

    SParserContext parser_context;

    std::string code_buffer;
    std::set<long long> declared_symbols_id;

    TSymbolScopeTraverser symbol_scope_traverser;
    TSubScopeTraverser subscope_tranverser;
    TLoopHeaderTraverser loop_header_tranverser;
};


