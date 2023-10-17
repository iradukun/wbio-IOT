#include "WioTMobility.h"
#include "SecurityManager.h"
#include <openssl/err.h>
#include <algorithm>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#include "deelx.h"
#pragma GCC diagnostic pop

CSecurityManager::CSecurityManager()
    : m_SslContext_Basic(nullptr)
    , m_SslContext_Client(nullptr)
{
}

CSecurityManager::~CSecurityManager()
{
    DeleteContext();
}

bool CSecurityManager::Start(const char* zCertificateFile,
                            const char* zPrivateKeyFile,
                            const char* zClientCertificatePath)
{
    InitSSL();

    if (!CreateContext())
    {
        LogErr("WioT", "Unable to create SSL context");
        return false;
    }
    if (!ConfigureContext(m_SslContext_Basic, zCertificateFile, zPrivateKeyFile, nullptr))
    {
        LogErr("WioT", "Error configuring basic SSL context");
        return false;
    }
    if (!ConfigureContext(m_SslContext_Client, zCertificateFile, zPrivateKeyFile, zClientCertificatePath))
    {
        LogErr("WioT", "Error configuring client SSL context");
        return false;
    }

    return true;
}

bool CSecurityManager::Stop()
{
    DeleteContext();
    CleanupSSL();
    return true;
}

bool CSecurityManager::Blacklist(const char* zAddrIP)
{
    if (!zAddrIP)
    {
        return false;
    }
    CRegexpA regex("^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");

    MatchResult result;
    result = regex.Match(zAddrIP);
    if (!result.IsMatched())
    {
        LogErr("WioT", "[SECUMAN] Invalid value for blacklisted IP address : %s", zAddrIP);
        return false;
    }

    unsigned long lAddrIP = inet_addr(zAddrIP);

    m_Blacklist.push_back(lAddrIP);
    LogDebug("WioT", "[SECUMAN] Address %s was added to the blacklist.", zAddrIP);

    return true;
}

bool CSecurityManager::IsBlacklisted(const sockaddr_in& Addr) const
{
    return std::find(m_Blacklist.begin(), m_Blacklist.end(), Addr.sin_addr.s_addr) != m_Blacklist.end();
}

CSecurityManager& CSecurityManager::GetInstance()
{
    static CSecurityManager s_Instance;

    return s_Instance;
}

void CSecurityManager::InitSSL()
{
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void CSecurityManager::CleanupSSL()
{
    EVP_cleanup();
}

bool CSecurityManager::CreateContext()
{
    const SSL_METHOD* method;
    SSL_CTX* ctx;

    method = SSLv23_server_method();

    ctx = SSL_CTX_new(method);
    if (!ctx)
    {
        LogErr("WioT", "Error creating basic SSL context.\n%s",
            ERR_error_string(ERR_get_error(), NULL));
        return false;
    }
    m_SslContext_Basic = ctx;

    ctx = SSL_CTX_new(method);
    if (!ctx)
    {
        LogErr("WioT", "Error creating client SSL context.\n%s",
            ERR_error_string(ERR_get_error(), NULL));
        return false;
    }
    m_SslContext_Client = ctx;

    return true;
}

bool CSecurityManager::DeleteContext()
{
    if (m_SslContext_Basic)
    {
        SSL_CTX_free(m_SslContext_Basic);
        m_SslContext_Basic = nullptr;
    }
    if (m_SslContext_Client)
    {
        SSL_CTX_free(m_SslContext_Client);
        m_SslContext_Client = nullptr;
    }
    return true;
}

bool CSecurityManager::ConfigureContext(SSL_CTX* pContext,
                                        const char* zCertificateFile,
                                        const char* zPrivateKeyFile,
                                        const char* zClientCertificatePath)
{
    SSL_CTX_set_ecdh_auto(pContext, 1);

    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(pContext, zCertificateFile, SSL_FILETYPE_PEM) <= 0)
    {
        LogErr("WioT", "Error setting SSL certificate to \"%s\".\n%s",
            zCertificateFile,
            ERR_error_string(ERR_get_error(), NULL));
        return false;
    }

    if (SSL_CTX_use_PrivateKey_file(pContext, zPrivateKeyFile, SSL_FILETYPE_PEM) <= 0)
    {
        LogErr("WioT", "Error setting SSL private key to \"%s\".\n%s",
            zPrivateKeyFile,
            ERR_error_string(ERR_get_error(), NULL));
        return false;
    }

    return true;
}
