#pragma once
#include "IProtocol.h"


class CProtocolBase
	: public IProtocol
{
public:
	CProtocolBase();
	~CProtocolBase();

	static CProtocolBase* GetProtocol(const char* zProtocolName);
	static int GetProtocolCount();

private:
	static int ms_nProtocolCount;
	static CProtocolBase* ms_pFirst;
	CProtocolBase* m_pNext;
};

