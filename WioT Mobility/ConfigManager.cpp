#include "WioTMobility.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#include "deelx.h"
#pragma GCC diagnostic pop
#include "LibLog.h"
#include "LibConfig.h"
#include "ConfigManager.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>

namespace Sagitech {
namespace Config {


constexpr int AllocPacketSize = 64;

struct TSection
{
	TSection(const char*_name, TSection *_parent)
		: zName(_name)
		, pParent(_parent)
		, pNext(nullptr)
		, pChildVariables(nullptr)
		, pChildSections(nullptr)
	{}

	~TSection()
	{
		TVariable* pVar = pChildVariables;
		while (pVar)
		{
			TVariable* pNext = pVar->pNext;
			delete pVar;
			pVar = pNext;
		}

		TSection* pSec = pChildSections;
		while (pSec)
		{
			TSection* pNext = pSec->pNext;
			delete pSec;
			pSec = pNext;
		}
	}

	std::string zName;
	TSection* pParent;
	TSection* pNext;
	TVariable* pChildVariables;
	TSection* pChildSections;
};

CConfigManager::CConfigManager()
	: m_pRoot(nullptr)
	, m_pCurrentParent(nullptr)
	, m_ppVariables(nullptr)
	, m_nNbVariables(0)
{
}

CConfigManager::~CConfigManager()
{
	delete m_pRoot;
	if (m_ppVariables)
	{
		free(m_ppVariables);
	}
}

CConfigManager& CConfigManager::GetInstance()
{
	static CConfigManager s_Instance;
	return s_Instance;
}

bool CConfigManager::NewDescription()
{
	m_pCurrentParent = nullptr;
	return true;
}

bool CConfigManager::BeginSection(const char* zName)
{
	if (!zName)
	{
		return false;
	}

	TSection* pSection = new TSection(zName, m_pCurrentParent);
	if (!m_pRoot)
	{
		m_pRoot = pSection;
	}
	else
	{

		TSection* pTmp = m_pCurrentParent ? m_pCurrentParent->pChildSections : m_pRoot;

		if (!pTmp)
		{
			if (m_pCurrentParent)
			{
				m_pCurrentParent->pChildSections = pSection;
			}
			else
			{
				m_pRoot = pSection;
			}
		}
		else
		{
			while (pTmp->pNext)
			{
				pTmp = pTmp->pNext;
			}
			pTmp->pNext = pSection;
		}
	}
	m_pCurrentParent = pSection;
	return true;
}

bool CConfigManager::EndSection()
{
	if (!m_pCurrentParent)
	{
		return false;
	}
	m_pCurrentParent = m_pCurrentParent->pParent;
	return true;
}

bool CConfigManager::AddVariable(const char* zName, 
	EVariableType eType, 
	const char* zShortDescription, 
	const char* zLongDescription,
	const void* pExtra,
	unsigned uFlags, 
	void* pContext, 
	Setter setter, 
	Getter getter)
{
	if (!m_pCurrentParent)
	{
		return false;
	}

	if (eType == EVariableType::Unknow ||
		eType == EVariableType::Command)
	{
		return false;
	}

	if (!zName)
	{
		Log(ELogLevel::Error, "ConfigManager", "Le nom de la variable n'a pas été spécifié.");
		return false;
	}

	if (GetVariable(zName, m_pCurrentParent))
	{
		Log(ELogLevel::Error, "ConfigManager", "La variable \"%s\" existe déjà.", zName);
		return false;
	}

	TVariable* pVariable = new TVariable;
	pVariable->zName = zName;
	pVariable->eType = eType;
	pVariable->zShortDescription = zShortDescription;
	pVariable->zLongDescription = zLongDescription;
	pVariable->pExtra = pExtra;
	pVariable->uFlags = uFlags;
	pVariable->pContext = pContext;
	pVariable->Set = setter;
	pVariable->Get = getter;

	return FinalizeAddVariable(pVariable);
}

bool CConfigManager::AddCommand(const char* zName, const char* zShortDescription, const char* zLongDescription, const void* pExtra, unsigned uFlags, void* pContext, Executer executer)
{
	if (!m_pCurrentParent)
	{
		return false;
	}

	if (!zName)
	{
		Log(ELogLevel::Error, "ConfigManager", "Le nom de la commande n'a pas été spécifié.");
		return false;
	}

	if (GetVariable(zName))
	{
		Log(ELogLevel::Error, "ConfigManager", "La variable \"%s\" existe déjà.");
		return false;
	}

	TVariable* pVariable = new TVariable;
	pVariable->zName = zName;
	pVariable->eType = EVariableType::Command;
	pVariable->zShortDescription = zShortDescription;
	pVariable->zLongDescription = zLongDescription;
	pVariable->pExtra = pExtra;
	pVariable->uFlags = uFlags;
	pVariable->pContext = pContext;
	pVariable->Exec = executer;

	return FinalizeAddVariable(pVariable);
}

bool CConfigManager::FinalizeAddVariable(TVariable* pVariable)
{
	if (m_nNbVariables % AllocPacketSize == 0)
	{
		TVariable** pTmp = reinterpret_cast<TVariable**>(realloc(m_ppVariables, (m_nNbVariables + AllocPacketSize) * sizeof(TVariable*)));
		if (!pTmp)
		{
			delete pVariable;
			Log(ELogLevel::Error, "ConfigManager", "Impossible d'allouer un espace pour stocker %d variables.", m_nNbVariables + AllocPacketSize);
			return false;
		}
		m_ppVariables = pTmp;
	}

	// on insert dans l'ordre alphabétique
	int pos = 0;
	while (pos < m_nNbVariables && m_ppVariables[pos]->zName.compare(pVariable->zName) < 0)
	{
		pos++;
	}
	if (pos < m_nNbVariables)
	{
		memmove(m_ppVariables + pos + 1, m_ppVariables + pos, (m_nNbVariables - pos) * sizeof(TVariable*));
	}
	// on a une variable de plus
	m_ppVariables[pos] = pVariable;
	m_nNbVariables++;

	if (!m_pCurrentParent->pChildVariables)
	{
		m_pCurrentParent->pChildVariables = pVariable;
	}
	else
	{
		TVariable* pTmp = m_pCurrentParent->pChildVariables;

		while (pTmp->pNext)
		{
			pTmp = pTmp->pNext;
		}
		pTmp->pNext = pVariable;
	}

	return true;
}

bool CConfigManager::CheckNameValid(const char* zName)
{
	if (!zName)
	{
		return false;
	}

	for (int i = 0; zName[i]; ++i)
	{
		const char car = zName[i];
		if ((car >= 'a' && car <= 'z')
			|| (car >= 'A' && car <= 'Z')
			|| (car >= '1' && car <= '9')
			|| car == '_')
		{
			continue;
		}
		return false;
	}

	return true;
}

const TVariable* CConfigManager::GetVariable(const char* zName, const TSection* pParent) const
{
	if (!zName)
	{
		return nullptr;
	}

	// si le nom de la variable ne contient pas de '.', on cherche directement la variable dans la liste (au risque de se tromper)
	const char* pDot = strchr(zName, '.');
	const bool bHasDot = pDot != nullptr;
	if (!pParent && !bHasDot)
	{
		// recherche par dicotomie
		int bottom = 0, top = m_nNbVariables - 1;
		while (bottom <= top)
		{
			const int pos = (top + bottom) / 2;
			const int cmp = m_ppVariables[pos]->zName.compare(zName);
			if (cmp == 0)
			{
				return m_ppVariables[pos];
			}
			if (bottom == top)
			{
				// pas trouvé
				break;
			}
			if (cmp < 0)
			{
				bottom = pos + 1;
			}
			else
			{
				top = pos - 1;
			}
		}
		// non trouvé
		return nullptr;
	}

	const TSection* pSection = pParent ? pParent : m_pRoot;

	if (bHasDot)
	{
		while (pSection)
		{

			const size_t nLen = pDot - zName;

			while (pSection)
			{
				if (pSection->zName.length()== nLen &&
					memcmp(pSection->zName.c_str(), zName, nLen) == 0)
				{
					break;
				}
				pSection = pSection->pNext;
			}
			if (!pSection)
			{
				return nullptr;
			}

			zName = pDot + 1;

			pDot = strchr(zName, '.');
			if (!pDot)
			{
				break;
			}

			pSection = pSection->pChildSections;
		}
	}
	if (!pSection)
	{
		return nullptr;
	}
	if (!zName[0])
	{
		return nullptr;
	}

	const TVariable* pVariable = pSection->pChildVariables;
	while (pVariable)
	{
		if (pVariable->zName.compare(zName) == 0)
		{
			return pVariable;
		}
		pVariable = pVariable->pNext;
	}

	return nullptr;
}

static const char* Type2String(EVariableType eType)
{
	switch (eType)
	{
	case EVariableType::Command:
		return "Command";
	case EVariableType::Bool:
		return "Bool";
	case EVariableType::Integer:
		return "Integer";
	case EVariableType::Unsigned:
		return "Unsigned";
	case EVariableType::Float:
		return "Float";
	case EVariableType::Double:
		return "Double";
	case EVariableType::String:
		return "String";
	case EVariableType::Enumeration:
		return "Enumeration";
	default:
		return "<Unknow>";
	}
}

static int DescribeVariable(const TVariable* pVariable, char* zDescription, int nMaxLength)
{
//	printf("In DescribeVariable(%p,\"%s\",%d)\n", (void*)pVariable, pVariable->zName, nMaxLength);
	int total_length = 0;

	int len = snprintf(zDescription + total_length, nMaxLength - total_length,
		"{\"name\":\"%s\",\"type\":\"%s\",\"shortDesc\":\"%s\",\"longDesc\":\"%s\",\"flags\":%d",
		pVariable->zName, Type2String(pVariable->eType), pVariable->zShortDescription, pVariable->zLongDescription, pVariable->uFlags);
	if (len < 0)
	{
		return -1;
	}
	total_length += len;
	if (pVariable->eType == EVariableType::Enumeration)
	{
		len = snprintf(zDescription + total_length, nMaxLength - total_length,",\"extra\":[");
		if (len < 0)
		{
			return -1;
		}
		total_length += len;
		const TEnumeration* pEnum = reinterpret_cast<const TEnumeration*>(pVariable->pExtra);
		if (pEnum && pEnum->zTitle)
		{
			for (;;)
			{
				len = snprintf(zDescription + total_length, nMaxLength - total_length,"{\"title\":\"%s\",\"value\":%d}",
					pEnum->zTitle,pEnum->nValue);
				if (len < 0)
				{
					return -1;
				}
				total_length += len;
				pEnum++;
				if (!pEnum->zTitle)
				{
					break;
				}
				if (total_length + 1 > nMaxLength)
				{
					return -1;
				}
				zDescription[total_length++] = ',';
			}
		}
		len = snprintf(zDescription + total_length, nMaxLength - total_length, "]");
		if (len < 0)
		{
			return -1;
		}
		total_length += len;
	}
	if (total_length + 1 > nMaxLength)
	{
		return -1;
	}
	zDescription[total_length++] = '}';
	return total_length;
}

static int DescribeVariableList(const TVariable* pVariable, char* zDescription, int nMaxLength)
{
//	printf("In DescribeVariableList(%p,%d)\n", (void*)pVariable, nMaxLength);
	if (nMaxLength < 2)
	{
		return -1;
	}

	int total_length = 0;

	zDescription[total_length++] = '[';

	if (pVariable)
	{
		for (;;)
		{
			const int len = DescribeVariable(pVariable, zDescription + total_length, nMaxLength - total_length - 1);
			pVariable = pVariable->pNext;
			if (len < 0)
			{
				return -1;
			}
			total_length += len;
			if (!pVariable)
			{
				break;
			}
			if (total_length > nMaxLength - 2)
			{
				return -1;
			}
			zDescription[total_length++] = ',';
		}
	}

	if (total_length > nMaxLength - 2)
	{
		return -1;
	}
	zDescription[total_length++] = ']';
	return total_length;
}

static int DescribeSectionList(const TSection* pSection, char* zDescription, int nMaxLength);

static int DescribeSection(const TSection* pSection, char* zDescription, int nMaxLength)
{
//	printf("In DescribeSection(%p,\"%s\",%d)\n", (void*)pSection, pSection->zName, nMaxLength);

	int total_length = 0;

	int len = snprintf(zDescription + total_length, nMaxLength - total_length,
		"{\"name\":\"%s\",\"variables\":",
		pSection->zName);
	if (len < 0)
	{
		return -1;
	}
	total_length += len;
	len = DescribeVariableList(pSection->pChildVariables, zDescription + total_length, nMaxLength - total_length);
	if (len < 0)
	{
		return -1;
	}
	total_length += len;
	len = snprintf(zDescription + total_length, nMaxLength - total_length,",\"sections\":");
	if (len < 0)
	{
		return -1;
	}
	total_length += len;
	len = DescribeSectionList(pSection->pChildSections, zDescription + total_length, nMaxLength - total_length);
	if (len < 0)
	{
		return -1;
	}
	total_length += len;
	if (total_length >nMaxLength-1)
	{
		return -1;
	}
	zDescription[total_length++]='}';
	return total_length;
}

static int DescribeSectionList(const TSection* pSection, char* zDescription, int nMaxLength)
{
//	printf("In DescribeSectionList(%p,%d)\n", (void*)pSection, nMaxLength);

	if (nMaxLength < 2)
	{
		return -1;
	}

	int total_length = 0;

	zDescription[total_length++] = '[';

	if (pSection)
	{
		for (;;)
		{
			const int len = DescribeSection(pSection, zDescription + total_length, nMaxLength - total_length - 1);
			pSection = pSection->pNext;
			if (len < 0)
			{
				return -1;
			}
			total_length += len;
			if (!pSection)
			{
				break;
			}
			if (total_length > nMaxLength - 2)
			{
				return -1;
			}
			zDescription[total_length++] = ',';
		}
	}
	if (total_length > nMaxLength - 2)
	{
		return -1;
	}

	zDescription[total_length++] = ']';
	return total_length;
}

int CConfigManager::Describe(char* zDescription, int nMaxLength)
{
	return DescribeSectionList(m_pRoot, zDescription, nMaxLength);
}

bool CConfigManager::ListAllVariables(IVariableSink* pSink) const
{
	if (!pSink)
	{
		return false;
	}

	for (int i = 0; i < m_nNbVariables; ++i)
	{
		char value[1024];
		if (m_ppVariables[i]->eType!=EVariableType::Command &&
			Config::GetVariable(m_ppVariables[i]->zName.c_str(), value, CountOf(value)))
		{
			if (!pSink->OnVariable(m_ppVariables[i]->zName.c_str(), value))
			{
				break;
			}
		}
	}
	return true;
}

bool NewDescription()
{
	return CConfigManager::GetInstance().NewDescription();
}

bool BeginSection(const char* zSectionName)
{
	return CConfigManager::GetInstance().BeginSection(zSectionName);
}

bool EndSection()
{
	return CConfigManager::GetInstance().EndSection();
}


bool AddBool(const char* zName, const char* zShortDescription, const char* zLongDescription, unsigned uFlags,
	bool (*setter)(const bool* pValue, void* pContext), bool (*getter)(bool* pValue, const int nBufferLength, void* pContext), void* pContext)
{
	return CConfigManager::GetInstance().AddVariable(zName, EVariableType::Bool, zShortDescription, zLongDescription, nullptr,
		uFlags, pContext, reinterpret_cast<Setter>(setter), reinterpret_cast<Getter>(getter));
}

bool AddInteger(const char* zName, const char* zShortDescription, const char* zLongDescription, unsigned uFlags,
	bool (*setter)(const int* pValue, void* pContext), bool (*getter)(int* pValue, const int nBufferLength, void* pContext), void* pContext)
{
	return CConfigManager::GetInstance().AddVariable(zName, EVariableType::Integer, zShortDescription, zLongDescription, nullptr,
		uFlags, pContext, reinterpret_cast<Setter>(setter), reinterpret_cast<Getter>(getter));
}

bool AddUnsigned(const char* zName, const char* zShortDescription, const char* zLongDescription, unsigned uFlags,
	bool (*setter)(const unsigned* pValue, void* pContext), bool (*getter)(unsigned* pValue, const int nBufferLength, void* pContext), void* pContext)
{
	return CConfigManager::GetInstance().AddVariable(zName, EVariableType::Unsigned, zShortDescription, zLongDescription, nullptr,
		uFlags, pContext, reinterpret_cast<Setter>(setter), reinterpret_cast<Getter>(getter));
}

bool AddFloat(const char* zName, const char* zShortDescription, const char* zLongDescription, unsigned uFlags,
	bool (*setter)(const float* pValue, void* pContext), bool (*getter)(float* pValue, const int nBufferLength, void* pContext), void* pContext)
{
	return CConfigManager::GetInstance().AddVariable(zName, EVariableType::Float, zShortDescription, zLongDescription, nullptr,
		uFlags, pContext, reinterpret_cast<Setter>(setter), reinterpret_cast<Getter>(getter));
}

bool AddDouble(const char* zName, const char* zShortDescription, const char* zLongDescription, unsigned uFlags,
	bool (*setter)(const double* pValue, void* pContext), bool (*getter)(double* pValue, const int nBufferLength, void* pContext), void* pContext)
{
	return CConfigManager::GetInstance().AddVariable(zName, EVariableType::Double, zShortDescription, zLongDescription, nullptr,
		uFlags, pContext, reinterpret_cast<Setter>(setter), reinterpret_cast<Getter>(getter));
}

bool AddString(const char* zName, const char* zShortDescription, const char* zLongDescription, unsigned uFlags,
	bool (*setter)(const char* pValue, void* pContext), bool (*getter)(char* pValue, const int nBufferLength, void* pContext), void* pContext)
{
	return CConfigManager::GetInstance().AddVariable(zName, EVariableType::String, zShortDescription, zLongDescription, nullptr,
		uFlags, pContext, reinterpret_cast<Setter>(setter), reinterpret_cast<Getter>(getter));
}

bool AddEnumeration(const char* zName, const TEnumeration* pEnumeration, const char* zShortDescription, const char* zLongDescription, unsigned uFlags,
	bool (*setter)(const int* pValue, void* pContext), bool (*getter)(int* pValue, const int nBufferLength, void* pContext), void* pContext)
{
	return CConfigManager::GetInstance().AddVariable(zName, EVariableType::Enumeration, zShortDescription, zLongDescription, pEnumeration,
		uFlags, pContext, reinterpret_cast<Setter>(setter), reinterpret_cast<Getter>(getter));
}

bool AddCommand(const char* zName, const char* zShortDescription, const char* zLongDescription, unsigned uFlags,
	bool (*execute)(void* pContext), void* pContext)
{
	return CConfigManager::GetInstance().AddCommand(zName, zShortDescription, zLongDescription, nullptr,
		uFlags, pContext, execute);
}


bool SetVariable(const char* zName, const char* zValue)
{
	const TVariable* pVariable = CConfigManager::GetInstance().GetVariable(zName);
	if (!pVariable)
	{
		Log(ELogLevel::Notice, "ConfigManager", "La variable \"%s\" n'a pas été trouvée.", zName);
		return false;
	}
	if (!pVariable->Set)
	{
		Log(ELogLevel::Error, "ConfigManager", "La variable \"%s\" ne peut pas être affectée.", zName);
		return false;
	}
	union
	{
		bool b;
		int i;
		unsigned u;
		float f;
		double d;
		char string[512];
	} value;
	const void* pValue = &value;
	int ret;
	switch (pVariable->eType)
	{
	case EVariableType::Command:
		return false;
	case EVariableType::Bool:
	{
		if (!strcmp(zValue, "true"))
		{
			value.b = true;
		}
		else if (!strcmp(zValue, "false"))
		{
			value.b = false;
		}
		else
		{
			return false;
		}
		ret = 1;
		break;
	}
	case EVariableType::Integer:
		ret = sscanf(zValue, "%d", &value.i);
		break;
	case EVariableType::Unsigned:
		ret = sscanf(zValue, "%u", &value.u);
		break;
	case EVariableType::Float:
		ret = sscanf(zValue, "%f", &value.f);
		break;
	case EVariableType::Double:
		ret = sscanf(zValue, "%lf", &value.d);
		break;
	case EVariableType::String:
	{
		size_t len = strlen(zValue);
		if (len > CountOf(value.string))
		{
			return -1;
		}
		if (len >= 2)
		{
			if (zValue[0] == '\"' && zValue[len - 1] == '\"')
			{
				zValue++;
				len -= 2;
			}
		}
		memcpy(value.string, zValue, len);
		value.string[len] = '\0';
		ret = 1;
		break;
	}
	case EVariableType::Enumeration:
	{
		size_t len = strlen(zValue + 1);
		if (len > CountOf(value.string))
		{
			return -1;
		}
		memcpy(value.string, zValue + 1, len - 1);
		value.string[len - 1] = '\0';

		const TEnumeration* pEnum = reinterpret_cast<const TEnumeration*>(pVariable->pExtra);
		if (!pEnum)
		{
			return false;
		}
		while (pEnum->zTitle)
		{
			if (!strcmp(pEnum->zTitle, value.string))
			{
				break;
			}
			pEnum++;
		}
		if (!pEnum->zTitle)
		{
			return false;
		}
		pValue = &pEnum->nValue;
		ret = 1;
		break;
	}
	default:
		return false;
	}
	if (ret != 1)
	{
		return false;
	}
	return pVariable->Set(pValue, pVariable->pContext);
}

int GetVariable(const char* zName, char* zValue, int nMaxLength)
{
	const TVariable* pVariable = CConfigManager::GetInstance().GetVariable(zName);
	if (!pVariable)
	{
		Log(ELogLevel::Notice, "ConfigManager", "La variable \"%s\" n'a pas été trouvée.", zName);
		return false;
	}
	if (!pVariable->Get)
	{
		Log(ELogLevel::Error, "ConfigManager", "La variable \"%s\" ne peut pas être lue.", zName);
		return -1;
	}

	if (!pVariable->Get)
	{
		return -1;
	}

	union
	{
		bool b;
		int i;
		unsigned u;
		float f;
		double d;
		char string[512];
	} value;

	int len = pVariable->Get(&value, sizeof(value), pVariable->pContext);
	if (len < 0)
	{
		return -1;
	}

	switch (pVariable->eType)
	{
	case EVariableType::Command:
		return false;
	case EVariableType::Bool:
		len = snprintf(zValue, nMaxLength, "%s", value.b ? "true" : "false");
		break;
	case EVariableType::Integer:
		len = snprintf(zValue, nMaxLength, "%d", value.i);
		break;
	case EVariableType::Unsigned:
		len = snprintf(zValue, nMaxLength, "%u", value.u);
		break;
	case EVariableType::Float:
		len = snprintf(zValue, nMaxLength, "%f", value.f);
		break;
	case EVariableType::Double:
		len = snprintf(zValue, nMaxLength, "%lf", value.d);
		break;
	case EVariableType::String:
		len = snprintf(zValue, nMaxLength, "\"%s\"", value.string);
		break;
	case EVariableType::Enumeration:
	{
//		printf("Searching value %d\n", value.i);
		len = -1;
		const TEnumeration* pEnum = reinterpret_cast<const TEnumeration*>(pVariable->pExtra);
		if (pEnum)
		{
			while (pEnum->zTitle)
			{
//				printf(" Enum \"%s\" =>%d\n", pEnum->zTitle, pEnum->nValue);
				if (pEnum->nValue == value.i)
				{
//					printf(" Found\n");
					len = snprintf(zValue, nMaxLength, "\"%s\"", pEnum->zTitle);
					break;
				}
				pEnum++;
			}
		}
		break;
	}
	default:
		return -1;
	}
	return len;
}

bool Execute(const char* zName)
{
	const TVariable* pVariable = CConfigManager::GetInstance().GetVariable(zName);
	if (!pVariable)
	{
		Log(ELogLevel::Notice, "ConfigManager", "La variable \"%s\" n'a pas été trouvée.", zName);
		return false;
	}
	if (pVariable->eType != EVariableType::Command)
	{
		Log(ELogLevel::Error, "ConfigManager", "La variable \"%s\" n'est pas une commande.", zName);
		return false;
	}
	if (!pVariable->Exec)
	{
		Log(ELogLevel::Error, "ConfigManager", "La variable \"%s\" ne peut pas être exécuté.", zName);
		return false;
	}
	return pVariable->Exec(pVariable->pContext);
}

int Describe(char* zDescription, int nMaxLength)
{
	return CConfigManager::GetInstance().Describe(zDescription, nMaxLength);
}


static char gs_ConfigFilename[FILENAME_MAX];

bool SetConfigFilename(const char* zFilename)
{
	if (!zFilename || zFilename[0]=='\0')
	{
		Log(ELogLevel::Notice, "ConfigManager", "Trying to set config file name with a null pointer or an empty string.");
		return false;
	}
	if (strlen(zFilename) > CountOf(gs_ConfigFilename) - 1)
	{
		Log(ELogLevel::Notice, "ConfigManager", "The file name specified to set the config file name is too long (max: %d char.).", FILENAME_MAX);
		return false;
	}

	strcpy(gs_ConfigFilename, zFilename);
	return true;
}

static int ReadNextLine(FILE* file, char* line, const int nMaxLen)
{
	if (!fgets(line, nMaxLen - 1, file))
	{
		return 0;
	}
	if (line[0] == '#' || line[0] == ';')
	{
		// commentaire
		return 0;
	}
	line[nMaxLen - 1] = '\0';
	int len = 0;
	while (line[len])
	{
		++len;
	}
	if (len==0)
	{
		return 0;
	}
	--len;
	while (len >= 0)
	{
		const char car = line[len];
		if (car == ' ' || car == '\t' || car == '\r' || car == '\n')
		{
			line[len] = '\0';
			if (len == 0)
			{
				return 0;
			}
			--len;
			continue;
		}
		break;
	}
	return len;
}

bool ImportConfigFromFile()
{
	Log(ELogLevel::Debug, "ConfigManager", "Opening config file \"%s\" for reading", gs_ConfigFilename);
	FILE* file = fopen(gs_ConfigFilename, "rt");
	if (!file)
	{
		Log(ELogLevel::Notice, "ConfigManager", "Failed to open config file \"%s\" for reading", gs_ConfigFilename);
		return false;
	}

	CRegexpA regex("^\\s*([\\w#.\\-]+)=(.*)$");
	char line[256] = "";
	int lineNumber = 0;
	while (!feof(file))
	{
		lineNumber++;
		if (!ReadNextLine(file, line, CountOf(line)))
		{
			continue;
		}
		Log(ELogLevel::Debug, "ConfigManager", "Line %d : \"%s\"", lineNumber, line);

		MatchResult result;
		result = regex.Match(line);
		if (!result.IsMatched())
		{
			Log(ELogLevel::Notice, "ConfigManager", "Error reading file \"%s\" at line %d : invalid format", gs_ConfigFilename, lineNumber);
			continue;
		}

		if (result.GetGroupStart(1) < 0 ||
			result.GetGroupStart(2) < 0)
		{
			Log(ELogLevel::Notice, "ConfigManager", "Error reading file \"%s\" at line %d : unexpected error", gs_ConfigFilename, lineNumber);
			continue;
		}
		const unsigned start = result.GetGroupStart(1);
		const unsigned len = result.GetGroupEnd(1) - start;

		char zName[64];
		if (len >= CountOf(zName))
		{
			Log(ELogLevel::Notice, "ConfigManager", "Error reading file \"%s\" at line %d : option name too long", gs_ConfigFilename, lineNumber);
			continue;
		}
		memcpy(zName, line + start, len * sizeof(char));
		zName[len] = '\0';

		Log(ELogLevel::Debug, "ConfigManager", "Value found from offset %d to %d => \"%s\""
			, result.GetGroupStart(2), result.GetGroupEnd(2), line + result.GetGroupStart(2));
		if (CConfigManager::GetInstance().GetVariable(zName))
		{
			SetVariable(zName, line + result.GetGroupStart(2));
		}
	}

	Log(ELogLevel::Debug, "ConfigManager", "Closing config file \"%s\"", gs_ConfigFilename);
	fclose(file);
	return true;
}

bool ExportConfigToFile()
{
	char zTmpFilename[FILENAME_MAX];
	strcpy(zTmpFilename, gs_ConfigFilename);
	strcat(zTmpFilename, ".tmp");

	FILE* fOut = fopen(zTmpFilename, "wt");
	if (!fOut)
	{
		Log(ELogLevel::Notice, "ConfigManager", "Failed to open config file \"%s\" for writing", zTmpFilename);
		return false;
	}
	FILE* fInt = fopen(gs_ConfigFilename, "rt");
	if (!fInt)
	{
		fclose(fInt);
		Log(ELogLevel::Notice, "ConfigManager", "Failed to open config file \"%s\" for reading", gs_ConfigFilename);
		return false;
	}

	CRegexpA regex("^\\s*([\\w-]+)=(.*)$");
	char line[256] = "";
	int lineNumber = 0;
	while (!feof(fInt))
	{
		lineNumber++;
		if (!fgets(line, CountOf(line) - 1, fInt))
		{
			break;
		}

		MatchResult result;
		result = regex.Match(line);
		if (!result.IsMatched())
		{
			fwrite(line,1,strlen(line),fOut);
			continue;
		}

		if (result.GetGroupStart(1) < 0 ||
			result.GetGroupStart(2) < 0)
		{
			Log(ELogLevel::Notice, "ConfigManager", "Error read file \"%s\" at line %d : unexpected error", gs_ConfigFilename, lineNumber);
			continue;
		}
		const unsigned start = result.GetGroupStart(1);
		const unsigned len = result.GetGroupEnd(1) - start;

		char zName[64];
		if (len >= CountOf(zName))
		{
			Log(ELogLevel::Notice, "ConfigManager", "Error read file \"%s\" at line %d : option name too long", gs_ConfigFilename, lineNumber);
			continue;
		}
		memcpy(zName, line + start, len * sizeof(char));
		zName[len] = '\0';

		char zValue[256];
		const int ret = GetVariable(zName, zValue, CountOf(zValue));
		if (ret < 0)
		{
			fwrite(line,1,strlen(line),fOut);
			continue;
		}
		fprintf(fOut, "%s=%s\n", zName, zValue);
	}

	fclose(fInt);
	fclose(fOut);

	char zBackFilename[FILENAME_MAX];
	strcpy(zBackFilename, gs_ConfigFilename);
	strcat(zBackFilename, ".back");

	unlink(zBackFilename);
	rename(gs_ConfigFilename, zBackFilename);
	rename(zTmpFilename, gs_ConfigFilename);

	return true;
}


} // namespace Config
} // namespace Sagitech

