#include "WioTMobility.h"
#include "ProtocolManager.h"
#include "ProtocolBase.h"

CProtocolManager::CProtocolManager()
{
}

CProtocolManager::~CProtocolManager()
{
}

const IProtocol* CProtocolManager::GetProtocol(const char *zProtocolName)
{
    return CProtocolBase::GetProtocol(zProtocolName);
}

int CProtocolManager::GetProtocolCount()
{
    return CProtocolBase::GetProtocolCount();
}

CProtocolManager& CProtocolManager::GetInstance()
{
    static CProtocolManager s_Instance;

    return s_Instance;
}
