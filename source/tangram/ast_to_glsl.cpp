#include "glslang_headers.h"
#include <assert.h>

#define assert_t(...) assert(__VA_ARGS__);

using namespace glslang;

class TTanGramTraverser : public TIntermTraverser {
public:
    TTanGramTraverser(TInfoSink& i) :
        TIntermTraverser(true, true, true, true), //
        infoSink(i), 
        extraOutput(NoExtraOutput)
    { }

    enum EExtraOutput {
        NoExtraOutput,
        BinaryDoubleOutput
    };
    void setDoubleOutput(EExtraOutput extra) { extraOutput = extra; }

    virtual bool visitBinary(TVisit, TIntermBinary* node);
    virtual bool visitUnary(TVisit, TIntermUnary* node);
    virtual bool visitAggregate(TVisit, TIntermAggregate* node);
    virtual bool visitSelection(TVisit, TIntermSelection* node);
    virtual void visitConstantUnion(TIntermConstantUnion* node);
    virtual void visitSymbol(TIntermSymbol* node);
    virtual bool visitLoop(TVisit, TIntermLoop* node);
    virtual bool visitBranch(TVisit, TIntermBranch* node);
    virtual bool visitSwitch(TVisit, TIntermSwitch* node);

    TInfoSink& infoSink;
protected:
    TTanGramTraverser(TTanGramTraverser&);
    TTanGramTraverser& operator=(TTanGramTraverser&);

    bool enable_size_optimize = false;
    EExtraOutput extraOutput;
    std::string code_buffer;
};

bool TTanGramTraverser::visitBinary(TVisit, TIntermBinary* node)
{
    return false;
}

bool TTanGramTraverser::visitUnary(TVisit, TIntermUnary* node)
{
    return false;
}

bool TTanGramTraverser::visitAggregate(TVisit visit, TIntermAggregate* node)
{
    switch (node->getOp()) 
    {
    case EOpFunction:
    {
        if (visit == EvPreVisit)
        {
            assert_t(node->getBasicType() == EbtVoid);
            code_buffer.append(node->getType().getBasicString());
            code_buffer.insert(code_buffer.end(), ' ');
            code_buffer.insert(code_buffer.end(), node->getName().begin(), node->getName().end());
        }
        else if(visit == EvPostVisit)
        {
            code_buffer.insert(code_buffer.end(), '}');
            if (!enable_size_optimize)
            {
                code_buffer.insert(code_buffer.end(), '\n');
            }
        }
        
        break;
    }
    }
    return true;
}

bool TTanGramTraverser::visitSelection(TVisit, TIntermSelection* node)
{
    return false;
}

void TTanGramTraverser::visitConstantUnion(TIntermConstantUnion* node)
{
}

void TTanGramTraverser::visitSymbol(TIntermSymbol* node)
{
}

bool TTanGramTraverser::visitLoop(TVisit, TIntermLoop* node)
{
    return false;
}

bool TTanGramTraverser::visitBranch(TVisit, TIntermBranch* node)
{
    return false;
}

bool TTanGramTraverser::visitSwitch(TVisit, TIntermSwitch* node)
{
    return false;
}



void ast_to_glsl(const char* const* shaderStrings, const int* shaderLengths)
{
    int shader_size = *shaderLengths;
    int sharp_pos = 0;
    for (int idx = 0; idx < shader_size; idx++)
    {
        if (((const char*)shaderStrings)[idx] == '#')
        {
            if (((const char*)shaderStrings)[idx + 1] == 'v')
            {
                sharp_pos = idx;
                break;
            }
        }
    }

    int main_size = shader_size - sharp_pos;
    if (main_size <= 0)
    {
        return;
    }

	const int defaultVersion = 100;
	const EProfile defaultProfile = ENoProfile;
	const bool forceVersionProfile = false;
	const bool isForwardCompatible = false;

    //https://raytracing-docs.nvidia.com/mdl/api/mi_neuray_example_df_vulkan.html

    EShMessages messages = EShMsgCascadingErrors;
    messages = static_cast<EShMessages>(messages | EShMsgAST);
    messages = static_cast<EShMessages>(messages | EShMsgHlslLegalization);

    std::string code((const char*)shaderStrings + sharp_pos, main_size);
    const char* shader_strings = code.data();
    const int shader_lengths = static_cast<int>(code.size());

    InitializeProcess();

	glslang::TShader shader(EShLangFragment);
	shader.setStringsWithLengths(&shader_strings, &shader_lengths, 1);
    shader.setEntryPoint("main");
	shader.parse(GetDefaultResources(), defaultVersion, isForwardCompatible, messages);
	TIntermediate* intermediate = shader.getIntermediate();

    TInfoSink info_sink;
    TTanGramTraverser tangram_tranverser(info_sink);
    
	intermediate->getTreeRoot()->traverse(&tangram_tranverser);


    
    FinalizeProcess();
}

