#pragma once
#include "IScriptModule.h"

class CScriptModuleDevice :
    public IScriptModule
{
    CScriptModuleDevice();
    ~CScriptModuleDevice();
    CScriptModuleDevice(const CScriptModuleDevice&) = delete;
    CScriptModuleDevice&operator=(const CScriptModuleDevice&) = delete;

public:
    static CScriptModuleDevice &GetInstance();

    // Hérité via IScriptModule
    const char* GetName() override;
    const TFunction* GetFunctions(int& nFuncCount) override;
    const TProperty* GetProperties(int& nPropCount) override;
    void PrivateInitialization(duk_context* ctx) override;
    void PrivateFinalization(duk_context* ctx) override;

private:
    static duk_ret_t GetDevice(duk_context* ctx);
    static duk_ret_t CallDeviceFunc(duk_context* ctx);
    static duk_ret_t GetDeviceList(duk_context* ctx);
};

