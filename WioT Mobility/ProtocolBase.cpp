#include "WioTMobility.h"
#include "ProtocolBase.h"
#include <cstring>

CProtocolBase* CProtocolBase::ms_pFirst;
int CProtocolBase::ms_nProtocolCount;

CProtocolBase::CProtocolBase()
{
	m_pNext = ms_pFirst;
	ms_pFirst = this;
	ms_nProtocolCount++;
}

CProtocolBase::~CProtocolBase()
{
	if (ms_pFirst == this)
	{
		ms_pFirst = m_pNext;
	}
	else
	{
		CProtocolBase* pCurrent = ms_pFirst;
		while (pCurrent)
		{
			if (pCurrent->m_pNext == this)
			{
				pCurrent->m_pNext = m_pNext;
				break;
			}
		}
	}
}

CProtocolBase* CProtocolBase::GetProtocol(const char* zProtocolName)
{
	CProtocolBase* pCurrent = ms_pFirst;
	while (pCurrent)
	{
		if (strcmp(pCurrent->GetName(),zProtocolName)==0)
		{
			return pCurrent;
		}
		pCurrent = pCurrent->m_pNext;
	}
	return nullptr;
}

int CProtocolBase::GetProtocolCount()
{
	return ms_nProtocolCount;
}
