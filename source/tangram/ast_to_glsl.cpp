#include "ast_tranversar_private.h"


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
        int symbol_max_line = symbols_max_line->find(symbol_node->getId())->second;

        if (symbol_max_line > scope_max_line)
        {
            declared_symbols_id.insert(symbol_node->getId());
            code_buffer.append(getTypeText(symbol_node->getType()));
            code_buffer.append(" ");
            code_buffer.append(symbol_node->getName());
            code_buffer.append(";");
            if (!enable_line_feed_optimize)
            {
                code_buffer.append("\n");
            }
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
    
    bool should_output_precision_str = ((basic_type == EbtFloat) && type.getQualifier().precision != EpqNone && (!(ignore_medium_presion_out && type.getQualifier().precision == EpqMedium))) || basic_type == EbtSampler;
    
    if(getQualifiers)
    {
        const TQualifier& qualifier = type.getQualifier();
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


    if (type.isMatrix())
    {
        assert_t(false);
    }

    if (type.isVector())
    {
        appendStr("vec");
        appendInt(type.getVectorSize());
    }

  

    if ((!is_vec) && (!is_blk))
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
    const auto binary_const_union_pre_left = [&]() {
        TIntermConstantUnion* contant_union = node->getLeft()->getAsConstantUnion();
        if (contant_union)
        {
            int array_size = contant_union->getConstArray().size();
            if (array_size > 1)
            {
                code_buffer.append("vec");
                code_buffer.append(std::to_string(array_size).c_str());
                code_buffer.append("(");
                parser_context.is_vector_times_scalar = true;
            }
        } };

    const auto binary_const_union_in_left = [&]() {
        TIntermConstantUnion* contant_union = node->getLeft()->getAsConstantUnion();
        if (contant_union)
        {
            int array_size = contant_union->getConstArray().size();
            if (array_size > 1)
            {
                parser_context.is_vector_times_scalar = false;
                code_buffer.append(")");
            }
        }};

    const auto binary_const_union_in_right = [&]() {
        TIntermConstantUnion* contant_union = node->getRight()->getAsConstantUnion();
        if (contant_union)
        {
            int array_size = contant_union->getConstArray().size();
            if (array_size > 1)
            {
                code_buffer.append("vec");
                code_buffer.append(std::to_string(array_size).c_str());
                code_buffer.append("(");
                parser_context.is_vector_times_scalar = true;
            }
        } };

    const auto binary_const_union_post_right = [&]() {
        TIntermConstantUnion* contant_union = node->getRight()->getAsConstantUnion();
        if (contant_union)
        {
            int array_size = contant_union->getConstArray().size();
            if (array_size > 1)
            {
                parser_context.is_vector_times_scalar = false;
                code_buffer.append(")");
            }
        }};

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
                code_buffer.append(getTypeText(node->getType()));
                code_buffer.append(" ");
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
            code_buffer.append(";");;
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
    case EOpIndexDirect: NODE_VISIT_FUNC(, 
        if (node->getLeft()->getType().isArray()) 
        { 
            code_buffer.append("["); 
        }
        else
        {
            code_buffer.append(".");
            parser_context.is_vector_swizzle = true;
        },
        if (node->getLeft()->getType().isArray())
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

    case EOpAdd:NODE_VISIT_FUNC(code_buffer.append("(");binary_const_union_pre_left(), binary_const_union_in_left(); code_buffer.append("+"); binary_const_union_in_right(), binary_const_union_post_right();code_buffer.append(")"); );
    case EOpSub:NODE_VISIT_FUNC(code_buffer.append("(");binary_const_union_pre_left(), binary_const_union_in_left(); code_buffer.append("-"); binary_const_union_in_right(), binary_const_union_post_right();code_buffer.append(")"); );
    case EOpMul:NODE_VISIT_FUNC(code_buffer.append("(");binary_const_union_pre_left(), binary_const_union_in_left(); code_buffer.append("*"); binary_const_union_in_right(), binary_const_union_post_right();code_buffer.append(")"); );
    case EOpDiv:NODE_VISIT_FUNC(code_buffer.append("(");binary_const_union_pre_left(), binary_const_union_in_left(); code_buffer.append("/"); binary_const_union_in_right(), binary_const_union_post_right();code_buffer.append(")"); );
    case EOpMod:NODE_VISIT_FUNC(code_buffer.append("(");binary_const_union_pre_left(), binary_const_union_in_left(); code_buffer.append("%"); binary_const_union_in_right(), binary_const_union_post_right();code_buffer.append(")"); );
    case EOpRightShift:NODE_VISIT_FUNC(, code_buffer.append(">>"), );
    case EOpLeftShift:NODE_VISIT_FUNC(, code_buffer.append("<<"), );
    case EOpAnd:NODE_VISIT_FUNC(, code_buffer.append("&&"), );
    case EOpEqual:NODE_VISIT_FUNC(, code_buffer.append("=="), );
    case EOpNotEqual:NODE_VISIT_FUNC(, code_buffer.append("!="), );
    case EOpLessThan:NODE_VISIT_FUNC(, code_buffer.append("<"), );
    case EOpGreaterThan:NODE_VISIT_FUNC(, code_buffer.append(">"), );
    case EOpLessThanEqual:NODE_VISIT_FUNC(, code_buffer.append("<="), );
    case EOpGreaterThanEqual:NODE_VISIT_FUNC(, code_buffer.append(">="), );
    case EOpVectorTimesScalar:NODE_VISIT_FUNC(binary_const_union_pre_left(), binary_const_union_in_left(); code_buffer.append("*");, ;);
    case EOpMatrixTimesVector:NODE_VISIT_FUNC(, code_buffer.append("*"); binary_const_union_in_right(); , binary_const_union_post_right(););
    case EOpMatrixTimesScalar:NODE_VISIT_FUNC(, code_buffer.append("*"), );
        
    case EOpVectorSwizzle:NODE_VISIT_FUNC(, code_buffer.append("."); parser_context.is_vector_swizzle = true; , parser_context.is_vector_swizzle = false);
    default:
    {
        assert_t(false);
        break;
    }
    };

    return true;
}

bool TAstToGLTraverser::visitUnary(TVisit visit, TIntermUnary* node)
{
    TOperator node_operator = node->getOp();
    switch (node_operator)
    {
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
    case EOpFract:  NODE_VISIT_FUNC(code_buffer.append("frac("); , , code_buffer.append(")"); );


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

    case EOpLength: NODE_VISIT_FUNC(code_buffer.append("length(");, , code_buffer.append(")"); );
    case EOpNormalize: NODE_VISIT_FUNC(code_buffer.append("normalize("); , , code_buffer.append(")"); );
    case EOpReflect: NODE_VISIT_FUNC(code_buffer.append("reflect(");, , code_buffer.append(")"); );

    case EOpNegative:NODE_VISIT_FUNC(code_buffer.append("-");, , );

    // GLSL spec 4.5
    // 4.1.10. Implicit Conversions
    case EOpConvUintToInt:     NODE_VISIT_FUNC(code_buffer.append("int(");,, code_buffer.append(")"););

    case EOpConvUintToFloat:NODE_VISIT_FUNC();

    // float32_t -> uint*
    case EOpConvFloatToUint8:  NODE_VISIT_FUNC(code_buffer.append("uint8(");,, code_buffer.append(")"););
    case EOpConvFloatToUint16: NODE_VISIT_FUNC(code_buffer.append("uint16(");,, code_buffer.append(")"););;
    case EOpConvFloatToUint:   NODE_VISIT_FUNC(code_buffer.append("uint(");,, code_buffer.append(")"););
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
        break;
    }

    case EOpMin: NODE_VISIT_FUNC(code_buffer.append("min("); , code_buffer.append(","), code_buffer.append(")"); );
    case EOpMax: NODE_VISIT_FUNC(code_buffer.append("max("); , code_buffer.append(","), code_buffer.append(")"); );
    case EOpClamp: NODE_VISIT_FUNC(code_buffer.append("clamp("); , code_buffer.append(","), code_buffer.append(")"); );
    case EOpMix: NODE_VISIT_FUNC(code_buffer.append("mix("); , code_buffer.append(","), code_buffer.append(")"); );
    case EOpStep: NODE_VISIT_FUNC(code_buffer.append("step("); code_buffer.append(","), , code_buffer.append(")"); );

    case EOpDistance: NODE_VISIT_FUNC(code_buffer.append("distance("); , code_buffer.append(","), code_buffer.append(")"); );
    case EOpDot: NODE_VISIT_FUNC(code_buffer.append("dot("); , code_buffer.append(","), code_buffer.append(")"); );
    case EOpCross: NODE_VISIT_FUNC(code_buffer.append("cross("); , code_buffer.append(","), code_buffer.append(")"); );

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

    case EOpLinkerObjects:
    {
        if (visit == EvPreVisit)
        {

        }
        else if (visit == EvInVisit)
        {
            code_buffer.append(";");
            if (!enable_line_feed_optimize)
            {
                code_buffer.insert(code_buffer.end(), '\n');
            }
        }
        else if (visit == EvPostVisit)
        {
            code_buffer.append(";");
            if (!enable_line_feed_optimize)
            {
                code_buffer.insert(code_buffer.end(), '\n');
            }
        }
        break;
    }
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

    code_buffer.append("if(");

    //condition
    incrementDepth(node);
    {
        node->getCondition()->traverse(this);
        code_buffer.append("){");
        if (!enable_line_feed_optimize) { code_buffer.append("\n"); }
        
    }

    {
        if (node->getTrueBlock())
        {
            node->getTrueBlock()->traverse(this);
        }
        if (!enable_line_feed_optimize) { code_buffer.append("\n"); }
        code_buffer.append("}");
        if (!enable_line_feed_optimize) { code_buffer.append("\n"); }
    }

    {
        if (node->getFalseBlock())
        {
            code_buffer.append("else{");
            node->getFalseBlock()->traverse(this);
            if (!enable_line_feed_optimize) { code_buffer.append("\n"); }
            code_buffer.append("}");
            if (!enable_line_feed_optimize) { code_buffer.append("\n"); }
        }
    }


    decrementDepth();
    

    return false;  /* visit the selection node on high level */
}

static char unionConvertToChar(int index)
{
    static const char const_indices[4] = { 'x','y','z','w' };
    return const_indices[index];
}

static TString OutputDouble(double value)
{
    return std::to_string(value).c_str();
};

void TAstToGLTraverser::outputConstantUnion(const TIntermConstantUnion* node, const TConstUnionArray& constUnion)
{
    int size = node->getType().computeNumComponents();

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
            code_buffer.append(OutputDouble(constUnion[i].getDConst()));
            break;
        }
        case EbtUint:
        {
            code_buffer.append(std::to_string(constUnion[i].getUConst()).c_str());
            code_buffer.append("u");
            break;
        }
        default:
        {
            assert_t(false);
            break;
        }
        }

        if (parser_context.is_vector_times_scalar && (i != (size - 1)))
        {
            code_buffer.append(",");
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
        code_buffer.append(getTypeText(node->getType()));
        code_buffer.append(" ");
    }

    code_buffer.append(node->getName());
    
    if (is_declared == false && node->getType().isArray())
    {
        code_buffer.append(getArraySize(node->getType()));
    }

    if (!node->getConstArray().empty())
    {
        assert_t(false);
    }
    else if (node->getConstSubtree())
    {
        assert_t(false);
    }
}

