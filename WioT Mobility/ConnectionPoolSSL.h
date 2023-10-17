#pragma once
#include "ConnectionPoolTCP.h"

class CConnectionPoolSSL
	: public CConnectionPoolTCP
{
public:
	CConnectionPoolSSL(LPoolIdx lPoolIdx);
	virtual ~CConnectionPoolSSL();
private:
	virtual CSocket* NewSocket();
};

