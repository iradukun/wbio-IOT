#include "WioTMobility.h"
#include "ScriptModuleServer.h"
#include "Externals.h"
#include "DeviceManager.h"
#include "ProtocolManager.h"
#include "ConnectionManager.h"
#include "CommandPrompt.h"

#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

CScriptModuleServer::CScriptModuleServer()
{
}

CScriptModuleServer::~CScriptModuleServer()
{
}

CScriptModuleServer& CScriptModuleServer::GetInstance()
{
	static CScriptModuleServer s_Instance;
	return s_Instance;
}

const char* CScriptModuleServer::GetName()
{
    return "Server";
}

const TFunction* CScriptModuleServer::GetFunctions(int& nFuncCount)
{
	static const TFunction s_Functions[] =
	{
		{"GetInfos", CScriptModuleServer::GetInfos, 0},
		{"GetStatistics", CScriptModuleServer::GetStatistics, 0},
	};
	nFuncCount = CountOf(s_Functions);
	return s_Functions;
}

const TProperty* CScriptModuleServer::GetProperties(int& nPropCount)
{
	nPropCount = 0;
    return nullptr;
}

void CScriptModuleServer::PrivateInitialization(duk_context* ctx)
{
}

void CScriptModuleServer::PrivateFinalization(duk_context* ctx)
{
}

duk_ret_t CScriptModuleServer::GetStatistics(duk_context* ctx)
{
	CCommandPrompt* pPrompt = GetPrompt(ctx);

	auto builder = bsoncxx::builder::stream::document{};
	bsoncxx::document::value stats = builder
		<< "DeviceCount" << DeviceManager.CurrentDeviceCount()
		<< "DevicePoolCount" << DeviceManager.GetActivePoolCount()
		<< "ProtocolCount" << ProtocolManager.GetProtocolCount()
		<< "ConnectionPoolCount" << ConnectionManager.GetActivePoolCount()
		<< "ConnectionInProgress" << ConnectionManager.GetConnectionInProgress()
		<< finalize;

	pPrompt->Print(bsoncxx::to_json(stats));

	return 0; // no return value
}

duk_ret_t CScriptModuleServer::GetInfos(duk_context* ctx)
{
	CCommandPrompt* pPrompt = GetPrompt(ctx);

    auto builder = bsoncxx::builder::stream::document{};
    bsoncxx::document::value infos = builder
        << "Version" << Sagitech::g_zProgramVersion
        << finalize;

	pPrompt->Print(bsoncxx::to_json(infos));

	return 0; // no return value
}
