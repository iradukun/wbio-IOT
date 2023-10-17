#pragma once
#include <boost/lockfree/queue.hpp>
#include <boost/thread.hpp>
#include "boost/thread/mutex.hpp"
#include "boost/thread/condition.hpp"
#include "vector"
#include "SocketSSL.h"
#include "CommandPrompt.h"


class ICommandMessage
{
public:
	virtual ~ICommandMessage() {}
	virtual bool Process() = 0;
};

class CCommandManager
{
private:
	CCommandManager();
	~CCommandManager();

public:
	bool Start();
	bool Stop();

	void SetListenPort(int nPort);
	void SetCertificateFile(const char* zCertificateFile);
	void SetPrivateKeyFile(const char* zPrivateKeyFile);

	static CCommandManager& GetInstance();

	bool OnCommandPromptEnded(CCommandPrompt* pPrompt);
private:
	boost::thread m_Thread;
	boost::mutex m_Mutex;
	std::list<CCommandPrompt*> m_Prompts;
	volatile int m_nStartedCount;

	int m_nListenPort;
	std::string m_zCertificateFile;
	std::string m_zPrivateKeyFile;

	boost::lockfree::queue<ICommandMessage*> m_msgQueue;

	boost::atomic<bool> m_bWantStop;

	void Thread();
	void ConsumeMessage();
	bool HandleConnection(socket_t listenfd, SSL_CTX *pCTX);
	bool AddCommandPrompt(CSocket* pSocket, socket_t connfd);
	bool WaitForAllPromptStopped();

	socket_t CreateServerSocket();
};

#define CommandManager CCommandManager::GetInstance()

