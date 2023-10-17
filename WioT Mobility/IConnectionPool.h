#pragma once
class IConnectionPool
{
public:
	virtual bool AddServer(int nListenPort, const char* zProtocol, ESecurity eSecurity) = 0;
};

