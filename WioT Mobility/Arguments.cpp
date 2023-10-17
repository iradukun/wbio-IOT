#include "WioTMobility.h"
#include "LibLog.h"
#include "Externals.h"
#include "Arguments.h"

#include <cstdarg>
#include <cctype>
#include <cstdio>
#include <memory.h>
#include <string>

namespace Sagitech {

extern const char* g_zLogFilename;
extern bool g_bLogSyslog;

extern bool g_bDaemon;

extern ELogLevel g_eLogLevel;

extern const char* g_zConfigFilename;

int stricmp(const char* a, const char* b)
{
	for (int i = 0; a[i] && b[i]; ++i)
	{
		const char ca = static_cast<char>(tolower(a[i]));
		const char cb = static_cast<char>(tolower(b[i]));
		if (ca == cb)
		{
			continue;
		}
		return (ca > cb) ? 1 : -1;
	}
	return 0;
}

enum class EArgument
{
	None,
	Optionnal,
	Required,
};

struct TOption
{
	std::string zName;
	std::string zDescription;
	EArgument eArg;
	union
	{
		EOptionRet(*CallNoArgument)();
		EOptionRet(*CallWithArgument)(const char *zArgument);
	};
};

static EOptionRet PrintError(const char* zMessage, ...)
{
	printf("%s: ", g_zProgramName);
	
	va_list va;
	va_start(va, zMessage);
	vprintf(zMessage, va);
	va_end(va);

	printf("\nTry '%s -help' for more information.\n", g_zProgramName);
	return EOptionRet::Error;
}

static void PrintHelp();
static void PrintVersion();
static EOptionRet ReadConfigFromFile(const char *zFilename);

static EOptionRet LogLevel(const char* zLogLevel)
{
	if (stricmp(zLogLevel, "Emergency") == 0)
	{
		g_eLogLevel = ELogLevel::Emergency;
		return EOptionRet::Continue;
	}
	if (stricmp(zLogLevel, "Alert") == 0)
	{
		g_eLogLevel = ELogLevel::Alert;
		return EOptionRet::Continue;
	}
	if (stricmp(zLogLevel, "Critical") == 0)
	{
		g_eLogLevel = ELogLevel::Critical;
		return EOptionRet::Continue;
	}
	if (stricmp(zLogLevel, "Error") == 0)
	{
		g_eLogLevel = ELogLevel::Error;
		return EOptionRet::Continue;
	}
	if (stricmp(zLogLevel, "Warning") == 0)
	{
		g_eLogLevel = ELogLevel::Warning;
		return EOptionRet::Continue;
	}
	if (stricmp(zLogLevel, "Notice") == 0)
	{
		g_eLogLevel = ELogLevel::Notice;
		return EOptionRet::Continue;
	}
	if (stricmp(zLogLevel, "Informational") == 0)
	{
		g_eLogLevel = ELogLevel::Informational;
		return EOptionRet::Continue;
	}
	if (stricmp(zLogLevel, "Debug") == 0)
	{
		g_eLogLevel = ELogLevel::Debug;
		return EOptionRet::Continue;
	}
	return PrintError("Unknow log level '%s'\n"
		"Available log levels are:\n"
		"  Emergency\n"
		"  Alert\n"
		"  Critical\n"
		"  Error\n"
		"  Warning\n"
		"  Notice\n"
		"  Informational\n"
		"  Debug", zLogLevel);
}


static EOptionRet LogFile(const char* zFilename)
{
	g_zLogFilename = zFilename;
	g_bLogSyslog = false;
	return EOptionRet::Continue;
}

static const TOption s_Options[] =
{
	{"daemon", "start as a daemon", EArgument::None, []() -> EOptionRet
		{
			g_bDaemon = true;
			return EOptionRet::Continue;
		}
	},
	{zName:"log-file", zDescription:"output logs to specified file", eArg:EArgument::Required, CallWithArgument:LogFile },
	{"log-stderr", "(default) output logs to stderr", EArgument::None, []() -> EOptionRet
		{
			g_zLogFilename = nullptr;
			g_bLogSyslog = false;
			return EOptionRet::Continue;
		}
	},
	{"log-syslog", "send logs to syslog", EArgument::None, []() -> EOptionRet
		{
			g_zLogFilename = nullptr;
			g_bLogSyslog = true;
			return EOptionRet::Continue;
		}
	},
	{zName:"log-level", zDescription:"minimum log level (default:Warning)", eArg:EArgument::Required, CallWithArgument:LogLevel },
	{zName:"config", zDescription:"read configuration from specified file", eArg:EArgument::Required, CallWithArgument:ReadConfigFromFile },
	{"help", "display this help and exit", EArgument::None, []() -> EOptionRet
		{
			PrintHelp();
			return EOptionRet::Exit;
		}
	},
	{"version", "output version information and exit", EArgument::None, []() -> EOptionRet
		{
			PrintVersion();
			return EOptionRet::Exit;
		}
	},
};
constexpr int nOptionCount = CountOf(s_Options);

static void PrintHelp()
{
	printf("Usage: %s [OPTION]...\n%s\n\n", g_zProgramName, g_zProgramDescription);

	int nArgMaxLen = 0;
	for (int opt = 0; opt < nOptionCount; opt++)
	{
		const int len = static_cast<int>(s_Options[opt].zName.length());
		if (nArgMaxLen < len)
		{
			nArgMaxLen = len;
		}
	}

	for (int opt = 0; opt < nOptionCount; opt++)
	{
		printf("  -%-*s  %s\n", nArgMaxLen, s_Options[opt].zName.c_str(), s_Options[opt].zDescription.c_str());
	}
	printf("\n");
}

static void PrintVersion()
{
	printf("%s version %s\n", g_zProgramName, g_zProgramVersion);
	printf("%s\n", g_zProgramCopyright);
}

EOptionRet ReadArguments(const int argc, char* argv[])
{
	for (int arg = 1; arg < argc; ++arg)
	{
		if (argv[arg][0] != '-')
		{
			return PrintError("invalid option '%s'", argv[arg]);
		}

		int opt;
		for (opt = 0; opt < nOptionCount; opt++)
		{
			if (s_Options[opt].zName.compare(argv[arg] + 1) == 0)
			{
				break;
			}
		}
		if (opt >= nOptionCount)
		{
			return PrintError("invalid option '%s'", argv[arg]);
		}

		EOptionRet optRet;
		switch (s_Options[opt].eArg)
		{
		case EArgument::None:
			optRet = s_Options[opt].CallNoArgument();
			break;
		case EArgument::Optionnal:
			if (arg >= argc - 1 || argv[arg + 1][0] == '-')
			{
				optRet = s_Options[opt].CallNoArgument();
			}
			else
			{
				optRet = s_Options[opt].CallWithArgument(argv[++arg]);
			}
			break;
		case EArgument::Required:
			if (arg >= argc - 1 || argv[arg + 1][0] == '-')
			{
				return PrintError("option '%s' require an argument", argv[arg]);
			}
			optRet = s_Options[opt].CallWithArgument(argv[++arg]);
			break;
		}
		if (optRet != EOptionRet::Continue)
		{
			return optRet;
		}
	}

	return EOptionRet::Continue;
}

#if 0
static int ReadNextLine(FILE* file, char* line, const int nMaxLen)
{
	if (!fgets(line, nMaxLen - 1, file))
	{
		return 0;
	}
	if (line[0] == '#' || line[0] == ';')
	{
		// commentaire
		return 0;
	}
	line[nMaxLen - 1] = '\0';
	int len = 0;
	while (line[len])
	{
		++len;
	}
	while (len >= 0)
	{
		const char car = line[len];
		if (car == ' ' || car == '\t' || car == '\r' || car == '\n')
		{
			if (len == 0)
			{
				return 0;
			}
			line[len--] = '\0';
			continue;
		}
		break;
	}
	return len;
}

EOptionRet ReadConfigFromFile(const char* zFilename)
{
	FILE* file = fopen(zFilename, "rt");
	if (!file)
	{
		return PrintError("failed to open config file '%s'", zFilename);
	}

	// dummy loop
	do
	{
		CRegexpA regex("^\\s*(?>([\\w-]+)=(.*)|([\\w-]+))$");
		char line[256]="";
		int lineNumber = 0;
		while (!feof(file))
		{
			lineNumber++;
			if (!ReadNextLine(file, line, CountOf(line)))
			{
				continue;
			}

			MatchResult result;
			result = regex.Match(line);
			if (!result.IsMatched())
			{
				PrintError("error read file '%s' at line %d : invalid format", zFilename, lineNumber);
				break;
			}

			EArgument arg;
			unsigned len, start;
			if (result.GetGroupStart(1) >= 0)
			{
				start = result.GetGroupStart(1);
				len = result.GetGroupEnd(1) - start;
				arg = EArgument::Required;
			}
			else if (result.GetGroupStart(3) >= 0)
			{
				start = result.GetGroupStart(3);
				len = result.GetGroupEnd(3) - start;
				arg = EArgument::None;
			}
			else
			{
				PrintError("error read file '%s' at line %d : unexpected error", zFilename, lineNumber);
				break;
			}

			char name[64];
			if (len >= CountOf(name))
			{
				PrintError("error read file '%s' at line %d : option name too long", zFilename, lineNumber);
				break;
			}
			memcpy(name, line + start, len * sizeof(char));
			name[len] = '\0';

			int opt;
			for (opt = 0; opt < nOptionCount; opt++)
			{
				if (strcmp(s_Options[opt].zName, name) == 0)
				{
					break;
				}
			}
			if (opt >= nOptionCount)
			{
				return PrintError("invalid option '%s'", name);
			}

			EOptionRet optRet;
			if (arg == EArgument::None)
			{
				if (s_Options[opt].eArg == EArgument::Required)
				{
					PrintError("error read file '%s' at line %d : option '%s' requires an argument", zFilename, lineNumber, name);
					break;
				}
				optRet = s_Options[opt].CallNoArgument();
			}
			else
			{
				if (s_Options[opt].eArg == EArgument::None)
				{
					PrintError("error read file '%s' at line %d : option '%s' does not accept an argument", zFilename, lineNumber, name);
					break;
				}
				optRet = s_Options[opt].CallWithArgument(line+result.GetGroupStart(2));
			}
			if (optRet != EOptionRet::Continue)
			{
				break;
			}
		}

		fclose(file);
		return EOptionRet::Continue;
	} while (false);

	fclose(file);
	return EOptionRet::Error;
}
#else
EOptionRet ReadConfigFromFile(const char* zFilename)
{
	g_zConfigFilename = zFilename;
	return EOptionRet::Continue;
}
#endif

} // namespace Sagitech
