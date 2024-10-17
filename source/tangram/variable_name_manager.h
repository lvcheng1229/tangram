#pragma once
#include <vector>
#include <string>
#include <unordered_map>

#include "xxhash.h"

class CUBStructMemberNameManager
{
public:


private:
	int symbol_index;
	std::unordered_map<XXH32_hash_t, std::string> struct_member_hash;
};

class CVariableNameManager
{
public:
	CVariableNameManager()
	{
		symbol_index = 0;
	}

	void getNewSymbolName(const XXH64_hash_t& symbol_hash, std::string& new_name);
	void getNewStructMemberName(const XXH32_hash_t& struct_hash, const XXH32_hash_t& member_hash, std::string& new_name);

private:
	int symbol_index;

	std::unordered_map<XXH32_hash_t, CUBStructMemberNameManager> struct_hash_map;
};