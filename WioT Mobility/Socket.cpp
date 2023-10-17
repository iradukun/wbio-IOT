#include "WioTMobility.h"
#include "Socket.h"

#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <sstream>

constexpr socket_t InvalidSocket = -1;
CSocket::CSocket()
    : m_hSocket(InvalidSocket)
{
}

CSocket::~CSocket()
{
    if (m_hSocket != InvalidSocket)
    {
        close(m_hSocket);
        m_hSocket = InvalidSocket;
    }
}

bool CSocket::OnAccept(socket_t hSocket)
{
    Close();
    m_hSocket = hSocket;

    return true;
}

bool CSocket::OnConnect(socket_t hSocket)
{
    Close();
    m_hSocket = hSocket;

    return true;
}

void CSocket::Close()
{
    if (m_hSocket != InvalidSocket)
    {
        close(m_hSocket);
        m_hSocket = InvalidSocket;
    }
}

ssize_t CSocket::Receive(void* pBuffer, ssize_t lSize)
{
    return recv(m_hSocket, pBuffer, lSize, MSG_NOSIGNAL);
}

ssize_t CSocket::Send(const void* pBuffer, ssize_t lSize)
{
    return send(m_hSocket, pBuffer, lSize, MSG_NOSIGNAL);
}

int CSocket::GetErrorCode(int ret)
{
    return errno;
}

std::string CSocket::GetErrorString(int err)
{
    return strerror(err);
}

bool CSocket::SetNonBlocking()
{
    int flags = fcntl(m_hSocket, F_GETFL, 0);
    if (flags == -1 ||
        fcntl(m_hSocket, F_SETFL, flags | O_NONBLOCK))
    {
        return false;
    }
    return true;
}

std::string CSocket::GetAddressIP() const
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    std::string ip;
    if (getpeername(m_hSocket, reinterpret_cast<sockaddr*>(&addr), &len))
    {
        ip = "<error>";
    }
    else
    {
        std::ostringstream oss;
        oss << ((addr.sin_addr.s_addr) & 0xff)
            << "." << ((addr.sin_addr.s_addr >> 8) & 0xff)
            << "." << ((addr.sin_addr.s_addr >> 16) & 0xff)
            << "." << ((addr.sin_addr.s_addr >> 24) & 0xff)
            << ":" << ntohs(addr.sin_port);
        ip = oss.str();
    }
    return ip;
}
