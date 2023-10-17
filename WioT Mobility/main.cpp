#include <cstdio>
#include <cstdlib>
#include "WioTMobility.h"

#include "ConnectionManager.h"
#include "SecurityManager.h"
#include "DeviceManager.h"
#include "CommandManager.h"
#include "DataManager.h"

#include "IConnectionPool.h"
#include "ProtocolTeltonika.h"
#include "ProtocolDualCam.h"
#include "Arguments.h"
#include "LibConfig.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#include "deelx.h"
#pragma GCC diagnostic pop

#include "boost/thread/mutex.hpp"
#include "boost/thread/condition.hpp"

#include <execinfo.h>
#include <signal.h>
#include <type_traits>
#include <sys/resource.h>

using namespace Sagitech;

static boost::mutex gs_Mutex;
static boost::condition gs_SignalEvent;

namespace Sagitech {

#define PRG_COPYRIGHT "Copyright (C) 2021 SAGITECH-SOFTWARE"

const char* g_zProgramName = "WioT_Mobility";
const char* g_zProgramDescription = "WioT Mobility server";
const char* g_zProgramVersion = "0.1b - build " __DATE__ " " __TIME__;
const char* g_zProgramCopyright = PRG_COPYRIGHT;

const char* g_zLogFilename;
bool g_bLogSyslog;
ELogLevel g_eLogLevel = ELogLevel::Warning;
bool g_bDaemon;
const char* g_zConfigFilename = "mobility.conf";

} // namespace Sagitech

constexpr int MaxFileCount = 10000;

static bool InitLog()
{
	SetLogLevel(g_eLogLevel);

	if (g_zLogFilename)
	{
		return SetLogToFile(g_zLogFilename);
	}
	if (g_bLogSyslog)
	{
		return SetLogToSyslog();
	}
	return SetLogToStderr();
}

static bool Daemonize()
{
	if (!g_bDaemon)
	{
		return true;
	}
	if (fork() != 0)
	{
		exit(0);
	}
	chdir("/");
	setsid();
	const long openMax = sysconf(_SC_OPEN_MAX);
	for (int i = 0; i < openMax; ++i)
	{
		close(i);
	}
	return true;
}

void SigHandlerDump(int sig)
{
	if (sig == SIGPIPE)
	{
		return;
	}
	void* array[20];

	// get void*'s for all entries on the stack
	auto size = backtrace(array, 20);

	// print out all the frames to stderr
	fprintf(stderr, "Error: signal %d:\n", sig);
	backtrace_symbols_fd(array, size, STDERR_FILENO);
	exit(1);
}

void SigHandlerQuit(int sig)
{
	LogInf("WioT", "Received signal to quit");

	boost::mutex::scoped_lock lock(gs_Mutex);
	gs_SignalEvent.notify_one();
}

void SigHandlerUser(int sig)
{
	if (sig == SIGUSR1)
	{
		return;
	}
	if (sig == SIGUSR2)
	{
		return;
	}
}


static char gs_zDatabaseServers[512] = "34.197.55.103:5002";
static char gs_zDatabaseTopicVehicle[512] = "kafka-iot-vehicle-data-topic";
static char gs_zDatabaseClientId[512] = "WioTMobility";

static char gs_zCertificateFile[512] = "cert.pem";
static char gs_zPrivateKeyFile[512] = "key.pem";


