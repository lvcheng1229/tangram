#pragma once
#include <vector>
#include <string>
#include <unordered_map>

#include "xxhash.h"
#include "common.h"

class CVariableNameManager
{
public:
	CVariableNameManager()
	{
		symbol_index = 0;
	}

	void getNewSymbolName(const XXH64_hash_t& symbol_hash, std::string& new_name);
private:
	int symbol_index;
};