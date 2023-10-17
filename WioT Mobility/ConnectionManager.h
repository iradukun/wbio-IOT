#pragma once
#include <boost/thread.hpp>
#include "boost/thread/mutex.hpp"
#include "boost/thread/condition.hpp"

typedef int LServerIdx;

constexpr int MaxConnectionPool = 64;
class IConnectionPool;
class CConnectionPoolBase;

class CConnectionManager
{
private:
	CConnectionManager();
	~CConnectionManager();

public:
	bool Start();
	bool Stop();

	bool GetServer(LServerIdx lServerIdx, IConnectionPool *&pServer);

	int GetActivePoolCount() const;

	static CConnectionManager& GetInstance();

	bool OnIncommingConnection(socket_t fd, CConnectionPoolBase* pPool, uintptr_t data);

	int GetConnectionInProgress() const;
private:
	CConnectionPoolBase* m_Pools[MaxConnectionPool];

	boost::atomic<bool> m_bWantStop;
	boost::thread m_Thread;

	void DoWork();

	mutable boost::mutex m_Mutex;
	boost::condition m_NewIncoming;

	struct TWaitingAttempt
	{
		socket_t fd;
		CConnectionPoolBase* pPool;
		uintptr_t data;
		time_t conn_date;
	};
	std::vector<TWaitingAttempt> m_Incomings;

	bool HandleConnection(int nConnIdx);
};

#define ConnectionManager CConnectionManager::GetInstance()
