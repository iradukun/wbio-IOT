#include "WioTMobility.h"
#include "ScriptModuleDevice.h"
#include "Externals.h"
#include "DeviceManager.h"
#include "CommandPrompt.h"
#include "Device.h"
#include "IProtocol.h"
#include "DeviceFunction.h"

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

constexpr const char* c_DeviceId = DUK_HIDDEN_SYMBOL("DeviceId");

CScriptModuleDevice::CScriptModuleDevice()
{
}

CScriptModuleDevice::~CScriptModuleDevice()
{
}

CScriptModuleDevice& CScriptModuleDevice::GetInstance()
{
	static CScriptModuleDevice s_Instance;
	return s_Instance;
}

const char* CScriptModuleDevice::GetName()
{
    return nullptr;
}

const TFunction* CScriptModuleDevice::GetFunctions(int& nFuncCount)
{
	static const TFunction s_Functions[] =
	{
		{"GetDevice", CScriptModuleDevice::GetDevice, 1},
		{"GetDeviceList", CScriptModuleDevice::GetDeviceList, 0},
	};
	nFuncCount = CountOf(s_Functions);
	return s_Functions;
}

const TProperty* CScriptModuleDevice::GetProperties(int& nPropCount)
{
	nPropCount = 0;
    return nullptr;
}

void CScriptModuleDevice::PrivateInitialization(duk_context* ctx)
{
}

void CScriptModuleDevice::PrivateFinalization(duk_context* ctx)
{
}

duk_ret_t CScriptModuleDevice::GetDevice(duk_context* ctx)
{
// CCommandPrompt* pPrompt = GetPrompt(ctx);

    // dummy loop
    do
    {
        if (duk_get_top(ctx) < 1)
        {
            LogErr("WioT", "[CMDPROMPT] GetParam accepts only 1 argument.");
            break;
        }

        if (!duk_is_string(ctx, 0))
        {
            LogErr("WioT", "[CMDPROMPT] The argument of GetDevice must be a string (the IMEI of the device).");
            break;
        }

        std::string zIMEI(duk_get_string(ctx, 0));

        CAutoPtr<CDevice> pDevice(DeviceManager.GetDeviceById(zIMEI.c_str()), false);

        duk_remove(ctx, 0);

        if (!pDevice)
        {
            LogErr("WioT", "[CMDPROMPT] Device with IMEI \"%s\" was not found.", zIMEI.c_str());
            break;
        }

        duk_push_object(ctx);

        duk_push_string(ctx, zIMEI.c_str());
        duk_put_prop_string(ctx, -2, c_DeviceId);

        duk_push_string(ctx, pDevice->GetDeviceId().c_str());
        duk_put_prop_string(ctx, -2, "DeviceId");

        if (!pDevice->GetDeviceType().empty())
        {
            duk_push_string(ctx, pDevice->GetDeviceType().c_str());
            duk_put_prop_string(ctx, -2, "DeviceType");
        }

        int nNbFunctions = 0;
        const TDeviceFunction* pFunctions = pDevice->GetProtocol()->GetDeviceFunctions(nNbFunctions);

        for (int i = 0; i < nNbFunctions; ++i)
        {
            duk_push_c_function(ctx, CallDeviceFunc, pFunctions[i].nNbArgument);
            duk_push_string(ctx, "name");
            duk_push_string(ctx, pFunctions[i].zFuncName);
            duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);
            duk_push_string(ctx, "idx");
            duk_push_int(ctx, i);
            duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);
            duk_put_prop_string(ctx, -2, pFunctions[i].zFuncName);
        }

        return 1;
    } while (false);

    duk_push_undefined(ctx);
    return 1;
}

duk_ret_t CScriptModuleDevice::CallDeviceFunc(duk_context* ctx)
{
    CCommandPrompt* pPrompt = GetPrompt(ctx);

    // 1 : geting the device
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, c_DeviceId);
    CAutoPtr<CDevice> pDevice(DeviceManager.GetDeviceById(duk_get_string(ctx, -1)), false);
    duk_pop_2(ctx);

    if (pDevice.Empty())
    {
        LogNotice("WioT", "[PROMPT] Device not found");
        pPrompt->PrintError("Internal error", "Device not found");
        return duk_ret_t(0);
    }

    duk_push_current_function(ctx);
    duk_get_prop_string(ctx, -1, "idx");   
    const int nFuncIdx = duk_get_int(ctx, -1);
    duk_pop_2(ctx);

    int nNbFunctions = 0;
    const TDeviceFunction* pFunctions = pDevice->GetProtocol()->GetDeviceFunctions(nNbFunctions);

    if (nFuncIdx < 0 || nFuncIdx >= nNbFunctions)
    {
        LogNotice("WioT", "[PROMPT] Invalid function idx %d (0<=idx<%d)", nFuncIdx, nNbFunctions);
        pPrompt->PrintError("Internal error", "Invalid function idx");
        return duk_ret_t(0);
    }

    std::string ret;
    const duk_ret_t nRet = pFunctions[nFuncIdx].fnCall(pFunctions[nFuncIdx].zFuncName, pDevice, ret, ctx);

    if (nRet < 0)
    {
        pPrompt->PrintError("Error", ret.empty() ? nullptr : ret.c_str());
    }
    else if (!ret.empty())
    {
        pPrompt->Print(ret);
    }
    return nRet;
}

duk_ret_t CScriptModuleDevice::GetDeviceList(duk_context* ctx)
{
    CCommandPrompt* pPrompt = GetPrompt(ctx);

    auto builder = bsoncxx::builder::stream::document{};
    auto array_builder = builder << "DeviceList" << open_array;

    class CDeviceSink : public IDeviceSink
    {
    public:
        CDeviceSink(bsoncxx::v_noabi::builder::stream::array_context<bsoncxx::v_noabi::builder::stream::key_context<> >& array_builder)
           : m_ArrayBuilder(array_builder)
        {
        }

        bool OnDevice(CDevice* pDevice) override
        {
            m_ArrayBuilder << open_document
                << "ID" << pDevice->GetDeviceId()
                << "Protocol" << pDevice->GetProtocol()->GetName()
                << "Type" << pDevice->GetDeviceType()
                << "PoolIdx" << pDevice->GetPoolIdx()
                << "Address" << pDevice->GetAddressIP()
                << "ConnectionTime" << pDevice->GetConnectionTime()
                << "LastEmissionTime" << pDevice->GetLastEmissionTime()
                << "LastReceptionTime" << pDevice->GetLastReceptionTime()
                << close_document;
            return true;
        }

    private:
        bsoncxx::v_noabi::builder::stream::array_context<bsoncxx::v_noabi::builder::stream::key_context<> >& m_ArrayBuilder;
    } sink(array_builder);

    DeviceManager.EnumerateDevice(&sink);

    bsoncxx::document::value deviceList = array_builder << close_array << finalize;

    pPrompt->Print(bsoncxx::to_json(deviceList));

    return 0; // no return value
}
