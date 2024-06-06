#include "glslang_headers.h"
#include <assert.h>
#include <set>

#define assert_t(...) assert(__VA_ARGS__);

using namespace glslang;

class TTanGramTraverser : public TIntermTraverser {
public:
    TTanGramTraverser() :
        TIntermTraverser(true, true, true, false) //
    { }

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

private:
    void outputConstantUnion(const TIntermTyped* node, const TConstUnionArray& constUnion);
    TString getTypeText(const TType& type);

protected:
    bool enable_line_feed_optimize = false;
    bool enable_white_space_optimize = true;
    bool ignore_medium_presion_out = true;

    std::string code_buffer;
    std::set<long long> declared_symbols_id;
};

void TTanGramTraverser::preTranverse(TIntermediate* intermediate)
{
    code_buffer.append("#version ");
    code_buffer.append(std::to_string(intermediate->getVersion()).c_str());
    code_buffer.append(" es");
}


TString TTanGramTraverser::getTypeText(const TType& type)
{
    TString type_string;

    const auto appendStr = [&](const char* s) { type_string.append(s); };
    const auto appendUint = [&](unsigned int u) { type_string.append(std::to_string(u).c_str()); };
    const auto appendInt = [&](int i) { type_string.append(std::to_string(i).c_str()); };

    // get qualifiers
    {
        const TQualifier& qualifier = type.getQualifier();
        if (qualifier.hasLayout())
        {
            appendStr("layout(");
            if (qualifier.hasPacking())
            {
                appendStr(TQualifier::getLayoutPackingString(qualifier.layoutPacking));
            }
            appendStr(")");
            appendStr(type.getStorageQualifierString());
            appendStr(" ");
        }
    }

    if (type.isArray())
    {
        assert_t(false);
    }

    if (type.isParameterized())
    {
        assert_t(false);
    }

    if (type.isParameterized())
    {
        assert_t(false);
    }

    if (type.isMatrix())
    {
        assert_t(false);
    }

    
    if (type.getQualifier().precision != EpqNone && (!(ignore_medium_presion_out && type.getQualifier().precision == EpqMedium)))
    {
        appendStr(type.getPrecisionQualifierString());
        appendStr(" ");
    }

    if (type.isVector())
    {
        appendStr("vec");
        appendInt(type.getVectorSize());
    }

    if (type.getBasicType() == EbtBlock)
    {
        type_string.append(type.getTypeName());
    }

    if (type.isStruct() && type.getStruct())
    {
        const TTypeList* structure = type.getStruct();
        appendStr("{");
        
        for (size_t i = 0; i < structure->size(); ++i) 
        {
            TType* struct_mem_type = (*structure)[i].type;
            bool hasHiddenMember = struct_mem_type->hiddenMember();
            assert_t(hasHiddenMember == false);
            type_string.append(getTypeText(*struct_mem_type));
            appendStr(";");
            if (!enable_line_feed_optimize)
            {
                appendStr("\n");
            }
        }
        appendStr("}");
    }

    return type_string;
}

bool TTanGramTraverser::visitBinary(TVisit visit, TIntermBinary* node)
{
    TOperator node_operator = node->getOp();
    if (visit == EvPreVisit)
    {
        switch (node_operator)
        {
        case EOpAssign:
        {
            bool is_declared = false;
            if (node->getLeft()->getAsSymbolNode() != nullptr)
            {
                TIntermSymbol* symbol_node = node->getLeft()->getAsSymbolNode();
                long long symbol_id = symbol_node->getId();
                auto iter = declared_symbols_id.find(symbol_id);
                if (iter == declared_symbols_id.end())
                {
                    declared_symbols_id.insert(symbol_id);
                }
                else
                {
                    is_declared = true;
                }
            }

            if (is_declared == false)
            {
                code_buffer.append(getTypeText(node->getType()));
                code_buffer.append(" ");
            }

            break;
        }
        case EOpIndexDirectStruct:
        {
            bool reference = node->getLeft()->getType().isReference();
            const TTypeList* members = reference ? node->getLeft()->getType().getReferentType()->getStruct() : node->getLeft()->getType().getStruct();
            int member_index = node->getRight()->getAsConstantUnion()->getConstArray()[0].getIConst();
            const TString& index_direct_struct_str = (*members)[member_index].type->getFieldName();
            code_buffer.append(index_direct_struct_str);

            // no need to output the struct name
            node->setLeft(nullptr);
            node->setRight(nullptr);

            break;
        }
        case EOpSub:
        case EOpVectorSwizzle:
        {
            //do nothing
            break;
        }
        default:
        {
            assert_t(false);
            break;
        }
        }
        
    }
    else if (visit == EvInVisit)
    {
        switch (node_operator)
        {
        case EOpAssign:
        {
            if (!enable_white_space_optimize)
            {
                code_buffer.append(" ");
            }

            code_buffer.append("=");

            if (!enable_white_space_optimize)
            {
                code_buffer.append(" ");
            }

            break;
        }
        case EOpSub:
        {
            if (!enable_white_space_optimize)
            {
                code_buffer.append(" ");
            }

            code_buffer.append("-");

            if (!enable_white_space_optimize)
            {
                code_buffer.append(" ");
            }
            break;
        }
        case EOpVectorSwizzle:
        {
            code_buffer.append(".");
            break;
        }
        case EOpIndexDirectStruct:
        {
            // do nothing
            break;
        }
        default:
        {
            assert_t(false);
            break;
        }
        }
    }
    else if (visit == EvPostVisit)
    {
        switch (node_operator)
        {
        case EOpAssign:
        {
            code_buffer.append(";");;
            if (!enable_line_feed_optimize)
            {
                code_buffer.append("\n");
            }
            break;
        }
        case EOpIndexDirectStruct:
        case EOpSub:
        case EOpVectorSwizzle:
        {
            //do nothing
            break;
        }
        default:
        {
            assert_t(false);
            break;
        }
        };
        
    }

    return true;
}

