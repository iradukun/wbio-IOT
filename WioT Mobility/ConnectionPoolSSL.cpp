#include "WioTMobility.h"
#include "ConnectionPoolSSL.h"
#include "SocketSSL.h"

CConnectionPoolSSL::CConnectionPoolSSL(LPoolIdx lPoolIdx)
    : CConnectionPoolTCP(lPoolIdx)
{
}

CConnectionPoolSSL::~CConnectionPoolSSL()
{
}

CSocket* CConnectionPoolSSL::NewSocket()
{
    //return new CSocketSSL;
    return nullptr;
}
