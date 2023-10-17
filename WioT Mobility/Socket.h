#pragma once
#include <string>
#include <openssl/ssl.h>

class CSocket
{
public:
	CSocket();
	virtual ~CSocket();

	virtual bool OnAccept(socket_t hSocket);
	virtual bool OnConnect(socket_t hSocket);
	virtual void Close();

	virtual ssize_t Receive(void* pBuffer, ssize_t lSize);
	virtual ssize_t Send(const void* pBuffer, ssize_t lSize);

	virtual int GetErrorCode(int ret);
	virtual std::string GetErrorString(int err);

	bool SetNonBlocking();

	socket_t GetSocket() const
	{
		return m_hSocket;
	}

	std::string GetAddressIP() const;
private:
	socket_t m_hSocket;
};

