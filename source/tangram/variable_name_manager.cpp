#include <assert.h>
#include "variable_name_manager.h"

void CUBStructMemberNameManager::getOrAddNewStructMemberCombineName(const XXH32_hash_t& member_hash, std::string& new_name)
{
	const auto& iter = struct_member_hash.find(member_hash);
	if (iter != struct_member_hash.end())
	{
		new_name = struct_name + "." + iter->second;
	}
	else
	{
		assert(symbol_index <= 26); //todo
		struct_member_hash[member_hash] = char(symbol_index + 'A');
		new_name = struct_name + "." + char(symbol_index + 'A');
		symbol_index++;

	}
}

void CUBStructMemberNameManager::getNewStructMemberName(const XXH32_hash_t& member_hash, std::string& new_name)
{
	const auto& iter = struct_member_hash.find(member_hash);
	assert(iter != struct_member_hash.end());
	new_name = iter->second;
}

void CVariableNameManager::getOrAddSymbolName(const XXH64_hash_t& symbol_hash, std::string& new_name)
{
	assert(symbol_index <= 26); //todo
	new_name = char(symbol_index + 'A');
	
	assert(symbol_index != 26); // 我们将第26个字母预留给Uniform Buffer
	
	symbol_index++;
}

void CVariableNameManager::getOrAddNewStructMemberName(const XXH32_hash_t& struct_hash, const XXH32_hash_t& member_hash, std::string& new_name)
{
	const auto& iter = struct_hash_map.find(struct_hash);
	if (iter != struct_hash_map.end())
	{
		CUBStructMemberNameManager& ub_struct_member_manager = iter->second;
		ub_struct_member_manager.getOrAddNewStructMemberCombineName(member_hash, new_name);
	}
	else
	{
		std::string struct_name = std::string("Z") + char(struct_symbol_index + 'A');
		struct_hash_map[struct_hash] = CUBStructMemberNameManager(struct_name);
		struct_hash_map[struct_hash].getOrAddNewStructMemberCombineName(member_hash, new_name);
		struct_symbol_index++;
	}
}

void CVariableNameManager::getNewStructMemberName(const XXH32_hash_t& struct_hash, const XXH32_hash_t& member_hash, std::string& new_name)
{
	const auto& iter = struct_hash_map.find(struct_hash);
	assert(iter != struct_hash_map.end());
	CUBStructMemberNameManager& ub_struct_member_manager = iter->second;
	ub_struct_member_manager.getNewStructMemberName(member_hash, new_name);
}

void CVariableNameManager::getNewStructInstanceName(const XXH32_hash_t& struct_hash, std::string& new_name)
{
	const auto& iter = struct_hash_map.find(struct_hash);
	assert(iter != struct_hash_map.end());
	CUBStructMemberNameManager& ub_struct_member_manager = iter->second;
	new_name = ub_struct_member_manager.getStructInstanceName();
}