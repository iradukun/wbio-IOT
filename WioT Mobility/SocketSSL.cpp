#include "WioTMobility.h"
#include "SocketSSL.h"
#include "SecurityManager.h"
#include <openssl/err.h>
#include <string>


#define MAX_ERROR_MSG_LEN (512)
#if 0
std::string getOpenSSLError()
{
    BIO* bio = BIO_new(BIO_s_mem());
    ERR_print_errors(bio);
    char* buf;
    size_t len = BIO_get_mem_data(bio, &buf);
    std::string ret(buf, len);
    BIO_free(bio);
    return ret;
}
#endif
CSocketSSL::CSocketSSL(SSL_CTX* pCTX)
    : m_CTX(pCTX)
    , m_SSL(nullptr)
{
}

CSocketSSL::~CSocketSSL()
{
    if (m_SSL)
    {
        SSL_free(m_SSL);
        m_SSL = nullptr;
    }
}

bool CSocketSSL::OnAccept(socket_t hSocket)
{
    if (!m_CTX ||
        !CSocket::OnAccept(hSocket))
    {
        return false;
    }
    m_SSL = SSL_new(m_CTX);
    SSL_set_fd(m_SSL, hSocket);

    const int ret = SSL_accept(m_SSL);
    if (ret == 0)
    {
        const int err = SSL_get_error(m_SSL, ret);
        char sslErrorString[MAX_ERROR_MSG_LEN] = { '\0' };
        LogErr("WioT", "SSL connection was closed by client.\n%s",
            ERR_error_string(err, sslErrorString));
        return false;
    }
    if (ret < 0)
    {
//        ERR_print_errors_fp(stderr);
        const int err = SSL_get_error(m_SSL, ret);
        char sslErrorString[MAX_ERROR_MSG_LEN] = { '\0' };
        LogErr("WioT", "SSL connection failed.\n%s",
            ERR_error_string(err, sslErrorString));
        return false;
    }
    return true;
}

bool CSocketSSL::OnConnect(socket_t hSocket)
{
    if (!CSocket::OnAccept(hSocket))
    {
        return false;
    }
    m_SSL = SSL_new(m_CTX);
    SSL_set_fd(m_SSL, hSocket);
    return true;
}

void CSocketSSL::Close()
{
    if (m_SSL)
    {
        SSL_shutdown(m_SSL);
        SSL_free(m_SSL);
        m_SSL = nullptr;
    }
}

ssize_t CSocketSSL::Receive(void* pBuffer, ssize_t lSize)
{
    return SSL_read(m_SSL, pBuffer, static_cast<int>(lSize));
}

ssize_t CSocketSSL::Send(const void* pBuffer, ssize_t lSize)
{
    return SSL_write(m_SSL, pBuffer, static_cast<int>(lSize));
}

int CSocketSSL::GetErrorCode(int ret)
{
    return SSL_get_error(m_SSL, ret);
}

std::string CSocketSSL::GetErrorString(int err)
{
    BIO* bio = BIO_new(BIO_s_mem());
    ERR_print_errors(bio);
    char* buf;
    size_t len = BIO_get_mem_data(bio, &buf);
    std::string ret(buf, len);
    BIO_free(bio);
    return ret;
}
