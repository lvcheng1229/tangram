#include <assert.h>
#include "variable_name_manager.h"

void CVariableNameManager::getNewSymbolName(const XXH64_hash_t& symbol_hash, std::string& new_name)
{
	assert(symbol_index <= 26); //todo
	new_name = char(symbol_index + 'A');
	
	assert(symbol_index != 26); // 我们将第26个字母预留给Uniform Buffer
	
	symbol_index++;
}

void CVariableNameManager::getNewStructMemberName(const XXH32_hash_t& struct_hash, const XXH32_hash_t& member_hash, std::string& new_name)
{

}