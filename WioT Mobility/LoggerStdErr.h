#pragma once

namespace Sagitech {
namespace Logger {

class CLoggerStdErr : public ILogger
{
public:
	CLoggerStdErr();
	virtual ~CLoggerStdErr();
	void Log(ELogLevel eLogLevel, const char* zSource, const char* zMessage) override;
};

} // namespace Logger
} // namespace Sagitech

