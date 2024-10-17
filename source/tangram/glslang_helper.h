#pragma once
#include "glslang_headers.h"
using namespace glslang;
int32_t getTypeSize(const TType& type);
TString getTypeText(const TType& type, bool getQualifiers = true, bool getSymbolName = false, bool getPrecision = true, bool getLayoutLocation = true);