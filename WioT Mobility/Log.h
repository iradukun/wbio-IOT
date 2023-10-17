#pragma once

namespace Sagitech {
namespace Logger {

class ILogger
{
public:
	virtual ~ILogger() {}
	virtual void Log(ELogLevel eLogLevel, const char* zSource, const char* zMessage) = 0;
};

} // namespace Logger
} // namespace Sagitech

