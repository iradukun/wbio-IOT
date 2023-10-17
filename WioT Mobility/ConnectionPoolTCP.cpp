#include "WioTMobility.h"
#include "ConnectionPoolTCP.h"
#include "ProtocolManager.h"
#include "SecurityManager.h"
#include "ConnectionManager.h"
#include <boost/chrono.hpp>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory.h>
#include <poll.h>
#include <cerrno>
#include "DeviceManager.h"
#include "SecurityManager.h"
#include "SocketSSL.h"

CConnectionPoolTCP::CConnectionPoolTCP(LPoolIdx lPoolIdx)
    : CConnectionPoolBase(lPoolIdx)
    , m_nNbServers{0}
{
}

CConnectionPoolTCP::~CConnectionPoolTCP()
{
}

bool CConnectionPoolTCP::AddServer(int nListenPort, const char* zProtocol, ESecurity eSecurity)
{
    LogDebug("WioT", "[CONPOOL%d] Entering AddServer(%d, \"%s\")", GetPoolIdx(), nListenPort, zProtocol);

    if (nListenPort <= 0 && nListenPort >= 65536)
    {
        LogErr("WioT", "[CONPOOL%d] Invalid value for listen port %d. Must be between 1 and 65535", GetPoolIdx(), nListenPort);
        return false;
    }
    if (!zProtocol || zProtocol[0]=='\0')
    {
        LogErr("WioT", "[CONPOOL%d] Protocol cannot be null or empty", GetPoolIdx());
        return false;
    }
    if (m_nNbServers >= MaxServerInPool)
    {
        LogErr("WioT", "[CONPOOL%d] The maximum connection count is reached on this server pool.\n"
                "Consider to create a new server pool.", GetPoolIdx());
        return false;
    }
    const IProtocol* pProtocol = ProtocolManager.GetProtocol(zProtocol);
    if (!pProtocol)
    {
        LogErr("WioT", "[CONPOOL%d] The protocol named \"%s\" was not found. Check the syntax.", GetPoolIdx(), zProtocol);
        return false;
    }

    // for the moment, we didn't check if the port is already reserved by another protocol
    // we will see later when creating listen sockets
    m_Servers[m_nNbServers].nListenPort = nListenPort;
    m_Servers[m_nNbServers].pProtocol = pProtocol;
    m_Servers[m_nNbServers].eSecurity = eSecurity;
    m_nNbServers++;
    LogInf("WioT", "[CONPOOL%d] Protocol \"%s\" was successfully added listening on port %d", GetPoolIdx(),
        zProtocol, nListenPort);

    return true;
}

static bool CheckSSl(socket_t connfd)
{
    // there is incomming data...
    uint8_t buf[1];

    const auto ret = recv(connfd, buf, 1, MSG_PEEK);
    if (ret > 0 &&
        buf[0] == 0x16)
    {
        return true;
    }
    // nothing comed or it was not beginning with 0x16
    return false;
}

CSocket* CConnectionPoolTCP::NewSocket(ESecurity eSecurity, socket_t connfd)
{
    switch (eSecurity)
    {
    case ESecurity::NoSSL:
        break;
    case ESecurity::AllowSSL:
        if (!CheckSSl(connfd))
        {
            break;
        }
    [[fallthrough]];
    case ESecurity::UseSSLOnly:
        return new CSocketSSL(SecurityManager.GetBasicContext());
    }
    return new CSocket;
}

int CConnectionPoolTCP::CreateSockets()
{
    LogInf("WioT", "[CONPOOL%d] Try to start %d servers.", GetPoolIdx(), m_nNbServers);

    // first, we create sockets
    for (int i = 0; i < m_nNbServers; ++i)
    {
        socket_t listenfd = socket(AF_INET, SOCK_STREAM, 0);

        int reuse = 1;
        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
        {
            const int err = errno;
            LogErr("WioT", "[CONPOOL%d] Cannot set SO_REUSEADDR for protocol \"%s\" on port %d.\n"
                "Error %d : %s", GetPoolIdx(),
                m_Servers[i].pProtocol->GetName(),
                m_Servers[i].nListenPort,
                err, strerror(err));

            close(listenfd);
            m_Servers[i].server = INVALID_SOCKET;
            continue;
        }

#ifdef SO_REUSEPORT
        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0)
        {
            const int err = errno;
            LogErr("WioT", "[CONPOOL%d] Cannot set SO_REUSEPORT protocol \"%s\" on port %d.\n"
                "Error %d : %s", GetPoolIdx(),
                m_Servers[i].pProtocol->GetName(),
                m_Servers[i].nListenPort,
                err, strerror(err));

            close(listenfd);
            m_Servers[i].server = INVALID_SOCKET;
            continue;
        }
#endif
        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(static_cast<uint16_t>(m_Servers[i].nListenPort));
        if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)))
        {
            const int err = errno;
            LogErr("WioT", "[CONPOOL%d] Cannot bind protocol \"%s\" on port %d.\n"
                "Error %d : %s", GetPoolIdx(),
                m_Servers[i].pProtocol->GetName(),
                m_Servers[i].nListenPort,
                err, strerror(err));

            close(listenfd);
            m_Servers[i].server = INVALID_SOCKET;
            continue;
        }
        LogInf("WioT", "[CONPOOL%d] Protocol \"%s\" successfully started on port %d (sock=%d).",
            GetPoolIdx(),
            m_Servers[i].pProtocol->GetName(),
            m_Servers[i].nListenPort,
            listenfd);
        m_Servers[i].server = listenfd;
    }
    // then, we move all not openned socket to the end
    int last = m_nNbServers - 1;
    int count = 0;
    while (count <= last)
    {
        if (m_Servers[count].server != INVALID_SOCKET)
        {
            // it is ok, so pass to the next
            count++;
            continue;
        }
        // this server does not succeeded in starting its socket
        // so we push it to the end
        // we exchange with the last
        TServer tmp = m_Servers[count];
        m_Servers[count] = m_Servers[last];
        m_Servers[last] = tmp;
        last--;
    }
    if (last < 0)
    {
        LogErr("WioT", "[CONPOOL%d] No server succeeded in creating its socket. Killing this pool.", GetPoolIdx());
        return 0;
    }

    // let's listen
    for (int i = 0; i <= last; ++i)
    {
        if (listen(m_Servers[i].server, -1) != 0)
        {
            const int err = errno;
            LogErr("WioT", "[CONPOOL%d] Error while listening port %d.\n"
                "Error %d : %s",
                GetPoolIdx(),
                m_Servers[i].nListenPort,
                err, strerror(err));
        }
    }

    return last + 1;
}