static void FillConfig()
{
	Config::NewDescription();

	Config::BeginSection("Database");

	Config::AddString("Servers", "List of kafka servers", "Kafka servers",
		Config::EVariableFlag::NeedRestart,
		[](const char* pValue, void* pContext)->bool {
			if (!pValue)
			{
				return false;
			}
			if (strlen(pValue) >= CountOf(gs_zDatabaseServers) - 1)
			{
				LogErr("WioT", "Cannot set ServerUrl : given file name is too long");
				return false;
			}
			strcpy(gs_zDatabaseServers, pValue);
			return true;
		},
		[](char* pValue, const int nBufferLength, void* pContext)->bool {
			if (!pValue)
			{
				return false;
			}
			if (static_cast<int>(strlen(gs_zDatabaseServers)) >= nBufferLength - 1)
			{
				LogErr("WioT", "Cannot get ServerUrl : buffer is too small");
				return false;
			}
			strcpy(pValue, gs_zDatabaseServers);
			return true;
		});
	Config::AddString("ClientId", "Client Id", "Client Id",
		Config::EVariableFlag::NeedRestart,
		[](const char* pValue, void* pContext)->bool {
			if (!pValue)
			{
				return false;
			}
			if (strlen(pValue) >= CountOf(gs_zDatabaseClientId) - 1)
			{
				LogErr("WioT", "Cannot set ClientId : given file name is too long");
				return false;
			}
			strcpy(gs_zDatabaseClientId, pValue);
			return true;
		},
		[](char* pValue, const int nBufferLength, void* pContext)->bool {
			if (!pValue)
			{
				return false;
			}
			if (static_cast<int>(strlen(gs_zDatabaseClientId)) >= nBufferLength - 1)
			{
				LogErr("WioT", "Cannot get ClientId : buffer is too small");
				return false;
			}
			strcpy(pValue, gs_zDatabaseClientId);
			return true;
		});


	Config::AddString("TopicGps", "Topic Vehicle", "Topic for Vehicle Data",
		Config::EVariableFlag::NeedRestart,
		[](const char* pValue, void* pContext)->bool {
			if (!pValue)
			{
				return false;
			}
			if (strlen(pValue) >= CountOf(gs_zDatabaseTopicVehicle) - 1)
			{
				LogErr("WioT", "Cannot set TopicGps : given file name is too long");
				return false;
			}
			strcpy(gs_zDatabaseTopicVehicle, pValue);
			return true;
		},
		[](char* pValue, const int nBufferLength, void* pContext)->bool {
			if (!pValue)
			{
				return false;
			}
			if (static_cast<int>(strlen(gs_zDatabaseTopicVehicle)) >= nBufferLength - 1)
			{
				LogErr("WioT", "Cannot get TopicGps : buffer is too small");
				return false;
			}
			strcpy(pValue, gs_zDatabaseTopicVehicle);
			return true;
		});	
	Config::EndSection();

	Config::BeginSection("Security");
	Config::AddString("CertificateFile", "Certificate File", "SSL Certificate File",
		Config::EVariableFlag::NeedRestart,
		[](const char* pValue, void* pContext)->bool {
			if (!pValue)
			{
				return false;
			}
			if (strlen(pValue) >= CountOf(gs_zCertificateFile) - 1) {
				LogErr("WioT", "Cannot set CertificateFile: given file name is too long");
				return false;
			}
			strcpy(gs_zCertificateFile, pValue);
			return true;
		},
		[](char* pValue, const int nBufferLength, void* pContext)->bool {
			if (!pValue)
			{
				return false;
			}
			if (static_cast<int>(strlen(gs_zCertificateFile)) >= nBufferLength - 1)
			{
				LogErr("WioT", "Cannot get CertificateFile: buffer is too small");
				return false;
			}
			strcpy(pValue, gs_zCertificateFile);
			return true;
		});
	Config::AddString("PrivateKeyFile", "Private Key File", "SSL Private Key File",
		Config::EVariableFlag::NeedRestart,
		[](const char* pValue, void* pContext)->bool {
			if (!pValue)
			{
				return false;
			}
			if (strlen(pValue) >= CountOf(gs_zPrivateKeyFile) - 1)
			{
				LogErr("WioT", "Cannot set PrivateKeyFile: given file name is too long");
				return false;
			}
			strcpy(gs_zPrivateKeyFile, pValue);
			return true;
		},
		[](char* pValue, const int nBufferLength, void* pContext)->bool {
			if (!pValue)
			{
				return false;
			}
			if (static_cast<int>(strlen(gs_zPrivateKeyFile)) >= nBufferLength - 1)
			{
				LogErr("WioT", "Cannot get PrivateKeyFile: buffer is too small");
				return false;
			}
			strcpy(pValue, gs_zPrivateKeyFile);
			return true;
		});
	Config::AddString("Blacklist", "Blacklist an IP address", "Blacklist an IP address",
		Config::EVariableFlag::NeedRestart,
		[](const char* pValue, void* pContext)->bool {
			if (!pValue)
			{
				return false;
			}

			if (!SecurityManager.Blacklist(pValue))
			{
				LogErr("WioT", "Cannot add \"%s\" to the IP Blacklist", pValue);
				return false;
			}

			return true;
		},
		nullptr);
	Config::EndSection();

	Config::BeginSection("Connection");

	for (int lPoolIdx = 0; lPoolIdx < MaxConnectionPool; ++lPoolIdx)
	{
		char zPoolIdx[64];
		sprintf(zPoolIdx, "Pool#%d", lPoolIdx);

		Config::BeginSection(zPoolIdx);

		Config::AddString("AddServer", "Add a server", "Add a server for a protocol. Format : listen_port:protocol_name(:SSL)",
			Config::EVariableFlag::NeedRestart,
			[](const char* pValue, void* pContext)->bool {
				if (!pValue)
				{
					return false;
				}
				LPoolIdx lPoolIdx = static_cast<LPoolIdx>(reinterpret_cast<uintptr_t>(pContext));
				if (lPoolIdx < 0 || lPoolIdx >= MaxConnectionPool)
				{
					LogErr("WioT", "Internal error : PoolIdx is out of range in configuration");
					return false;
				}

				CRegexpA regex("^(\\d+):(\\w+)(:SSL|:SSLonly)?$");
				MatchResult result;
				result = regex.Match(pValue);
				if (!result.IsMatched())
				{
					LogErr("WioT", "Invalid argument given to AddServer.\n"
						"Use format \"listen_port:protocol_name\".\n"
						"Example : \"5000:Teltonika\"");
					return false;
				}
				std::string listenPort(pValue + result.GetGroupStart(1), result.GetGroupEnd(1) - result.GetGroupStart(1));
				std::string protocolName(pValue + result.GetGroupStart(2), result.GetGroupEnd(2) - result.GetGroupStart(2));
				ESecurity eSecurity;
				if (result.GetGroupStart(3) > 0)
				{
					const std::string ssl(pValue + result.GetGroupStart(3), result.GetGroupEnd(3) - result.GetGroupStart(3));
					eSecurity = (ssl == ":SSLonly") ? ESecurity::UseSSLOnly : ESecurity::AllowSSL;
				}
				else
				{
					eSecurity = ESecurity::NoSSL;
				}
				IConnectionPool* pServer = nullptr;
				if (!ConnectionManager.GetServer(lPoolIdx, pServer) || !pServer)
				{
					LogErr("WioT", "Internal error : Server Pool #%d was not found.", lPoolIdx);
					return false;
				}

				const int nListenPort = atoi(listenPort.c_str());
				if (!pServer->AddServer(nListenPort, protocolName.c_str(), eSecurity))
				{
					LogErr("WioT", "Failed to add new server \"%d:%s\" to connection pool #%d.",
						nListenPort, protocolName.c_str(), lPoolIdx);
					return false;
				}

				return true;
			},
			[](char* pValue, const int nBufferLength, void* pContext)->bool {
				LogErr("WioT", "AddServer is write only");
				return false;
			},
			reinterpret_cast<void*>(static_cast<uintptr_t>(lPoolIdx)));
		Config::EndSection();
	}
	Config::EndSection();

	Config::BeginSection("Teltonika");
	Config::BeginSection("CameraRequest");
	Config::AddString("ServerAddressIP", "Server Address IP", "Address of the server to send photo or video to",
		Config::EVariableFlag::NeedRestart,
		[](const char* pValue, void* pContext)->bool {
			if (!pValue)
			{
				return false;
			}
			CProtocolTeltonika::SetCameraRequestServerAddressIP(pValue);
			return true;
		},
		[](char* pValue, const int nBufferLength, void* pContext)->bool {
			if (!pValue)
			{
				return false;
			}
			const std::string& zAddress = CProtocolTeltonika::GetCameraRequestServerAddressIP();
			if (static_cast<int>(zAddress.length()) >= nBufferLength - 1)
			{
				LogErr("WioT", "Cannot get CameraRequest.AddressIP : buffer is too small");
				return false;
			}
			strcpy(pValue, zAddress.c_str());
			return true;
		});
	Config::AddString("BasePath", "Base Path", "Path to the directory where file will be moved",
		Config::EVariableFlag::NeedRestart,
		[](const char* pValue, void* pContext)->bool {
			if (!pValue)
			{
				return false;
			}
			CProtocolDualCam::SetCameraBasePath(pValue);
			return true;
		},
		[](char* pValue, const int nBufferLength, void* pContext)->bool {
			if (!pValue)
			{
				return false;
			}
			const std::string& zPath = CProtocolDualCam::GetCameraBasePath();
			if (static_cast<int>(zPath.length()) >= nBufferLength - 1)
			{
				LogErr("WioT", "Cannot get CameraRequest.BasePath : buffer is too small");
				return false;
			}
			strcpy(pValue, zPath.c_str());
			return true;
		});
	Config::AddString("Url", "Base Path", "Path to the directory where file will be moved",
		Config::EVariableFlag::NeedRestart,
		[](const char* pValue, void* pContext)->bool {
			if (!pValue)
			{
				return false;
			}
			CProtocolDualCam::SetCameraUrl(pValue);
			return true;
		},
		[](char* pValue, const int nBufferLength, void* pContext)->bool {
			if (!pValue)
			{
				return false;
			}
			const std::string& zPath = CProtocolDualCam::GetCameraUrl();
			if (static_cast<int>(zPath.length()) >= nBufferLength - 1)
			{
				LogErr("WioT", "Cannot get CameraRequest.Url : buffer is too small");
				return false;
			}
			strcpy(pValue, zPath.c_str());
			return true;
		});
	Config::AddInteger("ServerPort", "Server Port", "Port of the server to send photo or video to",
		Config::EVariableFlag::NeedRestart,
		[](const int *pValue, void* pContext)->bool {
			if (!pValue)
			{
				return false;
			}
			if (*pValue <= 0 || *pValue >= 65536)
			{
				LogErr("WioT", "CameraRequest.ServerPort must be between 1 and 65535");
				return false;
			}
			CProtocolTeltonika::SetCameraRequestServerPort(*pValue);
			return true;
		},
		[](int *pValue, const int nBufferLength, void* pContext)->bool {
			if (!pValue || nBufferLength < static_cast<int>(sizeof(*pValue)))
			{
				return false;
			}
			*pValue = CProtocolTeltonika::GetCameraRequestServerPort();
			return true;
		});
	Config::EndSection();
	Config::EndSection();

}

