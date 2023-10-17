#pragma once
#include "ConnectionPoolBase.h"
#include <sys/socket.h>
#include <cstdint>
#include <vector>

class IProtocol;

constexpr int MaxServerInPool = 64;

class CSocket;

class CConnectionPoolTCP
    : public CConnectionPoolBase
{
public:
    CConnectionPoolTCP(LPoolIdx lPoolIdx);
    virtual ~CConnectionPoolTCP();

    // Hérité via IConnectionPool
    bool AddServer(int nListenPort, const char* zProtocol, ESecurity eSecurity) override;

private:
    CSocket* NewSocket(ESecurity eSecurity, socket_t connfd);

    struct TServer
    {
        int nListenPort{ 0 };
        const IProtocol* pProtocol{ nullptr };
        socket_t server{ INVALID_SOCKET };
        ESecurity eSecurity;
    };

    int m_nNbServers;
    TServer m_Servers[MaxServerInPool];

    int CreateSockets();
    bool CloseSockets();
    bool WaitConnections(int nNbValid);

    bool OnNewConnection(int nServerIdx);
    bool HandleConnection(socket_t sock, uintptr_t data) override;

    // Hérité via CConnectionPoolBase
    virtual void DoWork() override;
};

