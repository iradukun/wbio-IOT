#pragma once

namespace Sagitech {

enum class ELogLevel
{
	Emergency,
	Alert,
	Critical,
	Error,
	Warning,
	Notice,
	Informational,
	Debug,
};

void Log(ELogLevel eLogLevel, const char* zSource, const char* zMessage, ...);

void SetLogLevel(ELogLevel eLogLevel);
ELogLevel GetLogLevel();

bool SetLogToFile(const char* zFilename);
bool SetLogToStderr();
bool SetLogToSyslog();

const char* LogLevelToString(ELogLevel ll);

#define LogDebug(src, msg...)	Sagitech::Log(Sagitech::ELogLevel::Debug, src, msg)
#define LogInf(src, msg...)	Sagitech::Log(Sagitech::ELogLevel::Informational, src, msg)
#define LogNotice(src, msg...)	Sagitech::Log(Sagitech::ELogLevel::Notice, src, msg)
#define LogWarn(src, msg...)	Sagitech::Log(Sagitech::ELogLevel::Warning, src, msg)
#define LogErr(src, msg...)	Sagitech::Log(Sagitech::ELogLevel::Error, src, msg)

} // namespace Sagitech

