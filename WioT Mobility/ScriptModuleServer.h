#pragma once
#include "IScriptModule.h"

class CScriptModuleServer :
    public IScriptModule
{
    CScriptModuleServer();
    ~CScriptModuleServer();
    CScriptModuleServer(const CScriptModuleServer&) = delete;
    CScriptModuleServer&operator=(const CScriptModuleServer&) = delete;

public:
    static CScriptModuleServer &GetInstance();

    // Hérité via IScriptModule
    const char* GetName() override;
    const TFunction* GetFunctions(int& nFuncCount) override;
    const TProperty* GetProperties(int& nPropCount) override;
    void PrivateInitialization(duk_context* ctx) override;
    void PrivateFinalization(duk_context* ctx) override;

private:
    static duk_ret_t GetStatistics(duk_context* ctx);
    static duk_ret_t GetInfos(duk_context* ctx);
};

