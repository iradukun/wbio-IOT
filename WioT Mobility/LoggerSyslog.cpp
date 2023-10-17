#include "WioTMobility.h"
#include "LibLog.h"
#include "Log.h"
#include "LoggerSyslog.h"
#include <syslog.h>

namespace Sagitech {
namespace Logger {

CLoggerSyslog::CLoggerSyslog()
{
	openlog(nullptr, LOG_CONS | LOG_NDELAY, LOG_USER);
}

CLoggerSyslog::~CLoggerSyslog()
{
	closelog();
}

void CLoggerSyslog::Log(ELogLevel eLogLevel, const char* zSource, const char* zMessage)
{
	syslog(static_cast<int>(eLogLevel), "[%s] %s", zSource, zMessage);
}

} // namespace Logger
} // namespace Sagitech
