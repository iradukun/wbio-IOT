#include "WioTMobility.h"
#include "LibLog.h"
#include "Log.h"
#include "LoggerStdErr.h"
#include <sys/time.h>
#include <ctime>

namespace Sagitech {
namespace Logger {

CLoggerStdErr::CLoggerStdErr()
{

}

CLoggerStdErr::~CLoggerStdErr()
{

}

static const char * LevelToColor(ELogLevel ll)
{
	switch (ll)
	{
	case ELogLevel::Emergency:
	case ELogLevel::Alert:
	case ELogLevel::Critical:
	case ELogLevel::Error:
		return "\033[1;97;101m";
	case ELogLevel::Warning:
		return "\033[1;97;43m";
	case ELogLevel::Notice:
	case ELogLevel::Informational:
		return "\033[1;37;102m";
	case ELogLevel::Debug:
		return "\033[1;37;45m";
	default:
		return "";
	}
}

void CLoggerStdErr::Log(ELogLevel eLogLevel, const char* zSource, const char* zMessage)
{
	timeval tv;
	gettimeofday(&tv, nullptr);

	struct tm *tm=localtime(&tv.tv_sec);

	fprintf(stderr,
		"\033[1;33m----------------------------------------\033[0m\n"
		"%04d-%02d-%02d %02d:%02d:%02d.%03d\n"
		"%s%s\033[0m\n"
		"[%s] %s\n\n",
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, static_cast<int>(tv.tv_usec / 1000),
		LevelToColor(eLogLevel), LogLevelToString(eLogLevel),
		zSource, zMessage);
}

} // namespace Logger
} // namespace Sagitech
