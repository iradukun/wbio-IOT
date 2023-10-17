#pragma once
#include "duktape.h"

struct TFunction
{
	const char* zFunctionName;
	duk_c_function Proc;
	int nArgCount;
};

struct TProperty
{
	const char* zPropertyName;
	// TODO : rajouter les infos
};

class CCommandPrompt;
class IScriptModule
{
public:
	virtual const char * GetName() = 0;
	virtual const TFunction* GetFunctions(int& nFuncCount) = 0;
	virtual const TProperty* GetProperties(int& nPropCount) = 0;
	virtual void PrivateInitialization(duk_context* ctx) = 0;
	virtual void PrivateFinalization(duk_context* ctx) = 0;

	static CCommandPrompt* GetPrompt(duk_context* ctx);
};