bool CConnectionPoolTCP::CloseSockets()
{
    for (int i = 0; i < m_nNbServers; ++i)
    {
        if (m_Servers[i].server)
        {
            close(m_Servers[i].server);
        }
    }
    return true;
}

bool CConnectionPoolTCP::WaitConnections(int nNbValid)
{
    pollfd fds[MaxServerInPool];

    for (int i = 0; i < nNbValid; ++i)
    {
        fds[i].fd = m_Servers[i].server;
        fds[i].events = POLLIN;
        fds[i].revents = 0;
    }

    int res = poll(fds, nNbValid, 1000);
    if (res > 0)
    {
        // there is a connection attempt...
        for (int i = 0; i < nNbValid; ++i)
        {
            if (fds[i].revents & POLLIN)
            {
                OnNewConnection(i);
            }
        }
    }

    return true;
}

bool CConnectionPoolTCP::OnNewConnection(int nServerIdx)
{
    struct sockaddr_in peer_addr;
    socklen_t sockaddr_len = sizeof(peer_addr);
    int connfd = accept(m_Servers[nServerIdx].server,
        reinterpret_cast<sockaddr*>(&peer_addr), &sockaddr_len);
    if (connfd < 0)
    {
        const int err = errno;
        LogErr("WioT", "[CONPOOL%d] Failed to accept connection.\n"
            "Error %d : %s", GetPoolIdx(),
            err, strerror(err));
        return false;
    }

    if (SecurityManager.IsBlacklisted(peer_addr))
    {
        close(connfd);
        LogErr("WioT", "[CONPOOL%d] Rejected connection from blacklisted address %d.%d.%d.%d", GetPoolIdx(),
            (peer_addr.sin_addr.s_addr) & 0xff,
            (peer_addr.sin_addr.s_addr >> 8) & 0xff,
            (peer_addr.sin_addr.s_addr >> 16) & 0xff,
            (peer_addr.sin_addr.s_addr >> 24) & 0xff);
        return false;
    }

    LogInf("WioT", "[CONPOOL%d] Connection in progress from %d.%d.%d.%d:%d", GetPoolIdx(),
        (peer_addr.sin_addr.s_addr) & 0xff,
        (peer_addr.sin_addr.s_addr >> 8) & 0xff,
        (peer_addr.sin_addr.s_addr >> 16) & 0xff,
        (peer_addr.sin_addr.s_addr >> 24) & 0xff,
        ntohs(peer_addr.sin_port));
    return ConnectionManager.OnIncommingConnection(connfd, this, nServerIdx);
}

bool CConnectionPoolTCP::HandleConnection(socket_t sock, uintptr_t data)
{
    struct sockaddr_in peer_addr;
    socklen_t sockaddr_len = sizeof(peer_addr);

    const int nServerIdx = static_cast<int>(data);
    getpeername(sock, reinterpret_cast<sockaddr*>(&peer_addr), &sockaddr_len);
    CSocket* pSocket = NewSocket(m_Servers[nServerIdx].eSecurity, sock);
    if (!pSocket->OnAccept(sock))
    {
        delete pSocket;
        close(sock);
        LogErr("WioT", "[CONPOOL%d] Connection refused from %d.%d.%d.%d:%d", GetPoolIdx(),
        (peer_addr.sin_addr.s_addr) & 0xff,
        (peer_addr.sin_addr.s_addr >> 8) & 0xff,
        (peer_addr.sin_addr.s_addr >> 16) & 0xff,
        (peer_addr.sin_addr.s_addr >> 24) & 0xff,
        ntohs(peer_addr.sin_port));
        return true; // return true since no longer need to manage the connection
    }
    if (!DeviceManager.AddDevice(pSocket, m_Servers[nServerIdx].pProtocol))
    {
        delete pSocket;
        LogErr("WioT", "[CONPOOL%d] Failed to append the device to device pool", GetPoolIdx());
        return true; // return true since no longer need to manage the connection
    }
    LogInf("WioT", "[CONPOOL%d] New connection from %d.%d.%d.%d:%d", GetPoolIdx(),
        (peer_addr.sin_addr.s_addr) & 0xff,
        (peer_addr.sin_addr.s_addr >> 8) & 0xff,
        (peer_addr.sin_addr.s_addr >> 16) & 0xff,
        (peer_addr.sin_addr.s_addr >> 24) & 0xff,
        ntohs(peer_addr.sin_port));

    return true;
}

void CConnectionPoolTCP::DoWork()
{
    LogInf("WioT", "[CONPOOL%d] Entering thread", GetPoolIdx());

    const int nNbValid = CreateSockets();
    if (nNbValid > 0)
    {
        while (!WantStop())
        {
            if (!WaitConnections(nNbValid))
            {
                break;
            }
        }
        CloseSockets();
    }
    LogInf("WioT", "[CONPOOL%d] Leaving thread", GetPoolIdx());
}
