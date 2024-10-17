#include "glslang_helper.h"

int32_t getTypeSize(const TType& type)
{
    TBasicType basic_type = type.getBasicType();
    bool is_vec = type.isVector();
    bool is_mat = type.isMatrix();

    const TQualifier& qualifier = type.getQualifier();
    const TPrecisionQualifier& precision = qualifier.precision;
    assert(precision != TPrecisionQualifier::EpqNone);
    assert(precision != TPrecisionQualifier::EpqLow);

    int32_t basic_size = 0;

    if (is_vec)
    {
        switch (basic_type)
        {
        case EbtDouble:
            basic_size = 8;
            break;
        case EbtInt:
            basic_size = 4;
            break;
        case EbtUint:
            basic_size = 4;
            break;
        case EbtBool:
            assert(false);
            break;
        case EbtFloat:
            basic_size = 4;
        default:
            break;
        }

        basic_size *= type.getVectorSize();
    }

    if (is_mat)
    {
        int mat_raw_num = type.getMatrixRows();
        int mat_col_num = type.getMatrixCols();

        basic_size = 4 * mat_raw_num * mat_col_num;
    }

    if (precision == TPrecisionQualifier::EpqMedium)
    {
        basic_size /= 2;
    }

    return basic_size;
}

TString getTypeText(const TType& type, bool getQualifiers, bool getSymbolName, bool getPrecision, bool getLayoutLocation)
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
	if (getQualifiers)
	{
		if (qualifier.hasLayout())
		{
			appendStr("layout(");
			if (qualifier.hasAnyLocation() && getLayoutLocation)
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

		{
			appendStr(type.getPrecisionQualifierString());
			appendStr(" ");
		}
	}

	if ((getQualifiers == false) && getPrecision)
	{
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

	if (is_vec)
	{
		appendStr("vec");
		appendInt(type.getVectorSize());
	}

	if ((!is_vec) && (!is_blk) && (!is_mat))
	{
		appendStr(type.getBasicTypeString().c_str());
	}

	return type_string;
}
