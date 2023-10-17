#pragma once
#include <string>

namespace Sagitech {
namespace Config {

typedef bool (*Setter)(const void* pValue, void* pContext);
typedef bool (*Getter)(void* pValue, int nMaxLength, void* pContext);
typedef bool (*Executer)(void* pContext);

struct TVariable
{
	TVariable(): eType(EVariableType::Unknow), pNext(nullptr)
	{}
	std::string zName;
	EVariableType eType;
	int nIdx;

	unsigned uFlags;

	std::string zShortDescription;
	std::string zLongDescription;

	const void* pExtra;
	void* pContext;

	union
	{
		Setter Set;
		Executer Exec;
	};
	Getter Get;

	TVariable* pNext;
};

struct TSection;
struct TVariable;

class IVariableSink
{
public:
	virtual bool OnVariable(const char* zName, const char* zValue) = 0;
};

class CConfigManager
{
private:
	CConfigManager();
	~CConfigManager();
	CConfigManager(const CConfigManager&) = delete;
	CConfigManager& operator=(const CConfigManager&) = delete;

public:
	static CConfigManager& GetInstance();

	bool NewDescription();
	bool BeginSection(const char* zName);
	bool EndSection();

	bool AddVariable(const char* zName, EVariableType eType, const char* zShortDescription, const char* zLongDescription, 
		const void* pExtra, unsigned uFlags, void* pContext, Setter setter, Getter getter);
	bool AddCommand(const char* zName, const char* zShortDescription, const char* zLongDescription, 
		const void* pExtra, unsigned uFlags, void* pContext, Executer executer);

	const TVariable* GetVariable(const char* zName, const TSection* pParent = nullptr) const;

	int Describe(char* zDescription, int nMaxLength);

	bool ListAllVariables(IVariableSink* pSink) const;
private:
	bool FinalizeAddVariable(TVariable* pVariable);
	bool CheckNameValid(const char* zName);

	TSection* m_pRoot;
	TSection* m_pCurrentParent;
	TVariable** m_ppVariables;
	int m_nNbVariables;
};

} // namespace Config
} // namespace Sagitech


