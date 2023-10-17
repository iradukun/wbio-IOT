#pragma once

namespace Sagitech {
namespace Logger {

class CLoggerFile : public ILogger
{
public:
	CLoggerFile(const char *zFilename);
	virtual ~CLoggerFile();
	void Log(ELogLevel eLogLevel, const char* zSource, const char* zMessage) override;

private:
	const char* m_zFilename;
	FILE* m_hFile;
	time_t m_dtLastFlush;
};

} // namespace Logger
} // namespace Sagitech

