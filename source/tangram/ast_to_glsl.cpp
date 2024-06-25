#include "ast_tranversar_private.h"

//todo: 
// 1. fix bug xyzw.x -> to .x
// 2. mat fold highp vec3 _23=max(pc2_h[0].xyzw.xyz*dot(vec3(abs(_28.x),abs(_28.y),abs(_28.z)),vec3(0.300048828125,0.58984375,0.1099853515625)),vec3(0.))*1.;
// 3. use node path instead of swizzle context


#if TANGRAM_DEBUG
#include <fstream>
#include <iostream>
#endif

CSymbolNameMapper::CSymbolNameMapper()
{
    uppercase_offset = 65;  // 65 - 90
    lowercase_offset = 97;  // 97 - 122
    digital_offset = 48;    // 48 - 57

    candidate_char.resize(26 + 26 + 10);
    int global_offset = 0;
    for (int idx = 0; idx < 26; idx++)
    {
        candidate_char[global_offset] = char(uppercase_offset + idx);
        global_offset++;
    }

    for (int idx = 0; idx < 26; idx++)
    {
        candidate_char[global_offset] = char(lowercase_offset + idx);
        global_offset++;
    }

    for (int idx = 0; idx < 10; idx++)
    {
        candidate_char[global_offset] = char(digital_offset + idx);
        global_offset++;
    }
}

const TString& CSymbolNameMapper::getSymbolMappedName(long long symbol_id)
{
    auto iter = symbol_name_mapped.find(symbol_id);
    if (iter != symbol_name_mapped.end())
    {
        return iter->second;
    }
    else
    {
        symbol_name_mapped[symbol_id] = allocNextSymbolName();
        return symbol_name_mapped[symbol_id];
    }
}

const TString CSymbolNameMapper::allocNextSymbolName()
{
    if (symbol_index < 26)
    {
        TString symbol_name;
        symbol_name.resize(1);
        symbol_name[0] = char(symbol_index + uppercase_offset);
        symbol_index++;
        return symbol_name;
    }

    int first = symbol_index % 26;
    TString symbol_name;
    symbol_name.insert(symbol_name.end(),char(first + uppercase_offset));

    int var = first / 26;
    while (var != 0)
    {
        int char_index = (var % 66);
        symbol_name.insert(symbol_name.end(), candidate_char[char_index]);
        var = var / 66;
    }
    symbol_index++;
    return symbol_name;
}

void TAstToGLTraverser::preTranverse(TIntermediate* intermediate)
{
    code_buffer.append("#version ");
    code_buffer.append(std::to_string(intermediate->getVersion()).c_str());
    code_buffer.append(" es\n");

    const std::set<std::string>& extensions = intermediate->getRequestedExtensions();
    if (extensions.size() > 0) 
    {
        for (auto extIt = extensions.begin(); extIt != extensions.end(); ++extIt)
        {
            code_buffer.append("#extension ");
            code_buffer.append(*extIt);
            code_buffer.append(": require\n");
        }
    }

    code_buffer.append("precision mediump float;\n");
    code_buffer.append("precision highp int;\n");

    intermediate->getTreeRoot()->traverse(&symbol_scope_traverser);
}

void TAstToGLTraverser::declareSubScopeSymbol()
{
    auto symbols_max_line = symbol_scope_traverser.getSymbolMaxLine();
    int scope_max_line = subscope_tranverser.getSubScopeMaxLine();
    for (auto& iter : subscope_tranverser.getSubScopeSymbols())
    {
        TIntermSymbol* symbol_node = iter.second;
        auto symbol_map = symbols_max_line->find(symbol_node->getId());
        assert_t(symbol_map != symbols_max_line->end());
        int symbol_max_line = symbol_map->second;

        if (symbol_max_line > scope_max_line)
        {
            declared_symbols_id.insert(symbol_node->getId());
            code_buffer.append(getTypeText(symbol_node->getType()));
            code_buffer.append(" ");
            if (enable_symbol_name_optimization)
            {
                code_buffer.append(symbol_name_mapper.getSymbolMappedName(symbol_node->getId()));
            }
            else
            {
                code_buffer.append(symbol_node->getName());
            }
            code_buffer.append(";");
            if (!enable_line_feed_optimize) { code_buffer.append("\n"); }
        }
    }
}

