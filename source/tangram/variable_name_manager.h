#pragma once
#include <vector>
#include <string>
#include <unordered_map>

#include "xxhash.h"

class CUBStructMemberNameManager
{
public:
	CUBStructMemberNameManager() {};
	CUBStructMemberNameManager(std::string& struct_name) :struct_name(struct_name), symbol_index(0) {};
	void getOrAddNewStructMemberCombineName(const XXH32_hash_t& member_hash, std::string& new_name);
	void getNewStructMemberName( const XXH32_hash_t& member_hash, std::string& new_name);
	std::string getStructInstanceName()const { return struct_name; }
private:
	std::string struct_name;

	int symbol_index;
	std::unordered_map<XXH32_hash_t, std::string> struct_member_hash;
};

class CVariableNameManager
{
public:
	CVariableNameManager()
	{
		symbol_index = 0;
		struct_symbol_index = 0;
	}

	void getOrAddSymbolName(const XXH64_hash_t& symbol_hash, std::string& new_name);
	void getOrAddNewStructMemberName(const XXH32_hash_t& struct_hash, const XXH32_hash_t& member_hash, std::string& new_name);
	void getNewStructMemberName(const XXH32_hash_t& struct_hash, const XXH32_hash_t& member_hash, std::string& new_name);
	void getNewStructInstanceName(const XXH32_hash_t& struct_hash,std::string& new_name);

private:
	int symbol_index;
	int struct_symbol_index;

	std::unordered_map<XXH32_hash_t, CUBStructMemberNameManager> struct_hash_map;
};