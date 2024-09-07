#include "tangram_utility.h"

#include <boost/preprocessor/cat.hpp>

namespace tangram
{
	const std::string source_path = BOOST_PP_STRINGIZE(TANGRAM_DIR);

	std::string absolutePath(const std::string& relative_path)
	{
		return source_path + relative_path;
	}

	std::string intermediatePath()
	{
		return BOOST_PP_STRINGIZE(TANGRAM_INTERMEDIATE_DIR);
	}
};