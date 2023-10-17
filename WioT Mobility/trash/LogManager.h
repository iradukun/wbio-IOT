#pragma once
#include <stdarg.h>

class CLogManager
{
private:
	CLogManager();
	~CLogManager();

public:
	static CLogManager& GetInstance();

	enum class ELevel
	{
		None,
		Error,
		Warning, 
		Inf,
		Debug,
	};

	void Log(ELevel eLevel, const char* zMessage, va_list args);


	void SetLogLevel(ELevel eLevel);
	ELevel GetLogLevel() const;
private:
	ELevel m_eMaxLevel;
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void LogInf(const char* zMessage, ...)
{
	va_list args;
	va_start(args, zMessage);

	CLogManager::GetInstance().Log(CLogManager::ELevel::Inf, zMessage, args);

	va_end(args);
}

static void LogWarn(const char* zMessage, ...)
{
	va_list args;
	va_start(args, zMessage);

	CLogManager::GetInstance().Log(CLogManager::ELevel::Warning, zMessage, args);

	va_end(args);
}

static void LogErr(const char* zMessage, ...)
{
	va_list args;
	va_start(args, zMessage);

	CLogManager::GetInstance().Log(CLogManager::ELevel::Error, zMessage, args);

	va_end(args);
}

static void LogDebug(const char* zMessage, ...)
{
	va_list args;
	va_start(args, zMessage);

	CLogManager::GetInstance().Log(CLogManager::ELevel::Debug, zMessage, args);

	va_end(args);
}
#pragma GCC diagnostic pop
