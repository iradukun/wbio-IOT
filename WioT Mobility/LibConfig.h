#pragma once

namespace Sagitech {
namespace Config {

enum class EVariableType
{
	Unknow=-1,
	Command,
	Bool,
	Integer,
	Unsigned,
	Float,
	Double,
	String,
	Enumeration,
};

enum EVariableFlag
{
	None,
	ReadOnly = 0x01,
	NeedRestart = 0x02,
};

struct TEnumeration
{
	const char* zTitle;
	int nValue;
};

bool NewDescription();
bool BeginSection(const char* zSectionName);
bool EndSection();

bool AddBool(const char* zName, const char* zShortDescription, const char* zLongDescription, unsigned uFlags,
	bool (*setter)(const bool *pValue, void *pContext), bool (*getter)(bool* pValue, const int nBufferLength, void* pContext), void* pContext = nullptr);
bool AddInteger(const char* zName, const char* zShortDescription, const char* zLongDescription, unsigned uFlags,
	bool (*setter)(const int *pValue, void *pContext), bool (*getter)(int* pValue, const int nBufferLength, void* pContext), void* pContext = nullptr);
bool AddUnsigned(const char* zName, const char* zShortDescription, const char* zLongDescription, unsigned uFlags,
	bool (*setter)(const unsigned* pValue, void* pContext), bool (*getter)(unsigned* pValue, const int nBufferLength, void* pContext), void* pContext = nullptr);
bool AddFloat(const char* zName, const char* zShortDescription, const char* zLongDescription, unsigned uFlags,
	bool (*setter)(const float* pValue, void* pContext), bool (*getter)(float* pValue, const int nBufferLength, void* pContext), void* pContext = nullptr);
bool AddDouble(const char* zName, const char* zShortDescription, const char* zLongDescription, unsigned uFlags,
	bool (*setter)(const double* pValue, void* pContext), bool (*getter)(double* pValue, const int nBufferLength, void* pContext), void* pContext = nullptr);
bool AddString(const char* zName, const char* zShortDescription, const char* zLongDescription, unsigned uFlags,
	bool (*setter)(const char* pValue, void* pContext), bool (*getter)(char* pValue, const int nBufferLength, void* pContext), void* pContext = nullptr);
bool AddEnumeration(const char* zName, const TEnumeration *pEnumeration, const char* zShortDescription, const char* zLongDescription, unsigned uFlags,
	bool (*setter)(const int* pValue, void* pContext), bool (*getter)(int* pValue, const int nBufferLength, void* pContext), void* pContext = nullptr);
bool AddCommand(const char* zName, const char* zShortDescription, const char* zLongDescription, unsigned uFlags,
	bool (*execute)(void* pContext), void* pContext = nullptr);

bool SetVariable(const char* zName, const char *zValue);
int GetVariable(const char* zName, char *zValue, int nMaxLength);
bool Execute(const char* zName);

int Describe(char* zDescription, int nMaxLength);

bool SetConfigFilename(const char* zFilename);
bool ImportConfigFromFile();
bool ExportConfigToFile();

} // namespace Config
} // namespace Sagitech

#include "ConfigManager.h"
