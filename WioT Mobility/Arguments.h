#pragma once

namespace Sagitech {

enum class EOptionRet
{
	Error = -1,
	Exit,
	Continue,
};

EOptionRet ReadArguments(const int argc, char* argv[]);

} // namespace Sagitech
