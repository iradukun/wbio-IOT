#pragma once
#include <boost/thread.hpp>
#include "duktape.h"

class CSocket;

class CCommandPrompt
{
public:
	CCommandPrompt();
	~CCommandPrompt();

	bool Start(CSocket* pSocket, socket_t connfd);
	bool Stop();

	bool Join();

	void Print(const std::string& msg);
	void PrintError(const char* zReturnStatus, const char* zErrorMessage = nullptr);
private:
	CSocket* m_pSocket;
	boost::atomic<bool> m_bWantStop;
	boost::thread m_Thread;
	int m_nCurrentOffset;
	char m_Buffer[512];

	void Thread(socket_t connfd);
	void DoWork();
	bool OnIncomingData(duk_context* ctx);
	void InitializeJavascriptContext(duk_context* ctx);
	void FinalizeJavascriptContext(duk_context* ctx);
	void EvalString(duk_context* ctx, const char *zScript);

	static void FatalHandler(void* udata, const char* msg);
};

