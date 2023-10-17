#include "WioTMobility.h"
#include "LibLog.h"
#include "Log.h"
#include "LoggerFile.h"
#include <sys/time.h>
#include <ctime>

namespace Sagitech {
namespace Logger {

CLoggerFile::CLoggerFile(const char* zFilename)
	: m_zFilename(zFilename)
	, m_hFile(nullptr)
	, m_dtLastFlush(0)
{

}

CLoggerFile::~CLoggerFile()
{
	if (m_hFile)
	{
		fclose(m_hFile);
	}
}

void CLoggerFile::Log(ELogLevel eLogLevel, const char* zSource, const char* zMessage)
{
	if (!m_hFile)
	{
		m_hFile = fopen(m_zFilename, "wt");
		if (!m_hFile)
		{
			return;
		}
	}
	timeval tv;
	gettimeofday(&tv, nullptr);

	tm* tm = localtime(&tv.tv_sec);

	fprintf(m_hFile,
		"\033[1;33m----------------------------------------\033[0m\n"
		"%04d-%02d-%02d %02d:%02d:%02d.%03d\n"
		"%s\n"
		"[%s] %s\n\n",
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, static_cast<int>(tv.tv_usec / 1000),
		LogLevelToString(eLogLevel),
		zSource, zMessage);
	if (tv.tv_sec >= m_dtLastFlush + 1)
	{
		fflush(m_hFile);
		m_dtLastFlush = tv.tv_sec;
	}
}

} // namespace Logger
} // namespace Sagitech
