#include "WioTMobility.h"
#include "ScriptModuleCore.h"
#include "Externals.h"
#include "DeviceManager.h"
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

CScriptModuleCore::CScriptModuleCore()
{
}

CScriptModuleCore::~CScriptModuleCore()
{
}

CScriptModuleCore& CScriptModuleCore::GetInstance()
{
	static CScriptModuleCore s_Instance;
	return s_Instance;
}

const char* CScriptModuleCore::GetName()
{
	return nullptr;
}

const TFunction* CScriptModuleCore::GetFunctions(int& nFuncCount)
{
	static const TFunction s_Functions[] =
	{
		{"ReturnError", CScriptModuleCore::ReturnError, 1},
		{"ReturnSuccess", CScriptModuleCore::ReturnSuccess, 0},
		{"ReturnJSON", CScriptModuleCore::ReturnJSON, 1},
	};
	nFuncCount = CountOf(s_Functions);
	return s_Functions;
}

const TProperty* CScriptModuleCore::GetProperties(int& nPropCount)
{
	nPropCount = 0;
	return nullptr;
}

void CScriptModuleCore::PrivateInitialization(duk_context* ctx)
{
}

void CScriptModuleCore::PrivateFinalization(duk_context* ctx)
{
}

duk_ret_t CScriptModuleCore::ReturnSuccess(duk_context* ctx)
{
	CCommandPrompt* pPrompt = GetPrompt(ctx);

	pPrompt->Print("{\"ReturnStatus\":\"Success\"}");

	return 0; // no return value
}

duk_ret_t CScriptModuleCore::ReturnError(duk_context* ctx)
{
	CCommandPrompt* pPrompt = GetPrompt(ctx);

    // dummy loop
    do
    {
        if (duk_get_top(ctx) < 1)
        {
            LogErr("WioT", "[CMDPROMPT] ReturnError accepts only 1 argument.");
            break;
        }

        if (!duk_is_string(ctx, 0))
        {
            LogErr("WioT", "[CMDPROMPT] The argument of ReturnError must be a string (the error description).");
            break;
        }

        const char* zError = duk_get_string(ctx, 0);

        pPrompt->PrintError(zError);

        duk_remove(ctx, 0);

    } while (false);

	return 0; // no return value
}

duk_ret_t CScriptModuleCore::ReturnJSON(duk_context* ctx)
{
	CCommandPrompt* pPrompt = GetPrompt(ctx);

    // dummy loop
    do
    {
        if (duk_get_top(ctx) < 1)
        {
            LogErr("WioT", "[CMDPROMPT] ReturnJSON accepts only 1 argument.");
            break;
        }

        pPrompt->Print(duk_json_encode(ctx, 0));

        duk_pop(ctx);

    } while (false);

	return 0; // no return value
}
