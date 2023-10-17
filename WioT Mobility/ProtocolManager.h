#pragma once

#include "IProtocol.h"

class CProtocolManager
{
private:
	CProtocolManager();
	~CProtocolManager();

public:
	const IProtocol* GetProtocol(const char *zProtocolName);

	int GetProtocolCount();

public:
	static CProtocolManager& GetInstance();
};

#define ProtocolManager CProtocolManager::GetInstance()