#include <unistd.h>

int main(int argc, char *argv[])
{
#ifndef _WIN32
	signal(SIGINT, SigHandlerQuit);   // install our handler
	signal(SIGTERM, SigHandlerQuit);   // install our handler
	signal(SIGKILL, SigHandlerQuit);   // install our handler
	signal(SIGSEGV, SigHandlerDump);   // install our handler
	signal(SIGILL, SigHandlerDump);   // install our handler
	signal(SIGPIPE, SIG_IGN);   // install our handler
	signal(SIGFPE, SigHandlerDump);   // install our handler
	signal(SIGUSR1, SigHandlerUser);   // install our handler
	signal(SIGUSR2, SigHandlerUser);   // install our handler
#endif

	const EOptionRet optRet = ReadArguments(argc, argv);
	switch (optRet)
	{
	case EOptionRet::Continue:
		break;
	case EOptionRet::Exit:
		return EXIT_SUCCESS;
	case EOptionRet::Error:
	default:
		return EXIT_FAILURE;
	}

	if (!Daemonize() ||
		!InitLog())
	{
		return EXIT_FAILURE;
	}

	FillConfig();
	
	// on lit la configuration
	if (g_zConfigFilename[0])
	{
		Config::SetConfigFilename(g_zConfigFilename);
		Config::ImportConfigFromFile();
	}

	rlimit flimit;

	if (!getrlimit(RLIMIT_NOFILE, &flimit))
	{
		LogDebug("WioT", "Current file count limit is soft:%d, hard:%d", flimit.rlim_cur, flimit.rlim_max);
		if (flimit.rlim_cur < MaxFileCount)
		{
			const auto saved_limit = flimit.rlim_cur;
			flimit.rlim_cur = MaxFileCount;
			if (!setrlimit(RLIMIT_NOFILE, &flimit))
			{
				LogInf("WioT", "New file count limit is %d", MaxFileCount);
			}
			else
			{
				const int err = errno;
				LogErr("WioT", "Cannot set file count limit to value %d. Current file count limit is %ul.\n"
					"Error %d : %s",
					MaxFileCount,saved_limit,
					err, strerror(err));
			}
		}
	}
	else
	{
		const int err = errno;
		LogErr("WioT", "Cannot get file count limit.\n"
			"Error %d : %s", err, strerror(err));
	}

	if (!getrlimit(RLIMIT_NOFILE, &flimit))
	{
		LogDebug("WioT", "Check : file count limit is %d", flimit.rlim_cur);
	}

	LogInf("WioT", "Entering WioT Mobility server");

	// configuration
	DataManager.SetServers(gs_zDatabaseServers);
	DataManager.SetTopicVehicle(gs_zDatabaseTopicVehicle);
	DataManager.SetClientId(gs_zDatabaseClientId);

	SecurityManager.Start(gs_zCertificateFile, gs_zPrivateKeyFile, nullptr);

	DataManager.Start();

	CommandManager.Start();

    if (ConnectionManager.Start())
	{
		boost::mutex::scoped_lock lock(gs_Mutex);
		gs_SignalEvent.wait(lock);
	} 

    ConnectionManager.Stop();

	CommandManager.Stop();

	DeviceManager.CloseAll();
	DataManager.Stop();
	SecurityManager.Stop();

	LogInf("WioT", "Quiting WioT Mobility server");

    return EXIT_SUCCESS;
}