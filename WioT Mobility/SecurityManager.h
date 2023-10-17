#pragma once
#include <openssl/ssl.h>
#include <netinet/in.h>
#include <vector>

class CSecurityManager
{
private:
	CSecurityManager();
	~CSecurityManager();

public:
	bool Start(const char *zCertificateFile,
			   const char* zPrivateKeyFile,
			   const char *zClientCertificatePath);
	bool Stop();

	SSL_CTX* GetBasicContext()
	{
		return m_SslContext_Basic;
	}

	SSL_CTX* GetClientContext()
	{
		return m_SslContext_Client;
	}

	bool Blacklist(const char* zAddrIP);
	bool IsBlacklisted(const sockaddr_in &Addr) const;

	static CSecurityManager& GetInstance();
private:
	void InitSSL();
	void CleanupSSL();
	bool CreateContext();
	bool DeleteContext();
	bool ConfigureContext(SSL_CTX* pContext,
						  const char* zCertificateFile,
						  const char* zPrivateKeyFile,
						  const char* zClientCertificatePath);

	SSL_CTX* m_SslContext_Basic;
	SSL_CTX* m_SslContext_Client;
	std::vector<unsigned long> m_Blacklist;
};

#define SecurityManager CSecurityManager::GetInstance()