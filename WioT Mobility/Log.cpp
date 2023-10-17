#include "WioTMobility.h"
#include "LibLog.h"
#include "Log.h"
#include "LoggerStdErr.h"
#include "LoggerFile.h"
#include "LoggerSyslog.h"
#include <cstdarg>

namespace Sagitech {
namespace Logger {

static ELogLevel gs_eLogLevel = ELogLevel::Warning;
static ILogger* gs_pLogger = new CLoggerStdErr;

} // namespace Logger

void SetLogLevel(ELogLevel eLogLevel)
{
	Logger::gs_eLogLevel = eLogLevel;
}

ELogLevel GetLogLevel()
{
	return Logger::gs_eLogLevel;
}

const char* LogLevelToString(ELogLevel ll)
{
	switch (ll)
	{
	case ELogLevel::Emergency:
		return "Emergency";
	case ELogLevel::Alert:
		return "Alert";
	case ELogLevel::Critical:
		return "Critical";
	case ELogLevel::Error:
		return "Error";
	case ELogLevel::Warning:
		return "Warning";
	case ELogLevel::Notice:
		return "Notice";
	case ELogLevel::Informational:
		return "Informational";
	case ELogLevel::Debug:
		return "Debug";
	default :
		return "<unknow>";
	}
}

bool SetLogToFile(const char* zFilename)
{
	delete Logger::gs_pLogger;
	Logger::gs_pLogger = new Logger::CLoggerFile(zFilename);
	return Logger::gs_pLogger != nullptr;
}

bool SetLogToStderr()
{
	delete Logger::gs_pLogger;
	Logger::gs_pLogger = new Logger::CLoggerStdErr;
	return Logger::gs_pLogger != nullptr;
}

bool SetLogToSyslog()
{
	delete Logger::gs_pLogger;
	Logger::gs_pLogger = new Logger::CLoggerSyslog();
	return Logger::gs_pLogger != nullptr;
}


void Log(ELogLevel eLogLevel, const char* zSource, const char* zMessage, ...)
{
	if (eLogLevel <= Logger::gs_eLogLevel && Logger::gs_pLogger)
	{
		char buffer[512];
		va_list args;

		va_start(args, zMessage);
		int len = vsnprintf(buffer, CountOf(buffer), zMessage, args);
		va_end(args);
		if (len >= static_cast<int>(CountOf(buffer)))
		{
			buffer[CountOf(buffer) - 4] = '.';
			buffer[CountOf(buffer) - 3] = '.';
			buffer[CountOf(buffer) - 2] = '.';
			buffer[CountOf(buffer) - 1] = '\0';
		}
		Logger::gs_pLogger->Log(eLogLevel, zSource, buffer);
	}
}

} // namespace Sagitech

