#pragma once
#include "glslang_headers.h"
using namespace glslang;
int32_t getTypeSize(const TType& type);
TString getTypeText_HashTree(const TType& type, bool getQualifiers = true, bool getSymbolName = false, bool getPrecision = true, bool getLayoutLocation = true);
TString getTypeText(const TType& type, bool getQualifiers = true, bool getSymbolName = false, bool getPrecision = true);

template<typename T>
T alignValue(T value,size_t align)
{
	return ((value + (align - 1)) / align) * align;
}