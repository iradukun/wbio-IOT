#pragma once
#include "IScriptModule.h"

class CScriptModuleCore :
    public IScriptModule
{
    CScriptModuleCore();
    ~CScriptModuleCore();
    CScriptModuleCore(const CScriptModuleCore&) = delete;
    CScriptModuleCore&operator=(const CScriptModuleCore&) = delete;

public:
    static CScriptModuleCore &GetInstance();

    // Hérité via IScriptModule
    const char* GetName() override;
    const TFunction* GetFunctions(int& nFuncCount) override;
    const TProperty* GetProperties(int& nPropCount) override;
    void PrivateInitialization(duk_context* ctx) override;
    void PrivateFinalization(duk_context* ctx) override;

private:
    static duk_ret_t ReturnError(duk_context* ctx);
    static duk_ret_t ReturnSuccess(duk_context* ctx);
    static duk_ret_t ReturnJSON(duk_context* ctx);
};

