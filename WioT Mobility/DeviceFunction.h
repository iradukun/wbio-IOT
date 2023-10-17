#pragma once
#include "duktape.h"
#include <string>

class CDevice;

struct TDeviceFunction
{
	const char* zFuncName;
	int nNbArgument;
	duk_ret_t (*fnCall)(const char* zFuncName, CDevice *pDevice, std::string& ret, duk_context* ctx);
};


template<class T>
T GetArg(duk_context* ctx, int nArg);

#ifndef GET_ARG_INSTANCIATION
extern template int GetArg(duk_context* ctx, int nArg);
extern template std::string GetArg(duk_context* ctx, int nArg);
extern template float GetArg(duk_context* ctx, int nArg);
extern template bool GetArg(duk_context* ctx, int nArg);
#endif

#define CALLABLE_0(ProtocolType, ProtocolFunc)		\
[](const char* zFuncName, CDevice* pDevice, std::string& ret, duk_context* ctx)->duk_ret_t \
{	\
	if (!pDevice)	\
	{	\
		ret = "Device is null"; \
		return -1;	\
	}	\
	const ProtocolType* pProtocol = static_cast<const ProtocolType*>(pDevice->GetProtocol());	\
	\
	return pProtocol->ProtocolFunc(zFuncName, pDevice, ret);	\
}

#define CALLABLE_1(ProtocolType, ProtocolFunc, T0)		\
[](const char* zFuncName, CDevice* pDevice, std::string& ret, duk_context* ctx)->duk_ret_t \
{	\
	if (!pDevice)	\
	{	\
		ret = "Device is null"; \
		return -1;	\
	}	\
	const ProtocolType* pProtocol = static_cast<const ProtocolType*>(pDevice->GetProtocol());	\
	\
	return pProtocol->ProtocolFunc(zFuncName, pDevice, ret, GetArg<T0>(ctx, 0));	\
}

#define CALLABLE_2(ProtocolType, ProtocolFunc, T0, T1)		\
[](const char* zFuncName, CDevice* pDevice, std::string& ret, duk_context* ctx)->duk_ret_t \
{	\
	if (!pDevice)	\
	{	\
		ret = "Device is null"; \
		return -1;	\
	}	\
	const ProtocolType* pProtocol = static_cast<const ProtocolType*>(pDevice->GetProtocol());	\
	\
	return pProtocol->ProtocolFunc(pDevice, GetArg<T0>(ctx, 0), GetArg<T1>(ctx, 1));	\
}

#define CALLABLE_3(ProtocolType, ProtocolFunc, T0, T1, T2)		\
[](const char* zFuncName, CDevice* pDevice, std::string& ret, duk_context* ctx)->duk_ret_t \
{	\
	if (!pDevice)	\
	{	\
		ret = "Device is null"; \
		return -1;	\
	}	\
	const ProtocolType* pProtocol = static_cast<const ProtocolType*>(pDevice->GetProtocol());	\
	\
	return pProtocol->ProtocolFunc(zFuncName, pDevice, ret, GetArg<T0>(ctx, 0), GetArg<T1>(ctx, 1), GetArg<T2>(ctx, 2));	\
}

#define CALLABLE_4(ProtocolType, ProtocolFunc, T0, T1, T2, T3)		\
[](const char* zFuncName, CDevice* pDevice, std::string& ret, duk_context* ctx)->duk_ret_t \
{	\
	if (!pDevice)	\
	{	\
		ret = "Device is null"; \
		return -1;	\
	}	\
	const ProtocolType* pProtocol = static_cast<const ProtocolType*>(pDevice->GetProtocol());	\
	\
	return pProtocol->ProtocolFunc(zFuncName, pDevice, ret, GetArg<T0>(ctx, 0), GetArg<T1>(ctx, 1), GetArg<T2>(ctx, 2), GetArg<T3>(ctx, 3));	\
}



