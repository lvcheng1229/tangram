#include <assert.h>
#include "variable_name_manager.h"

void CVariableNameManager::getNewSymbolName(const XXH64_hash_t& symbol_hash, std::string& new_name)
{
	assert(symbol_index <= 26); //todo
	new_name = char(symbol_index + 'A');
	symbol_index++;
}
