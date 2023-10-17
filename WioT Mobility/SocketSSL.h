#pragma once
#include "Socket.h"
#include <openssl/ssl.h>

class CSocketSSL :
    public CSocket
{
public:
    CSocketSSL(SSL_CTX *pCTX);
    virtual ~CSocketSSL();

	bool OnAccept(socket_t hSocket) override;
	bool OnConnect(socket_t hSocket) override;
	void Close();

	ssize_t Receive(void* pBuffer, ssize_t lSize) override;
	ssize_t Send(const void* pBuffer, ssize_t lSize) override;

	int GetErrorCode(int ret) override;
	std::string GetErrorString(int err) override;

private:
	CSocketSSL() = delete;
	CSocketSSL(const CSocketSSL&) = delete;
	CSocketSSL&operator=(const CSocketSSL&) = delete;
	SSL_CTX* m_CTX;
	SSL* m_SSL;
};

