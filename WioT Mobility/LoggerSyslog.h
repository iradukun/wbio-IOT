#pragma once

namespace Sagitech {
namespace Logger {

class CLoggerSyslog : public ILogger
{
public:
	CLoggerSyslog();
	virtual ~CLoggerSyslog();
	void Log(ELogLevel eLogLevel, const char* zSource, const char* zMessage) override;
};

} // namespace Logger
} // namespace Sagitech