bool TAstToGLTraverser::visitLoop(TVisit, TIntermLoop* node)
{
    return false;
}

bool TAstToGLTraverser::visitBranch(TVisit visit, TIntermBranch* node)
{
    TOperator node_operator = node->getFlowOp();
    switch (node->getFlowOp())
    {
    case EOpDefault: NODE_VISIT_FUNC(
        code_buffer.append("default:");
        if (!enable_line_feed_optimize) { code_buffer.append("\n"); }
        //code_buffer.append("{");
        //if (!enable_line_feed_optimize) { code_buffer.append("\n"); }
        , 
           , 
        //code_buffer.append("}"); 
        //if (!enable_line_feed_optimize) { code_buffer.append("\n"); }
        );
    case EOpBreak:NODE_VISIT_FUNC(code_buffer.append("break;");,,);
    }

    if (node->getExpression())
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
        if (!enable_line_feed_optimize)
        {
            code_buffer.append("\n");
        }
    }

    // body
    {
        code_buffer.append("{");
        node->getBody()->traverse(this);
        code_buffer.append("}");
        if (!enable_line_feed_optimize)
        {
            code_buffer.append("\n");
        }
    }
    decrementDepth();


    return false; /* visit the switch node on high level */
}



void ast_to_glsl(const char* const* shaderStrings, const int* shaderLengths, char* compiled_buffer, int& compiled_size)
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


    InitializeProcess();

	glslang::TShader shader(EShLangFragment);
    {
        const char* shader_strings = code.data();
        const int shader_lengths = static_cast<int>(code.size());
        shader.setStringsWithLengths(&shader_strings, &shader_lengths, 1);
        shader.setEntryPoint("main");
        shader.parse(GetDefaultResources(), defaultVersion, isForwardCompatible, messages);
    }

	TIntermediate* intermediate = shader.getIntermediate();

    TAstToGLTraverser tangram_tranverser;
    
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

    memcpy(compiled_buffer, tangram_tranverser.getCodeBuffer().data(), tangram_tranverser.getCodeBuffer().size());
    compiled_size = tangram_tranverser.getCodeBuffer().size();
    
    FinalizeProcess();
}