bool TTanGramTraverser::visitUnary(TVisit, TIntermUnary* node)
{
    return false;
}

bool TTanGramTraverser::visitAggregate(TVisit visit, TIntermAggregate* node)
{
    TOperator node_operator = node->getOp();
    switch (node_operator)
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
        else if (visit == EvInVisit)
        {
            
        }
        else if(visit == EvPostVisit)
        {
            code_buffer.insert(code_buffer.end(), '}');
            if (!enable_line_feed_optimize)
            {
                code_buffer.insert(code_buffer.end(), '\n');
            }
        }
        
        break;
    }
    case EOpParameters:
    {
        if (visit == EvPreVisit)
        {
            assert_t(node->getSequence().size() == 0);
        }
        else if (visit == EvInVisit)
        {
            assert_t(false);
        }
        else if (visit == EvPostVisit)
        {
            code_buffer.append(") { ");
            if (!enable_line_feed_optimize)
            {
                code_buffer.insert(code_buffer.end(), '\n');
            }
        }
    }
    }
    return true;
}

bool TTanGramTraverser::visitSelection(TVisit, TIntermSelection* node)
{
    return false;
}

static char unionConvertToChar(int index)
{
    static const char const_indices[3] = { 'x','y','z' };
    return const_indices[index];
}

void TTanGramTraverser::outputConstantUnion(const TIntermTyped* node, const TConstUnionArray& constUnion)
{
    int size = node->getType().computeNumComponents();
    for (int i = 0; i < size; i++)
    {
        TBasicType const_type = constUnion[i].getType();
        switch (const_type)
        {
        case EbtInt:
        {
            code_buffer.insert(code_buffer.end(), unionConvertToChar(constUnion[i].getIConst()));
            break;
        }
        default:
        {
            assert_t(false);
            break;
        }
        }
    }
}


void TTanGramTraverser::visitConstantUnion(TIntermConstantUnion* node)
{
    outputConstantUnion(node, node->getConstArray());
}

void TTanGramTraverser::visitSymbol(TIntermSymbol* node)
{
    bool is_declared = false;

    {
        long long symbol_id = node->getId();
        auto iter = declared_symbols_id.find(symbol_id);
        if (iter == declared_symbols_id.end())
        {
            declared_symbols_id.insert(symbol_id);
        }
        else
        {
            is_declared = true;
        }
    }

    if (is_declared == false)
    {
        code_buffer.append(getTypeText(node->getType()));
    }

    code_buffer.append(node->getName());

    if (!node->getConstArray().empty())
    {
        assert_t(false);
    }
    else if (node->getConstSubtree())
    {
        assert_t(false);
    }
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

    TTanGramTraverser tangram_tranverser;
    
    TIntermAggregate* root_aggregate = intermediate->getTreeRoot()->getAsAggregate();
    if (root_aggregate != nullptr)
    {
        // tranverse linker object at first
        TIntermSequence& sequence = root_aggregate->getSequence();
        TIntermSequence sequnce_reverse;
        for (TIntermSequence::reverse_iterator sit = sequence.rbegin(); sit != sequence.rend(); sit++)
        {
            sequnce_reverse.push_back(*sit);
        }
        sequence = sequnce_reverse;
    }
    else
    {
        assert_t(false);
    }

    tangram_tranverser.preTranverse(intermediate);
	intermediate->getTreeRoot()->traverse(&tangram_tranverser);


    
    FinalizeProcess();
}

