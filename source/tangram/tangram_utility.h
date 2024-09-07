#pragma once
#include <string>
#include <boost/preprocessor/stringize.hpp>

namespace tangram
{
	std::string absolutePath(const std::string& relative_path);
	std::string intermediatePath();
}