TString TAstToGLTraverser::getTypeText(const TType& type, bool getQualifiers, bool getSymbolName, bool getPrecision)
{
    TString type_string;

    const auto appendStr = [&](const char* s) { type_string.append(s); };
    const auto appendUint = [&](unsigned int u) { type_string.append(std::to_string(u).c_str()); };
    const auto appendInt = [&](int i) { type_string.append(std::to_string(i).c_str()); };

    TBasicType basic_type = type.getBasicType();
    bool is_vec = type.isVector();
    bool is_blk = (basic_type == EbtBlock);
    bool is_mat = type.isMatrix();

    const TQualifier& qualifier = type.getQualifier();

    // build-in variable
    if (qualifier.storage >= EvqVertexId && qualifier.storage <= EvqFragStencil)
    {
        return TString();
    }

    bool should_output_precision_str = ((basic_type == EbtFloat) && type.getQualifier().precision != EpqNone && (!(ignore_medium_presion_out && type.getQualifier().precision == EpqMedium))) || basic_type == EbtSampler;
    
    if(getQualifiers)
    {
        
        if (qualifier.hasLayout())
        {
            appendStr("layout(");
            if (qualifier.hasAnyLocation())
            {
                appendStr(" location=");
                appendUint(qualifier.layoutLocation);
            }
            if (qualifier.hasPacking())
            {
                appendStr(TQualifier::getLayoutPackingString(qualifier.layoutPacking));
            }
            appendStr(")");
        }
        

        if (qualifier.flat)
        {
            appendStr(" flat");
            appendStr(" ");
        }
            
        //bool should_out_storage_qualifier = ((basic_type == EbtBlock) || (basic_type == EbtSampler));
        bool should_out_storage_qualifier = true;
        if (type.getQualifier().storage == EvqTemporary ||
            type.getQualifier().storage == EvqGlobal)
        {
            should_out_storage_qualifier = false;
        }

        if (should_out_storage_qualifier)
        {
            appendStr(type.getStorageQualifierString());
            appendStr(" ");
        }

        if (should_output_precision_str)
        {
            appendStr(type.getPrecisionQualifierString());
            appendStr(" ");
        }
    }
    
    if ((getQualifiers == false) && getPrecision)
    {
        if (should_output_precision_str)
        {
            appendStr(type.getPrecisionQualifierString());
            appendStr(" ");
        }
    }

    
    if (is_vec)
    {
        switch (basic_type)
        {
        case EbtDouble:
            appendStr("d");
            break;
        case EbtInt:
            appendStr("i");
            break;
        case EbtUint:
            appendStr("u");
            break;
        case EbtBool:
            appendStr("b");
            break;
        case EbtFloat:
        default:
            break;
        }
    }

    

    if (type.isParameterized())
    {
        assert_t(false);
    }


    if (is_mat)
    {
        appendStr("mat");

        int mat_raw_num = type.getMatrixRows();
        int mat_col_num = type.getMatrixCols();

        if (mat_raw_num == mat_col_num)
        {
            appendInt(mat_raw_num);
        }
        else
        {
            appendInt(mat_raw_num);
            appendStr("x");
            appendInt(mat_col_num);
        }
    }

    if (type.isVector())
    {
        appendStr("vec");
        appendInt(type.getVectorSize());
    }

  

    if ((!is_vec) && (!is_blk) && (!is_mat))
    {
        appendStr(type.getBasicTypeString().c_str());
    }

    if (basic_type == EbtBlock)
    {
        type_string.append(type.getTypeName());
    }

    if (getSymbolName)
    {
        appendStr(" ");
        appendStr(type.getFieldName().c_str());
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
            type_string.append(getTypeText(*struct_mem_type, false, true));
            if (struct_mem_type->isArray())
            {
                type_string.append(getArraySize(*struct_mem_type));
            }
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

TString TAstToGLTraverser::getArraySize(const TType& type)
{
    TString type_string;

    const auto appendStr = [&](const char* s) { type_string.append(s); };
    const auto appendUint = [&](unsigned int u) { type_string.append(std::to_string(u).c_str()); };
    const auto appendInt = [&](int i) { type_string.append(std::to_string(i).c_str()); };

    if (type.isArray())
    {
        const TArraySizes* array_sizes = type.getArraySizes();
        for (int i = 0; i < (int)array_sizes->getNumDims(); ++i)
        {
            int size = array_sizes->getDimSize(i);
            if (size == UnsizedArraySize && i == 0 && array_sizes->isVariablyIndexed())
            {
                assert_t(false);
            }
            else
            {
                if (size == UnsizedArraySize)
                {
                    assert_t(false);
                }
                else
                {
                    appendStr("[");
                    appendInt(array_sizes->getDimSize(i));
                    appendStr("]");
                }
            }
        }
    }
    return type_string;
}

bool TAstToGLTraverser::visitBinary(TVisit visit, TIntermBinary* node)
{
    TOperator node_operator = node->getOp();
    switch (node_operator)
    {
    case EOpAssign:
    {
        if (visit == EvPreVisit)
        {
            bool is_declared = false;
            TIntermSymbol* symbol_node = node->getLeft()->getAsSymbolNode();

            if (symbol_node == nullptr)
            {
                TIntermBinary* binary_node = node->getLeft()->getAsBinaryNode();
                if (binary_node)
                {
                    symbol_node = binary_node->getLeft()->getAsSymbolNode();
                }
            }

            if (symbol_node != nullptr)
            {
                long long symbol_id = symbol_node->getId();
                auto iter = declared_symbols_id.find(symbol_id);
                if (iter == declared_symbols_id.end()) { declared_symbols_id.insert(symbol_id); }
                else { is_declared = true; }
            }

            if (is_declared == false)
            {
                TString type_str = getTypeText(node->getType());

                // check twice, since some build-in symbol are makred as EvqTemporary instead of EvqVertexId to EvqFragStencil
                const TString& symbol_name = symbol_node->getName();
                bool is_build_in_variable = false;
                if ((symbol_name[0] == 'g') && (symbol_name[1] == 'l') && (symbol_name[2] == '_'))
                {
                    is_build_in_variable = true;
                }

                if ((type_str != "") && (!is_build_in_variable))
                {
                    code_buffer.append(type_str);
                    code_buffer.append(" ");
                }
            }
        }
        else if (visit == EvInVisit)
        {
            if (!enable_white_space_optimize) { code_buffer.append(" "); }
            code_buffer.append("=");
            if (!enable_white_space_optimize) { code_buffer.append(" "); }
        }
        else if (visit == EvPostVisit)
        {
            if (parser_context.is_in_loop_header == false) { code_buffer.append(";");}
            if (!enable_line_feed_optimize) { code_buffer.append("\n"); }
        }
        break;
    }
    case EOpIndexDirectStruct:
    {
        if (visit == EvPreVisit)
        {
            bool reference = node->getLeft()->getType().isReference();
            assert_t(reference == false);

            const TTypeList* members = reference ? node->getLeft()->getType().getReferentType()->getStruct() : node->getLeft()->getType().getStruct();
            int member_index = node->getRight()->getAsConstantUnion()->getConstArray()[0].getIConst();
            const TString& index_direct_struct_str = (*members)[member_index].type->getFieldName();

            code_buffer.append(node->getLeft()->getAsSymbolNode()->getName());
            code_buffer.append(".");
            code_buffer.append(index_direct_struct_str);

            // no need to output the struct name
            node->setLeft(nullptr);
            node->setRight(nullptr);
        }
        else if (visit == EvInVisit)
        {

        }
        else if (visit == EvPostVisit)
        {

        }
        break;
    }
    case EOpIndexIndirect: 
    NODE_VISIT_FUNC(, code_buffer.append("["); , code_buffer.append("]"); );
    case EOpIndexDirect: 
    NODE_VISIT_FUNC(if (node->getLeft()->getAsBinaryNode())
    {
        TIntermBinary* bin_node = node->getLeft()->getAsBinaryNode();
        TOperator node_operator = bin_node->getOp();
        switch (node_operator)
        {
        case EOpVectorTimesScalar:
        case EOpMatrixTimesVector:
        case EOpMatrixTimesScalar:
        {
            code_buffer.append("(");
            break;
        }
        default: {break; }
        }
    },
        
        if (node->getLeft()->getAsBinaryNode())
        {
            TIntermBinary* bin_node = node->getLeft()->getAsBinaryNode();
            TOperator node_operator = bin_node->getOp();
            switch (node_operator)
            {
            case EOpVectorTimesScalar:
            case EOpMatrixTimesVector:
            case EOpMatrixTimesScalar:
            {
                code_buffer.append(")");
                break;
            }
            default: {break; }
            }
        };
        if (node->getLeft()->getType().isArray() || node->getLeft()->getType().isMatrix())
        { 
            code_buffer.append("["); 
        }
        else
        {
            code_buffer.append(".");
            parser_context.is_vector_swizzle = true;
        },

         

        if (node->getLeft()->getType().isArray() || node->getLeft()->getType().isMatrix())
        { 
            code_buffer.append("]"); 
            
        }
        else
        {
            parser_context.is_vector_swizzle = false;
        } );
    
        //
    // binary operations
    //

    case EOpAdd:NODE_VISIT_FUNC(code_buffer.append("("); , ; code_buffer.append("+");, ; code_buffer.append(")"); );
    case EOpSub:NODE_VISIT_FUNC(code_buffer.append("("); , ; code_buffer.append("-");, ; code_buffer.append(")"); );
    case EOpMul:NODE_VISIT_FUNC(code_buffer.append("("); , ; code_buffer.append("*");, ; code_buffer.append(")"); );
    case EOpDiv:NODE_VISIT_FUNC(code_buffer.append("("); , ; code_buffer.append("/");, ; code_buffer.append(")"); );
    case EOpMod:NODE_VISIT_FUNC(code_buffer.append("("); , ; code_buffer.append("%");, ; code_buffer.append(")"); );
    case EOpRightShift:NODE_VISIT_FUNC(, code_buffer.append(">>"), );
    case EOpLeftShift:NODE_VISIT_FUNC(, code_buffer.append("<<"), );
    case EOpAnd:NODE_VISIT_FUNC(code_buffer.append("(");, code_buffer.append("&");, code_buffer.append(")"));
    case EOpEqual:NODE_VISIT_FUNC(code_buffer.append("("), code_buffer.append("=="), code_buffer.append(")"));
    case EOpNotEqual:NODE_VISIT_FUNC(code_buffer.append("("), code_buffer.append("!="), code_buffer.append(")"));
    

    // binary less than
    case EOpLessThan:NODE_VISIT_FUNC(code_buffer.append("("), code_buffer.append("<"); , code_buffer.append(")"));

    case EOpGreaterThan:NODE_VISIT_FUNC(code_buffer.append("("), code_buffer.append(">"), code_buffer.append(")"));
    case EOpLessThanEqual:NODE_VISIT_FUNC(code_buffer.append("("), code_buffer.append("<="), code_buffer.append(")"));
    case EOpGreaterThanEqual:NODE_VISIT_FUNC(code_buffer.append("("), code_buffer.append(">="), code_buffer.append(")"));
    

    case EOpVectorSwizzle:NODE_VISIT_FUNC(, code_buffer.append("."); parser_context.is_vector_swizzle = true;, parser_context.is_vector_swizzle = false);

    case EOpInclusiveOr: NODE_VISIT_FUNC(code_buffer.append("("), code_buffer.append("|"), code_buffer.append(")"));
    case EOpExclusiveOr: NODE_VISIT_FUNC(code_buffer.append("("), code_buffer.append("^"), code_buffer.append(")"));

    case EOpVectorTimesScalar:NODE_VISIT_FUNC(code_buffer.append("(");, code_buffer.append("*");, code_buffer.append(")"));
    case EOpVectorTimesMatrix: NODE_VISIT_FUNC(code_buffer.append("("); , code_buffer.append("*"); , code_buffer.append(")"));
    case EOpMatrixTimesVector:NODE_VISIT_FUNC(code_buffer.append("("), code_buffer.append("*");, code_buffer.append(")"));
    case EOpMatrixTimesScalar:NODE_VISIT_FUNC(, code_buffer.append("*"), );
    case EOpMatrixTimesMatrix: NODE_VISIT_FUNC(code_buffer.append("("), code_buffer.append("*"), code_buffer.append(")"));

    case EOpLogicalOr:  NODE_VISIT_FUNC(code_buffer.append("("), code_buffer.append("||"), code_buffer.append(")"));
    case EOpLogicalAnd: NODE_VISIT_FUNC(code_buffer.append("("), code_buffer.append("&&"), code_buffer.append(")"));

    default:
    {
        assert_t(false);
        break;
    }
    };

    return true;
}

void TAstToGLTraverser::emitTypeConvert(TVisit visit, TIntermUnary* node, const TString& unaryName, const TString& vecName, bool onlyConvertVec)
{
    if (visit == EvPreVisit) 
    {
        int vec_size = node->getVectorSize();
        if (vec_size > 1) 
        { 
            code_buffer.append(vecName);
            code_buffer.append(std::to_string(vec_size).c_str());
            code_buffer.append("(");
        }
        else if(onlyConvertVec == false)
        { 
            code_buffer.append(unaryName);
            code_buffer.append("(");
        }      
    }
    else if (visit == EvPostVisit) 
    {
        int vec_size = node->getVectorSize();
        if ((vec_size > 1) || (onlyConvertVec == false))
        {
            code_buffer.append(")");
        }
    }
}

bool TAstToGLTraverser::visitUnary(TVisit visit, TIntermUnary* node)
{

    TOperator node_operator = node->getOp();
    switch (node_operator)
    {
    case EOpLogicalNot:NODE_VISIT_FUNC(code_buffer.append("(!"); , , code_buffer.append(")"); );
    case EOpPostIncrement: NODE_VISIT_FUNC(, , code_buffer.append("++"); if (parser_context.is_in_loop_header == false) { code_buffer.append(";"); });
    case EOpPostDecrement: NODE_VISIT_FUNC(, , code_buffer.append("--"); if (parser_context.is_in_loop_header == false) { code_buffer.append(";"); });
    case EOpPreIncrement: NODE_VISIT_FUNC(code_buffer.append("++"), , ; );
    case EOpPreDecrement: NODE_VISIT_FUNC(code_buffer.append("--"), , ; );

    case EOpSin:            NODE_VISIT_FUNC(code_buffer.append("sin("); , , code_buffer.append(")"); );
    case EOpCos:            NODE_VISIT_FUNC(code_buffer.append("cos(");, , code_buffer.append(")"); );
    case EOpTan:            NODE_VISIT_FUNC(code_buffer.append("tan(");, , code_buffer.append(")"); );
    case EOpAsin:           NODE_VISIT_FUNC(code_buffer.append("asin(");, , code_buffer.append(")"); );
    case EOpAcos:           NODE_VISIT_FUNC(code_buffer.append("acos(");, , code_buffer.append(")"); );
    case EOpAtan:           NODE_VISIT_FUNC(code_buffer.append("atan(");, , code_buffer.append(")"); );

    case EOpExp: NODE_VISIT_FUNC(code_buffer.append("exp(");, , code_buffer.append(")"); );
    case EOpLog: NODE_VISIT_FUNC(code_buffer.append("log(");, , code_buffer.append(")"); );
    case EOpExp2: NODE_VISIT_FUNC(code_buffer.append("exp2(");, , code_buffer.append(")"); );
    case EOpLog2: NODE_VISIT_FUNC(code_buffer.append("log2(");, , code_buffer.append(")"); );
    case EOpSqrt: NODE_VISIT_FUNC(code_buffer.append("sqrt(");, , code_buffer.append(")"); );
    case EOpInverseSqrt:  NODE_VISIT_FUNC(code_buffer.append("inversesqrt("); , , code_buffer.append(")"); );
    
    case EOpAbs:  NODE_VISIT_FUNC(code_buffer.append("abs("); , , code_buffer.append(")"); );
    case EOpSign:  NODE_VISIT_FUNC(code_buffer.append("sign("); , , code_buffer.append(")"); );
    case EOpFloor:  NODE_VISIT_FUNC(code_buffer.append("floor("); , , code_buffer.append(")"); );
    case EOpTrunc:  NODE_VISIT_FUNC(code_buffer.append("trunc("); , , code_buffer.append(")"); );
    case EOpRound:  NODE_VISIT_FUNC(code_buffer.append("round("); , , code_buffer.append(")"); );
    case EOpRoundEven:  NODE_VISIT_FUNC(code_buffer.append("roundEven("); , , code_buffer.append(")"); );
    case EOpCeil:  NODE_VISIT_FUNC(code_buffer.append("ceil("); , , code_buffer.append(")"); );
    case EOpFract:  NODE_VISIT_FUNC(code_buffer.append("fract("); , , code_buffer.append(")"); );


    case EOpIsNan:  NODE_VISIT_FUNC(code_buffer.append("isnan(");,, code_buffer.append(")"); );
    case EOpIsInf:  NODE_VISIT_FUNC(code_buffer.append("isinf(");,, code_buffer.append(")"); );

    case EOpFloatBitsToInt:  NODE_VISIT_FUNC(code_buffer.append("floatBitsToInt(");,, code_buffer.append(")"); );
    case EOpFloatBitsToUint:  NODE_VISIT_FUNC(code_buffer.append("floatBitsToUint(");,, code_buffer.append(")"); );
    case EOpIntBitsToFloat:  NODE_VISIT_FUNC(code_buffer.append("intBitsToFloat(");,, code_buffer.append(")"); );
    case EOpUintBitsToFloat:  NODE_VISIT_FUNC(code_buffer.append("uintBitsToFloat(");,, code_buffer.append(")"); );
    case EOpDoubleBitsToInt64:  NODE_VISIT_FUNC(code_buffer.append("doubleBitsToInt64(");,, code_buffer.append(")"); );
    case EOpDoubleBitsToUint64:  NODE_VISIT_FUNC(code_buffer.append("doubleBitsToUint64(");,, code_buffer.append(")"); );
    case EOpInt64BitsToDouble:  NODE_VISIT_FUNC(code_buffer.append("int64BitsToDouble(");,, code_buffer.append(")"); );
    case EOpUint64BitsToDouble:  NODE_VISIT_FUNC(code_buffer.append("uint64BitsToDouble(");,, code_buffer.append(")"); );
    case EOpFloat16BitsToInt16:  NODE_VISIT_FUNC(code_buffer.append("float16BitsToInt16(");,, code_buffer.append(")"); );
    case EOpFloat16BitsToUint16:  NODE_VISIT_FUNC(code_buffer.append("float16BitsToUint16(");,, code_buffer.append(")"); );
    case EOpInt16BitsToFloat16:  NODE_VISIT_FUNC(code_buffer.append("int16BitsToFloat16(");,, code_buffer.append(")"); );
    case EOpUint16BitsToFloat16:  NODE_VISIT_FUNC(code_buffer.append("uint16BitsToFloat16(");,, code_buffer.append(")"); );

    //Geometric Functions
    case EOpLength: NODE_VISIT_FUNC(code_buffer.append("length(");, , code_buffer.append(")"); );
    case EOpNormalize: NODE_VISIT_FUNC(code_buffer.append("normalize("); , , code_buffer.append(")"); );
    case EOpDPdx:       NODE_VISIT_FUNC(code_buffer.append("dFdx("); , , code_buffer.append(")"); );
    case EOpDPdy:       NODE_VISIT_FUNC(code_buffer.append("dFdx("); , , code_buffer.append(")"); );
    case EOpReflect: NODE_VISIT_FUNC(code_buffer.append("reflect(");, , code_buffer.append(")"); );

    case EOpMatrixInverse:  NODE_VISIT_FUNC(code_buffer.append("inverse(");, , code_buffer.append(")"); );
    case EOpTranspose:      NODE_VISIT_FUNC(code_buffer.append("transpose(");, , code_buffer.append(")"); );
    
    case EOpAny:            NODE_VISIT_FUNC(code_buffer.append("any(");, , code_buffer.append(")"); );
    case EOpAll:            NODE_VISIT_FUNC(code_buffer.append("all(");, , code_buffer.append(")"); );

    // todo: remove (
    case EOpNegative:NODE_VISIT_FUNC(code_buffer.append("-");, , );

    // GLSL spec 4.5
    // 4.1.10. Implicit Conversions

     // * -> bool
    case EOpConvInt8ToBool:    NODE_VISIT_FUNC(code_buffer.append("bool");; code_buffer.append("("); , , code_buffer.append(")"););
    case EOpConvUint8ToBool:   NODE_VISIT_FUNC(code_buffer.append("bool");; code_buffer.append("("); , , code_buffer.append(")"););
    case EOpConvInt16ToBool:   NODE_VISIT_FUNC(code_buffer.append("bool");; code_buffer.append("("); , , code_buffer.append(")"););
    case EOpConvUint16ToBool:  NODE_VISIT_FUNC(code_buffer.append("bool");; code_buffer.append("("); , , code_buffer.append(")"););
    case EOpConvIntToBool:     NODE_VISIT_FUNC(code_buffer.append("bool");; code_buffer.append("("); , , code_buffer.append(")"););
    case EOpConvUintToBool:    NODE_VISIT_FUNC(code_buffer.append("bool");; code_buffer.append("("); , , code_buffer.append(")"););
    case EOpConvInt64ToBool:   NODE_VISIT_FUNC(code_buffer.append("bool");; code_buffer.append("("); , , code_buffer.append(")"););
    case EOpConvUint64ToBool:  NODE_VISIT_FUNC(code_buffer.append("bool");; code_buffer.append("("); , , code_buffer.append(")"););
    case EOpConvFloat16ToBool: NODE_VISIT_FUNC(code_buffer.append("bool");; code_buffer.append("("); , , code_buffer.append(")"););
    case EOpConvFloatToBool:   NODE_VISIT_FUNC(code_buffer.append("bool");; code_buffer.append("("); , , code_buffer.append(")"););
    case EOpConvDoubleToBool:  NODE_VISIT_FUNC(code_buffer.append("bool");; code_buffer.append("("); , , code_buffer.append(")"););

        // bool -> *
    case EOpConvBoolToUint:    emitTypeConvert(visit, node, "uint", "uvec"); break;
    case EOpConvBoolToInt:     emitTypeConvert(visit, node, "int", "ivec"); break;
    case EOpConvBoolToFloat16: emitTypeConvert(visit, node, "half", "half"); break;
    case EOpConvBoolToFloat:   emitTypeConvert(visit, node, "float", "vec"); break;

    //todo: fix me
    // If the operand types do not match, then
    // there must be a conversion from ¡°Implicit Conversions¡± applied to one operand that can make
    // them match, in which case this conversion is done
    
    // wrong operand types: no operation '==' exists that takes a left-hand operand of type ' temp highp uint' and a right operand of type 'layout( column_major std140 offset=1424) uniform highp int' (or there is no acceptable conversion)
    
    // int32_t -> (u)int*
    case EOpConvIntToUint:    NODE_VISIT_FUNC(
        TIntermBinary* binary_node = getParentNode()->getAsBinaryNode();
        if (binary_node && (binary_node->getOp() == EOpEqual))
        {
            code_buffer.append("uint(");
        },,
            TIntermBinary* binary_node = getParentNode()->getAsBinaryNode();
        if (binary_node && (binary_node->getOp() == EOpEqual))
        {
            code_buffer.append(")");
        };
    );

     // int32_t -> float*
    case EOpConvIntToFloat:    emitTypeConvert(visit, node, "float", "vec"/*, true*, todo: fix me, implict convert*/); break;

    // uint32_t -> (u)int*
    case EOpConvUintToInt:     emitTypeConvert(visit, node, "int", "ivec"); break;

    case EOpConvUintToFloat:   emitTypeConvert(visit, node, "float", "vec"/*, true*, todo: fix me, implict convert*/); break;
    
        // float32_t -> int*
    case EOpConvFloatToInt:   emitTypeConvert(visit, node, "int", "ivec"); break;

    // float32_t -> uint*
    case EOpConvFloatToUint8:  NODE_VISIT_FUNC(code_buffer.append("uint8(");,, code_buffer.append(")"););
    case EOpConvFloatToUint16: NODE_VISIT_FUNC(code_buffer.append("uint16(");,, code_buffer.append(")"););;
    case EOpConvFloatToUint:   emitTypeConvert(visit, node, "uint", "uvec"); break;
    case EOpConvFloatToUint64: NODE_VISIT_FUNC(code_buffer.append("uint64(");,, code_buffer.append(")"););

    default:
        assert_t(false);
        break;
    }
    return true;
}


bool TAstToGLTraverser::visitAggregate(TVisit visit, TIntermAggregate* node)
{
    TOperator node_operator = node->getOp();
    switch (node_operator)
    {
    case EOpComma:NODE_VISIT_FUNC(, code_buffer.append(","), );
    case EOpFunction:
    {
        if (visit == EvPreVisit)
        {
            assert_t(node->getBasicType() == EbtVoid);
            code_buffer.append(node->getType().getBasicString());
            code_buffer.insert(code_buffer.end(), ' ');
            code_buffer.append(node->getName());
        }
        else if (visit == EvInVisit)
        {
            
        }
        else if(visit == EvPostVisit)
        {
            code_buffer.insert(code_buffer.end(), '}');
            if (!enable_line_feed_optimize) { code_buffer.append("\n"); }
        }
        
        break;
    }
    case EOpParameters:NODE_VISIT_FUNC(assert_t(node->getSequence().size() == 0); , assert_t(false);, code_buffer.append(") { "); if (!enable_line_feed_optimize) { code_buffer.append("\n"); });

    //8.3. Common Functions
    case EOpMin: NODE_VISIT_FUNC(code_buffer.append("min(");, code_buffer.append(","); ,  code_buffer.append(")"); );
    case EOpMax: NODE_VISIT_FUNC(code_buffer.append("max("); , code_buffer.append(","); , code_buffer.append(")"); );
    case EOpClamp: NODE_VISIT_FUNC(code_buffer.append("clamp("); , code_buffer.append(","), code_buffer.append(")"); );
    case EOpMix: NODE_VISIT_FUNC(code_buffer.append("mix("); , code_buffer.append(","), code_buffer.append(")"); );
    case EOpStep: NODE_VISIT_FUNC(code_buffer.append("step("); , code_buffer.append(","), code_buffer.append(")"); );
    case EOpSmoothStep:NODE_VISIT_FUNC(code_buffer.append("smoothstep(");, code_buffer.append(","), code_buffer.append(")"); );

    case EOpDistance: NODE_VISIT_FUNC(code_buffer.append("distance("); , code_buffer.append(","), code_buffer.append(")"); );
    case EOpDot: NODE_VISIT_FUNC(code_buffer.append("dot(");  , code_buffer.append(",");, code_buffer.append(")"); );
    case EOpCross: NODE_VISIT_FUNC(code_buffer.append("cross("); ,  code_buffer.append(","); ,; code_buffer.append(")"); );
    case EOpRefract:       NODE_VISIT_FUNC(code_buffer.append("refract(");, code_buffer.append(","); , code_buffer.append(")"); );

    case EOpConstructVec2:NODE_VISIT_FUNC(code_buffer.append("vec2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructVec3:NODE_VISIT_FUNC(code_buffer.append("vec3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructVec4:NODE_VISIT_FUNC(code_buffer.append("vec4("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructDVec2:NODE_VISIT_FUNC(code_buffer.append("dvec2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructDVec3:NODE_VISIT_FUNC(code_buffer.append("dvec3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructDVec4:NODE_VISIT_FUNC(code_buffer.append("dvec4("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructBool:NODE_VISIT_FUNC(code_buffer.append("bool("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructBVec2:NODE_VISIT_FUNC(code_buffer.append("bvec2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructBVec3:NODE_VISIT_FUNC(code_buffer.append("bvec3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructBVec4:NODE_VISIT_FUNC(code_buffer.append("bvec4("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructInt8:NODE_VISIT_FUNC(code_buffer.append("int8_t("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructI8Vec2:NODE_VISIT_FUNC(code_buffer.append("i8vec2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructI8Vec3:NODE_VISIT_FUNC(code_buffer.append("i8vec3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructI8Vec4:NODE_VISIT_FUNC(code_buffer.append("i8vec4("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructInt:NODE_VISIT_FUNC(code_buffer.append("int("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructIVec2:NODE_VISIT_FUNC(code_buffer.append("ivec2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructIVec3:NODE_VISIT_FUNC(code_buffer.append("ivec3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructIVec4:NODE_VISIT_FUNC(code_buffer.append("ivec4("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructUint8:NODE_VISIT_FUNC(code_buffer.append("uint8_t("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructU8Vec2:NODE_VISIT_FUNC(code_buffer.append("u8vec2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructU8Vec3:NODE_VISIT_FUNC(code_buffer.append("u8vec3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructU8Vec4:NODE_VISIT_FUNC(code_buffer.append("u8vec4("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructUint:NODE_VISIT_FUNC(code_buffer.append("uint("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructUVec2:NODE_VISIT_FUNC(code_buffer.append("uvec2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructUVec3:NODE_VISIT_FUNC(code_buffer.append("uvec3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructUVec4:NODE_VISIT_FUNC(code_buffer.append("uvec4("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructInt64:NODE_VISIT_FUNC(code_buffer.append("int64("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructI64Vec2:NODE_VISIT_FUNC(code_buffer.append("i64vec2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructI64Vec3:NODE_VISIT_FUNC(code_buffer.append("i64vec3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructI64Vec4:NODE_VISIT_FUNC(code_buffer.append("i64vec4("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructUint64:NODE_VISIT_FUNC(code_buffer.append("uint64("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructU64Vec2:NODE_VISIT_FUNC(code_buffer.append("u64vec2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructU64Vec3:NODE_VISIT_FUNC(code_buffer.append("u64vec3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructU64Vec4:NODE_VISIT_FUNC(code_buffer.append("u64vec4("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructInt16:NODE_VISIT_FUNC(code_buffer.append("int16_t("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructI16Vec2:NODE_VISIT_FUNC(code_buffer.append("i16vec2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructI16Vec3:NODE_VISIT_FUNC(code_buffer.append("i16vec3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructI16Vec4:NODE_VISIT_FUNC(code_buffer.append("i16vec4("), code_buffer.append(","), code_buffer.append(")"));
    
    case EOpConstructMat2x2:NODE_VISIT_FUNC(code_buffer.append("mat2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructMat2x3:NODE_VISIT_FUNC(code_buffer.append("mat2x3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructMat2x4:NODE_VISIT_FUNC(code_buffer.append("mat2x4("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructMat3x2:NODE_VISIT_FUNC(code_buffer.append("mat3x2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructMat3x3:NODE_VISIT_FUNC(code_buffer.append("mat3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructMat3x4:NODE_VISIT_FUNC(code_buffer.append("mat3x4("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructMat4x2:NODE_VISIT_FUNC(code_buffer.append("mat4x2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructMat4x3:NODE_VISIT_FUNC(code_buffer.append("mat4x3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructMat4x4:NODE_VISIT_FUNC(code_buffer.append("mat4("), code_buffer.append(","), code_buffer.append(")"));

    case EOpConstructDMat2x2:NODE_VISIT_FUNC(code_buffer.append("dmat2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructDMat2x3:NODE_VISIT_FUNC(code_buffer.append("dmat2x3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructDMat2x4:NODE_VISIT_FUNC(code_buffer.append("dmat2x4("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructDMat3x2:NODE_VISIT_FUNC(code_buffer.append("dmat3x2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructDMat3x3:NODE_VISIT_FUNC(code_buffer.append("dmat3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructDMat3x4:NODE_VISIT_FUNC(code_buffer.append("dmat3x4("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructDMat4x2:NODE_VISIT_FUNC(code_buffer.append("dmat4x2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructDMat4x3:NODE_VISIT_FUNC(code_buffer.append("dmat4x3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructDMat4x4:NODE_VISIT_FUNC(code_buffer.append("dmat4("), code_buffer.append(","), code_buffer.append(")"));

    case EOpConstructIMat2x2:NODE_VISIT_FUNC(code_buffer.append("imat2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructIMat2x3:NODE_VISIT_FUNC(code_buffer.append("imat2x3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructIMat2x4:NODE_VISIT_FUNC(code_buffer.append("imat2x4("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructIMat3x2:NODE_VISIT_FUNC(code_buffer.append("imat3x2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructIMat3x3:NODE_VISIT_FUNC(code_buffer.append("imat3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructIMat3x4:NODE_VISIT_FUNC(code_buffer.append("imat3x4("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructIMat4x2:NODE_VISIT_FUNC(code_buffer.append("imat4x2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructIMat4x3:NODE_VISIT_FUNC(code_buffer.append("imat4x3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructIMat4x4:NODE_VISIT_FUNC(code_buffer.append("imat4("), code_buffer.append(","), code_buffer.append(")"));

    case EOpConstructUMat2x2:NODE_VISIT_FUNC(code_buffer.append("umat2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructUMat2x3:NODE_VISIT_FUNC(code_buffer.append("umat2x3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructUMat2x4:NODE_VISIT_FUNC(code_buffer.append("umat2x4("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructUMat3x2:NODE_VISIT_FUNC(code_buffer.append("umat3x2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructUMat3x3:NODE_VISIT_FUNC(code_buffer.append("umat3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructUMat3x4:NODE_VISIT_FUNC(code_buffer.append("umat3x4("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructUMat4x2:NODE_VISIT_FUNC(code_buffer.append("umat4x2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructUMat4x3:NODE_VISIT_FUNC(code_buffer.append("umat4x3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructUMat4x4:NODE_VISIT_FUNC(code_buffer.append("umat4("), code_buffer.append(","), code_buffer.append(")"));

    case EOpConstructBMat2x2:NODE_VISIT_FUNC(code_buffer.append("bmat2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructBMat2x3:NODE_VISIT_FUNC(code_buffer.append("bmat2x3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructBMat2x4:NODE_VISIT_FUNC(code_buffer.append("bmat2x4("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructBMat3x2:NODE_VISIT_FUNC(code_buffer.append("bmat3x2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructBMat3x3:NODE_VISIT_FUNC(code_buffer.append("bmat3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructBMat3x4:NODE_VISIT_FUNC(code_buffer.append("bmat3x4("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructBMat4x2:NODE_VISIT_FUNC(code_buffer.append("bmat4x2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructBMat4x3:NODE_VISIT_FUNC(code_buffer.append("bmat4x3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructBMat4x4:NODE_VISIT_FUNC(code_buffer.append("bmat4("), code_buffer.append(","), code_buffer.append(")"))

    case EOpConstructFloat16:NODE_VISIT_FUNC(code_buffer.append("float16_t("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructF16Vec2:NODE_VISIT_FUNC(code_buffer.append("f16vec2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructF16Vec3:NODE_VISIT_FUNC(code_buffer.append("f16vec3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructF16Vec4:NODE_VISIT_FUNC(code_buffer.append("f16vec4("), code_buffer.append(","), code_buffer.append(")"));

    case EOpConstructF16Mat2x2:NODE_VISIT_FUNC(code_buffer.append("f16mat2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructF16Mat2x3:NODE_VISIT_FUNC(code_buffer.append("f16mat2x3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructF16Mat2x4:NODE_VISIT_FUNC(code_buffer.append("f16mat2x4("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructF16Mat3x2:NODE_VISIT_FUNC(code_buffer.append("f16mat3x2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructF16Mat3x3:NODE_VISIT_FUNC(code_buffer.append("f16mat3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructF16Mat3x4:NODE_VISIT_FUNC(code_buffer.append("f16mat3x4("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructF16Mat4x2:NODE_VISIT_FUNC(code_buffer.append("f16mat4x2("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructF16Mat4x3:NODE_VISIT_FUNC(code_buffer.append("f16mat4x3("), code_buffer.append(","), code_buffer.append(")"));
    case EOpConstructF16Mat4x4:NODE_VISIT_FUNC(code_buffer.append("f16mat4("), code_buffer.append(","), code_buffer.append(")"))

    // 5.9 Expressions
    // aggrate than 
    // component-wise relational  comparisons on vectors
    case EOpLessThan:         NODE_VISIT_FUNC(code_buffer.append("lessThan("), code_buffer.append(","), code_buffer.append(")"));
    case EOpGreaterThan:      NODE_VISIT_FUNC(code_buffer.append("greaterThan("), code_buffer.append(","), code_buffer.append(")"));
    case EOpLessThanEqual:    NODE_VISIT_FUNC(code_buffer.append("lessThanEqual("), code_buffer.append(","), code_buffer.append(")"));
    case EOpGreaterThanEqual: NODE_VISIT_FUNC(code_buffer.append("greaterThanEqual("), code_buffer.append(","), code_buffer.append(")"));
    
    //component-wise equality
    case EOpVectorEqual:      NODE_VISIT_FUNC(code_buffer.append("equal("), code_buffer.append(","), code_buffer.append(")"));
    case EOpVectorNotEqual:   NODE_VISIT_FUNC(code_buffer.append("notEqual("), code_buffer.append(","), code_buffer.append(")"));

    case EOpMod:           NODE_VISIT_FUNC(code_buffer.append("mod("), code_buffer.append(","), code_buffer.append(")"));
    case EOpModf:           NODE_VISIT_FUNC(code_buffer.append("modf("), code_buffer.append(","), code_buffer.append(")"));
    case EOpPow:           NODE_VISIT_FUNC(code_buffer.append("pow(");, code_buffer.append(",");, code_buffer.append(")"); );

    case EOpAtan:          NODE_VISIT_FUNC(code_buffer.append("atan("); , code_buffer.append(","); , code_buffer.append(")"); );

    case EOpLinkerObjects:NODE_VISIT_FUNC(, code_buffer.append(";"); if (!enable_line_feed_optimize) { code_buffer.append("\n"); }, code_buffer.append(";"); if (!enable_line_feed_optimize) { code_buffer.append("\n"); });
    case EOpTextureQuerySize:NODE_VISIT_FUNC(code_buffer.append("textureSize("), code_buffer.append(","), code_buffer.append(")"));
    case EOpTextureQueryLod:NODE_VISIT_FUNC(code_buffer.append("textureQueryLod("), code_buffer.append(","), code_buffer.append(")"));
    case EOpTextureQueryLevels:NODE_VISIT_FUNC(code_buffer.append("textureQueryLevels("), code_buffer.append(","), code_buffer.append(")"));
    case EOpTextureQuerySamples:NODE_VISIT_FUNC(code_buffer.append("textureSamples("), code_buffer.append(","), code_buffer.append(")"));
    case EOpTexture:NODE_VISIT_FUNC(code_buffer.append("texture("), code_buffer.append(","), code_buffer.append(")"));
    case EOpTextureProj:NODE_VISIT_FUNC(code_buffer.append("textureProj("), code_buffer.append(","), code_buffer.append(")"));
    case EOpTextureLod:NODE_VISIT_FUNC(code_buffer.append("textureLod("), code_buffer.append(","), code_buffer.append(")"));
    case EOpTextureOffset:NODE_VISIT_FUNC(code_buffer.append("textureOffset("), code_buffer.append(","), code_buffer.append(")"));
    case EOpTextureFetch:NODE_VISIT_FUNC(code_buffer.append("texelFetch("), code_buffer.append(","), code_buffer.append(")"));
    case EOpTextureFetchOffset:NODE_VISIT_FUNC(code_buffer.append("textureFetchOffset("), code_buffer.append(","), code_buffer.append(")"));
    case EOpTextureProjOffset:NODE_VISIT_FUNC(code_buffer.append("textureProjOffset("), code_buffer.append(","), code_buffer.append(")"));
    case EOpTextureLodOffset:NODE_VISIT_FUNC(code_buffer.append("textureLodOffset("), code_buffer.append(","), code_buffer.append(")"));
    case EOpTextureProjLod:NODE_VISIT_FUNC(code_buffer.append("textureProjLod("), code_buffer.append(","), code_buffer.append(")"));
    case EOpTextureGrad:NODE_VISIT_FUNC(code_buffer.append("textureGrad("), code_buffer.append(","), code_buffer.append(")"));
    case EOpTextureGradOffset:NODE_VISIT_FUNC(code_buffer.append("textureGradOffset("), code_buffer.append(","), code_buffer.append(")"));
    case EOpTextureProjGrad:NODE_VISIT_FUNC(code_buffer.append("textureProjGrad("), code_buffer.append(","), code_buffer.append(")"));
    case EOpTextureProjGradOffset:NODE_VISIT_FUNC(code_buffer.append("textureProjGradOffset("), code_buffer.append(","), code_buffer.append(")"));
    case EOpTextureGather:NODE_VISIT_FUNC(code_buffer.append("textureGather("), code_buffer.append(","), code_buffer.append(")"));
    case EOpTextureGatherOffset:NODE_VISIT_FUNC(code_buffer.append("textureGatherOffset("), code_buffer.append(","), code_buffer.append(")"));
    case EOpTextureClamp:NODE_VISIT_FUNC(code_buffer.append("textureClamp("), code_buffer.append(","), code_buffer.append(")"));
    case EOpTextureOffsetClamp:NODE_VISIT_FUNC(code_buffer.append("textureOffsetClamp("), code_buffer.append(","), code_buffer.append(")"));
    case EOpTextureGradClamp:NODE_VISIT_FUNC(code_buffer.append("textureGradClamp("), code_buffer.append(","), code_buffer.append(")"));
    case EOpTextureGradOffsetClamp:NODE_VISIT_FUNC(code_buffer.append("textureGradOffsetClamp("), code_buffer.append(","), code_buffer.append(")"));
    case EOpTextureGatherLod:NODE_VISIT_FUNC(code_buffer.append("texture("), code_buffer.append(","), code_buffer.append(")"));
    case EOpTextureGatherLodOffset:NODE_VISIT_FUNC(code_buffer.append("textureGatherLodOffset("), code_buffer.append(","), code_buffer.append(")"));
    case EOpTextureGatherLodOffsets:NODE_VISIT_FUNC(code_buffer.append("textureGatherLodOffsets("), code_buffer.append(","), code_buffer.append(")"));
    }
    return true;
}

bool TAstToGLTraverser::visitSelection(TVisit, TIntermSelection* node)
{
    subscope_tranverser.resetSubScopeMaxLine();
    subscope_tranverser.visitSelection(EvPreVisit, node);
    declareSubScopeSymbol();

    if (node->getShortCircuit() == false)
    {
        assert_t(false);
    }

    if (node->getFlatten())
    {
        assert_t(false);
    }

    if (node->getDontFlatten())
    {
        assert_t(false);
    }

    bool is_ternnary = false;
   
    TIntermNode* true_block = node->getTrueBlock();
    TIntermNode* false_block = node->getFalseBlock();

    if ((true_block != nullptr) && (false_block != nullptr))
    {
        int true_block_line = true_block->getLoc().line;
        int false_block_line = false_block->getLoc().line;
        if (abs(true_block_line - false_block_line) <= 1)
        {
            is_ternnary = true;
        }
    }

    if (is_ternnary)
    {
        incrementDepth(node);
        code_buffer.append("((");
        node->getCondition()->traverse(this);
        code_buffer.append(")?(");
        true_block->traverse(this);
        code_buffer.append("):(");
        false_block->traverse(this);
        code_buffer.append("))");
    }
    else
    {
        incrementDepth(node);
        code_buffer.append("if(");

        node->getCondition()->traverse(this);
        code_buffer.append("){");
        if (!enable_line_feed_optimize) { code_buffer.append("\n"); }

        if (true_block) { true_block->traverse(this); }
        code_buffer.append("}");
        if (!enable_line_feed_optimize) { code_buffer.append("\n"); }

        if (false_block)
        {
            code_buffer.append("else{");
            false_block->traverse(this);
            code_buffer.append("}");
            if (!enable_line_feed_optimize) { code_buffer.append("\n"); }
        }

        decrementDepth();
    }

    return false;  /* visit the selection node on high level */
}

char unionConvertToChar(int index)
{
    static const char const_indices[4] = { 'x','y','z','w' };
    return const_indices[index];
}

TString OutputDouble(double value)
{
    if (std::isinf(value))
    {
        assert_t(false);
    }
    else if (std::isnan(value))
    {
        assert_t(false);
    }
    else
    {
        const int maxSize = 340;
        char buf[maxSize];
        const char* format = "%.23f";
        if (fabs(value) > 0.0 && (fabs(value) < 1e-5 || fabs(value) > 1e12))
        {
            format = "%-.31e";
        }
            
        int len = snprintf(buf, maxSize, format, value);
        assert(len < maxSize);

        // remove a leading zero in the 100s slot in exponent; it is not portable
        // pattern:   XX...XXXe+0XX or XX...XXXe-0XX
        if (len > 5) 
        {
            if (buf[len - 5] == 'e' && (buf[len - 4] == '+' || buf[len - 4] == '-') && buf[len - 3] == '0') 
            {
                assert_t(false);
                buf[len - 3] = buf[len - 2];
                buf[len - 2] = buf[len - 1];
                buf[len - 1] = '\0';
            }
        }

        
        for (int idx = len - 1; idx >= 0; idx--)
        {
            char next_char = buf[idx];
            bool is_no_zero = next_char > 48 && next_char <= 57;
            bool is_dot = (next_char == char('.'));
            if (is_dot || is_no_zero)
            {
                buf[idx + 1] = char('\0');
                break;
            }
        }


        return TString(buf);
    }
    return std::to_string(value).c_str();
};

void TAstToGLTraverser::outputConstantUnion(const TIntermConstantUnion* node, const TConstUnionArray& constUnion)
{
    int size = node->getType().computeNumComponents();

    bool is_construct_vector = false;
    bool is_construct_matrix = false;

    if (getParentNode()->getAsAggregate())
    {
        TIntermAggregate* parent_node = getParentNode()->getAsAggregate();
        TOperator node_operator = parent_node->getOp();
        switch (node_operator)
        {
        case EOpMin:
        case EOpMax:
        case EOpClamp:
        case EOpMix:
        case EOpStep:
        case EOpDistance:
        case EOpDot:
        case EOpCross:

        case EOpLessThan:
        case EOpGreaterThan:
        case EOpLessThanEqual:
        case EOpGreaterThanEqual:
        case EOpVectorEqual:
        case EOpVectorNotEqual:

        case EOpMod:
        case EOpModf:
        case EOpPow:

        case EOpConstructMat2x2:
        case EOpConstructMat2x3:
        case EOpConstructMat2x4:
        case EOpConstructMat3x2:
        case EOpConstructMat3x3:
        case EOpConstructMat3x4:
        case EOpConstructMat4x2:
        case EOpConstructMat4x3:
        case EOpConstructMat4x4:

        case  EOpTexture:
        case  EOpTextureProj:
        case  EOpTextureLod:
        case  EOpTextureOffset:
        case  EOpTextureFetch:
        case  EOpTextureFetchOffset:
        case  EOpTextureProjOffset:
        case  EOpTextureLodOffset:
        case  EOpTextureProjLod:
        case  EOpTextureProjLodOffset:
        case  EOpTextureGrad:
        case  EOpTextureGradOffset:
        case  EOpTextureProjGrad:
        case  EOpTextureProjGradOffset:
        case  EOpTextureGather:
        case  EOpTextureGatherOffset:
        case  EOpTextureGatherOffsets:
        case  EOpTextureClamp:
        case  EOpTextureOffsetClamp:
        case  EOpTextureGradClamp:
        case  EOpTextureGradOffsetClamp:
        case  EOpTextureGatherLod:
        case  EOpTextureGatherLodOffset:
        case  EOpTextureGatherLodOffsets:

        case EOpAny:
        case EOpAll:
        {
            is_construct_vector = true;
            break;
        }
        default:
        {
            break;
        }
        }
    }

    if (getParentNode()->getAsBinaryNode())
    {
        TIntermBinary* parent_node = getParentNode()->getAsBinaryNode();
        TOperator node_operator = parent_node->getOp();
        switch (node_operator)
        {
        case EOpAssign:

        case  EOpAdd:
        case  EOpSub:
        case  EOpMul:
        case  EOpDiv:
        case  EOpMod:
        case  EOpRightShift:
        case  EOpLeftShift:
        case  EOpAnd:
        case  EOpInclusiveOr:
        case  EOpExclusiveOr:
        case  EOpEqual:
        case  EOpNotEqual:
        case  EOpVectorEqual:
        case  EOpVectorNotEqual:
        case  EOpLessThan:
        case  EOpGreaterThan:
        case  EOpLessThanEqual:
        case  EOpGreaterThanEqual:
        case  EOpComma:

        case  EOpVectorTimesScalar:
        case  EOpVectorTimesMatrix:

        case  EOpLogicalOr:
        case  EOpLogicalXor:
        case  EOpLogicalAnd:
        {
            is_construct_vector = true;
            break;
        }

        case  EOpMatrixTimesVector:
        case  EOpMatrixTimesScalar:
        {
            is_construct_vector = true;
            if (node->getMatrixCols() > 1 && node->getMatrixRows() > 1)
            {
                is_construct_matrix = true;
            }
            break;
        }

        default:
        {
            break;
        }
        }
    }

    if (is_construct_matrix)
    {
        int mat_row = node->getMatrixRows();
        int mat_col = node->getMatrixCols();

        int array_idx = 0;
        for (int idx_row = 0; idx_row < mat_row; idx_row++)
        {
            switch (node->getBasicType())
            {
            case EbtDouble:
                code_buffer.append("d");
                break;
            case EbtInt:
                code_buffer.append("i");
                break;
            case EbtUint:
                code_buffer.append("u");
                break;
            case EbtBool:
                code_buffer.append("b");
                break;
            case EbtFloat:
            default:
                break;
            };

            code_buffer.append("vec");
            code_buffer.append(std::to_string(mat_col).c_str());
            code_buffer.append("(");
            for (int idx_col = 0; idx_col < mat_col; idx_col++)
            {
                TBasicType const_type = constUnion[array_idx].getType();
                switch (const_type)
                {
                case EbtDouble:
                {
                    code_buffer.append(OutputDouble(constUnion[array_idx].getDConst()));
                    break;
                }
                default:
                {
                    assert_t(false);
                    break;
                }
                }

                if (idx_col != (mat_col - 1))
                {
                    code_buffer.append(",");
                }
            }
            code_buffer.append(")");
            if (idx_row != (mat_row - 1))
            {
                code_buffer.append(",");
            }
            array_idx++;
        }
        return;
    }

    if (is_construct_vector)
    {
        constUnionBegin(node, node->getBasicType());
    }

    bool is_all_components_same = true;
    for (int i = 1; i < size; i++)
    {
        TBasicType const_type = constUnion[i].getType();
        switch (const_type)
        {
        case EbtInt: {if (constUnion[i].getIConst() != constUnion[i - 1].getIConst()) { is_all_components_same = false; } break; }
        case EbtDouble: {if (constUnion[i].getDConst() != constUnion[i - 1].getDConst()) { is_all_components_same = false; } break; }
        case EbtUint: {if (constUnion[i].getUConst() != constUnion[i - 1].getUConst()) { is_all_components_same = false; } break; }
        default:
        {
            assert_t(false);
            break;
        }
        };
    };

    if (is_all_components_same)
    {
        size = 1;
    }

    for (int i = 0; i < size; i++)
    {
        TBasicType const_type = constUnion[i].getType();
        switch (const_type)
        {
        case EbtInt:
        {
            if (parser_context.is_vector_swizzle)
            {
                code_buffer.insert(code_buffer.end(), unionConvertToChar(constUnion[i].getIConst()));
            }
            else
            {
                code_buffer.append(std::to_string(constUnion[i].getIConst()).c_str());
            }
            break;
        }
        case EbtDouble:
        {
            if (constUnion[i].getDConst() < 0)
            {
                //todo: fix me
                code_buffer.append("(");
            }
            code_buffer.append(OutputDouble(constUnion[i].getDConst()));
            if (constUnion[i].getDConst() < 0)
            {
                code_buffer.append(")");
            }
            break;
        }
        case EbtUint:
        {
            code_buffer.append(std::to_string(constUnion[i].getUConst()).c_str());
            code_buffer.append("u");
            break;
        }
        case EbtBool:
        {
            if (constUnion[i].getBConst()) { code_buffer.append("true"); }
            else { code_buffer.append("false"); }
            break;
        }

        default:
        {
            assert_t(false);
            break;
        }
        }

        if (parser_context.is_subvector_scalar && (i != (size - 1)))
        {
            code_buffer.append(",");
        }
    }

    if (is_construct_vector)
    {
        constUnionEnd(node);
    }
}

void TAstToGLTraverser::constUnionBegin(const TIntermConstantUnion* const_untion, TBasicType basic_type)
{
    if (const_untion)
    {
        int array_size = const_untion->getConstArray().size();
        if (array_size > 1)
        {
            switch (basic_type)
            {
            case EbtDouble:
                code_buffer.append("d");
                break;
            case EbtInt:
                code_buffer.append("i");
                break;
            case EbtUint:
                code_buffer.append("u");
                break;
            case EbtBool:
                code_buffer.append("b");
                break;
            case EbtFloat:
            default:
                break;
            };
            code_buffer.append("vec");
            code_buffer.append(std::to_string(array_size).c_str());
            code_buffer.append("(");
            parser_context.is_subvector_scalar = true;
        }
    }
}

void TAstToGLTraverser::constUnionEnd(const TIntermConstantUnion* const_untion)
{
    if (const_untion)
    {
        int array_size = const_untion->getConstArray().size();
        if (array_size > 1)
        {
            parser_context.is_subvector_scalar = false;
            code_buffer.append(")");
        }
    }
}

void TAstToGLTraverser::visitConstantUnion(TIntermConstantUnion* node)
{
    outputConstantUnion(node, node->getConstArray());
}

void TAstToGLTraverser::visitSymbol(TIntermSymbol* node)
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
        TString type_str = getTypeText(node->getType());

        const TString& symbol_name = node->getName();
        bool is_build_in_variable = false;
        if ((symbol_name[0] == 'g') && (symbol_name[1] == 'l') && (symbol_name[2] == '_'))
        {
            is_build_in_variable = true;
        }

        if ((type_str != "") && (!is_build_in_variable))
        {
            code_buffer.append(type_str);
            code_buffer.append(" ");
        }
    }

#if TANGRAM_DEBUG
    if (node->getName() == "_49")
    {
        int aa = 1;
    }
#endif

    if (enable_symbol_name_optimization)
    {
        code_buffer.append(symbol_name_mapper.getSymbolMappedName(node->getId()));
    }
    else
    {
        code_buffer.append(node->getName());
    }
    
    
    if (is_declared == false && node->getType().isArray())
    {
        code_buffer.append(getArraySize(node->getType()));
    }

    if (!node->getConstArray().empty() && (is_declared == false))
    {
        code_buffer.append("=");
        TString type_str = node->getType().getBasicTypeString().c_str();
        assert_t(node->isVector() == false);
        assert_t(node->getType().getBasicType() == EbtFloat);
        code_buffer.append(type_str);
        code_buffer.append("[](");
        const TConstUnionArray&  const_array = node->getConstArray();
        for (int idx = 0; idx < const_array.size(); idx++)
        {
            code_buffer.append(OutputDouble(const_array[idx].getDConst()));
            if (idx != (const_array.size() - 1))
            {
                code_buffer.append(",");
            }
        }
        code_buffer.append(")");
        
    }
    else if (node->getConstSubtree())
    {
        assert_t(false);
    }
}

bool TAstToGLTraverser::visitLoop(TVisit, TIntermLoop* node)
{
    subscope_tranverser.resetSubScopeMaxLine();
    subscope_tranverser.visitLoop(EvPreVisit, node);
    declareSubScopeSymbol();

    bool is_do_while = (!node->testFirst());

    if (is_do_while == false)
    {
        loop_header_tranverser.resetTraverser();
        loop_header_tranverser.visitLoop(EvPreVisit, node);
        for (auto& iter : loop_header_tranverser.getLoopHeaderSymbols())
        {
            TIntermSymbol* symbol_node = iter.second;
            declared_symbols_id.insert(symbol_node->getId());
            code_buffer.append(getTypeText(symbol_node->getType()));
            code_buffer.append(" ");
            if (enable_symbol_name_optimization)
            {
                code_buffer.append(symbol_name_mapper.getSymbolMappedName(symbol_node->getId()));
            }
            else
            {
                code_buffer.append(symbol_node->getName());
            }
            
            code_buffer.append(";");
            if (!enable_line_feed_optimize) { code_buffer.append("\n"); }
        }
    }
    

    if (node->getUnroll()) { assert_t(false); }
    if (node->getLoopDependency()) { assert_t(false); }

    if (is_do_while == false)
    {
        incrementDepth(node);
        parser_context.is_in_loop_header = true;
        code_buffer.append("for(;");
        if (node->getTest()) { node->getTest()->traverse(this); }
        code_buffer.append(";");
        if (node->getTerminal()) { node->getTerminal()->traverse(this); };
        parser_context.is_in_loop_header = false;

        // loop body
        code_buffer.append("){");
        if (node->getBody()) { node->getBody()->traverse(this); }
        code_buffer.append("\n}");

        if (!enable_line_feed_optimize) { code_buffer.append("\n"); }
        decrementDepth();
    }
    else
    {
        incrementDepth(node);
        code_buffer.append("do{");
        if (!enable_line_feed_optimize) { code_buffer.append("\n"); }
        if (node->getBody()) { node->getBody()->traverse(this); }
        code_buffer.append("\n}while(");
        if (node->getTest()) { node->getTest()->traverse(this); }
        code_buffer.append(");\n");
    }

    return false; /* visit the switch node on high level */
}

bool TAstToGLTraverser::visitBranch(TVisit visit, TIntermBranch* node)
{
    TOperator node_operator = node->getFlowOp();
    switch (node->getFlowOp())
    {
    case EOpDefault: NODE_VISIT_FUNC(code_buffer.append("default:"); if (!enable_line_feed_optimize) { code_buffer.append("\n"); }, , );
    case EOpBreak:NODE_VISIT_FUNC(code_buffer.append("break;"); , , );
    case EOpKill:NODE_VISIT_FUNC(code_buffer.append("discard;"); , , );
    case EOpContinue:NODE_VISIT_FUNC(code_buffer.append("continue;");,,);
    case EOpCase: NODE_VISIT_FUNC(code_buffer.append("case "); , , code_buffer.append(":"););
    default:
        assert_t(false);
        break;
    }

    if (node->getExpression() && (node->getFlowOp() != EOpCase))
    {
        assert_t(false);
    }

    return true;
}

bool TAstToGLTraverser::visitSwitch(TVisit visit, TIntermSwitch* node)
{
    subscope_tranverser.resetSubScopeMaxLine();
    subscope_tranverser.visitSwitch(EvPreVisit, node);

    declareSubScopeSymbol();

    code_buffer.append("switch(");
    if (node->getFlatten())
    {
        assert_t(false);
    }

    if (node->getDontFlatten())
    {
        assert_t(false);
    }
    
    // condition
    incrementDepth(node);
    {
        
        node->getCondition()->traverse(this);
        code_buffer.append(")");
        if (!enable_line_feed_optimize) { code_buffer.append("\n"); }
    }

    // body
    {
        code_buffer.append("{");
        node->getBody()->traverse(this);
        code_buffer.append("}");
        if (!enable_line_feed_optimize) { code_buffer.append("\n"); }
    }
    decrementDepth();


    return false; /* visit the switch node on high level */
}

void init_ast_to_glsl()
{
    InitializeProcess();
}

void finish_ast_to_glsl()
{
    FinalizeProcess();
}

bool ast_to_glsl(const char* const* shaderStrings, const int* shaderLengths, char* compiled_buffer, int& compiled_size)
{
    static int invalid_num = 0;

    int shader_size = *shaderLengths;
    int sharp_pos = 0;
    for (int idx = 0; idx < shader_size; idx++)
    {
        if (((const char*)shaderStrings)[idx] == '#')
        {
            if (((const char*)shaderStrings)[idx+1] == 'v')
            {
                sharp_pos = idx;
                break;
            }
        }
    }

    int main_size = shader_size - sharp_pos;
    if (main_size <= 0)
    {
        return false;
    }

	const int defaultVersion = 100;
	const EProfile defaultProfile = ENoProfile;
	const bool forceVersionProfile = false;
	const bool isForwardCompatible = false;

    EShMessages messages = EShMsgCascadingErrors;
    messages = static_cast<EShMessages>(messages | EShMsgAST);
    messages = static_cast<EShMessages>(messages | EShMsgHlslLegalization);

    std::string src_code((const char*)shaderStrings + sharp_pos, main_size);
    {
        glslang::TShader shader(EShLangFragment);
        {
            const char* shader_strings = src_code.data();
            const int shader_lengths = static_cast<int>(src_code.size());
            shader.setStringsWithLengths(&shader_strings, &shader_lengths, 1);
            shader.setEntryPoint("main");
            shader.parse(GetDefaultResources(), defaultVersion, isForwardCompatible, messages);
        }

        TIntermediate* intermediate = shader.getIntermediate();
        TInvalidShaderTraverser invalid_traverser;
        intermediate->getTreeRoot()->traverse(&invalid_traverser);
        if (!invalid_traverser.getIsValidShader())
        {
#if TANGRAM_DEBUG
            std::cout << "invalid num:" << invalid_num << std::endl;
#endif
            invalid_num++;
            return false;
        }

        TIntermAggregate* root_aggregate = intermediate->getTreeRoot()->getAsAggregate();
        if (root_aggregate != nullptr)
        {
            // tranverse linker object at first
            TIntermSequence& sequence = root_aggregate->getSequence();
            TIntermSequence sequnce_reverse;
            for (TIntermSequence::reverse_iterator sit = sequence.rbegin(); sit != sequence.rend(); sit++)
            {
                TString skip_str("compiler_internal_Adjust");
                if ((*sit)->getAsAggregate())
                {
                    TString func_name = (*sit)->getAsAggregate()->getName();
                    if ((func_name.size() > skip_str.size()) && (func_name.substr(0, skip_str.size()) == skip_str))
                    {
                        return false;
                    }
                }
                sequnce_reverse.push_back(*sit);
            }
            sequence = sequnce_reverse;
        }
        else
        {
            assert_t(false);
        }

        TAstToGLTraverser tangram_tranverser;
        tangram_tranverser.preTranverse(intermediate);
        intermediate->getTreeRoot()->traverse(&tangram_tranverser);

        memcpy(compiled_buffer, tangram_tranverser.getCodeBuffer().data(), tangram_tranverser.getCodeBuffer().size());
        compiled_size = tangram_tranverser.getCodeBuffer().size();
    }

    // validation
#if TANGRAM_DEBUG
    std::string dst_code((const char*)compiled_buffer, compiled_size);
    {
        glslang::TShader shader(EShLangFragment);
        {
            const char* shader_strings = dst_code.data();
            const int shader_lengths = static_cast<int>(dst_code.size());
            shader.setStringsWithLengths(&shader_strings, &shader_lengths, 1);
            shader.setEntryPoint("main");
        }
        bool is_success = shader.parse(GetDefaultResources(), defaultVersion, isForwardCompatible, messages);

        if(!is_success)
        {
            {
                std::string out_file_path("H:/TanGram/tangram/source/resource/src.hlsl");
                std::ofstream out_file = std::ofstream(out_file_path, std::ios::out | std::ios::binary);
                out_file.write(src_code.data(), src_code.size());
                out_file.close();
            }

            {
                std::string out_file_path("H:/TanGram/tangram/source/resource/dst.hlsl");
                std::ofstream out_file = std::ofstream(out_file_path, std::ios::out | std::ios::binary);
                out_file.write(dst_code.data(), dst_code.size());
                out_file.close();
            }
        }
        assert_t(is_success);
    }
#endif
    

    return true;
}

