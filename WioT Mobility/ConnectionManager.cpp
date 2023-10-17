#include "WioTMobility.h"
#include "ConnectionManager.h"
#include "ConnectionPoolTCP.h"
#include <poll.h>

constexpr int IncommingDataTimeout = 60;

CConnectionManager::CConnectionManager()
	: m_bWantStop(false)
{
	for (int i = 0; i < MaxConnectionPool; ++i)
	{
		m_Pools[i] = nullptr;
	}
}

CConnectionManager::~CConnectionManager()
{
	for (int i = 0; i < MaxConnectionPool; ++i)
	{
		delete m_Pools[i];
	}
}

bool CConnectionManager::Start()
{
	int nNbPools = 0;
	for (int i = 0; i < MaxConnectionPool; ++i)
	{
		if (m_Pools[i] &&
			m_Pools[i]->Start())
		{
			nNbPools++;
		}
	}
	if (nNbPools == 0)
	{
		LogWarn("WioT", "[CONMAN] No connection pool was successfully started.");
		return false;
	}
	struct TLauncher
	{
		CConnectionManager* pManager;

		TLauncher(CConnectionManager* __pManager) : pManager(__pManager)
		{}

		void operator()()
		{
			pManager->DoWork();
		}
	} launcher(this);

	m_Thread = boost::thread{ launcher };

	return true;
}

bool CConnectionManager::Stop()
{
	m_bWantStop = true;
	{
		boost::mutex::scoped_lock lock(m_Mutex);
		m_NewIncoming.notify_all();
	}

	if (m_Thread.joinable())
	{
		for (;;)
		{
			if (m_Thread.timed_join(boost::chrono::seconds(WaitThreadTimeout)))
			{
				break;
			}
			LogWarn("WioT", "[CONMAN] Waiting for thread to stop...");
		}
	}

	int nNbPools = 0;
	for (int i = 0; i < MaxConnectionPool; ++i)
	{
		if (m_Pools[i])
		{
			// Stop() will return immediatelly,
			// We will have to wait for task to complete
			m_Pools[i]->Stop();
			nNbPools++;
		}
	}
	if (nNbPools)
	{
		CConnectionPoolBase::WaitForAllPoolStopped();
		for (int i = 0; i < MaxConnectionPool; ++i)
		{
			if (m_Pools[i])
			{
				delete m_Pools[i];
				m_Pools[i] = nullptr;
			}
		}
	}

	boost::mutex::scoped_lock lock(m_Mutex);
	for (auto sock : m_Incomings)
	{
		close(sock.fd);
	}
	m_Incomings.clear();

	return true;
}

bool CConnectionManager::GetServer(LServerIdx lServerIdx, IConnectionPool*& pServer)
{
	if (lServerIdx < 0 || lServerIdx >= MaxConnectionPool)
	{
		LogErr("WioT", "[CONMAN] Invalid server index %d. Should be 0 <= IDX < %d", lServerIdx, MaxConnectionPool);
		return false;
	}
	CConnectionPoolBase* pServerTCP = m_Pools[lServerIdx];
	if (!pServerTCP)
	{
		pServerTCP = new CConnectionPoolTCP(lServerIdx);
		m_Pools[lServerIdx] = pServerTCP;
	}
	pServer = pServerTCP;

	return true;
}

int CConnectionManager::GetActivePoolCount() const
{
	int nCount = 0;
	for (int i=0;i< MaxConnectionPool;++i)
	{
		if (m_Pools[i])
		{
			nCount++;
		}
	}
	return nCount;
}

CConnectionManager& CConnectionManager::GetInstance()
{
	static CConnectionManager s_Instance;
	return s_Instance;
}

bool CConnectionManager::OnIncommingConnection(socket_t fd, CConnectionPoolBase* pPool, uintptr_t data)
{
	TWaitingAttempt tmp;

	tmp.fd = fd;
	tmp.pPool = pPool;
	tmp.data = data;
	tmp.conn_date = time(nullptr);

	boost::mutex::scoped_lock lock(m_Mutex);
	m_Incomings.push_back(tmp);
	m_NewIncoming.notify_all();
	return true;
}

int CConnectionManager::GetConnectionInProgress() const
{
	boost::mutex::scoped_lock lock(m_Mutex);
	return static_cast<int>(m_Incomings.size());
}

void CConnectionManager::DoWork()
{
	while (!m_bWantStop)
	{
		int nNbIncomings = 0;
		std::vector<pollfd> fds;
		{
			const time_t now = time(nullptr);

			boost::mutex::scoped_lock lock(m_Mutex);
			if (!m_Incomings.empty())
			{
				fds.resize(m_Incomings.size());
				for (auto iter = m_Incomings.begin(); iter != m_Incomings.end();)
				{
					if (iter->conn_date == 0 ||
						(now - iter->conn_date) > IncommingDataTimeout)
					{
						if (iter->conn_date != 0)
						{
							struct sockaddr_in peer_addr;
							socklen_t sockaddr_len = sizeof(peer_addr);
							getpeername(iter->fd, reinterpret_cast<sockaddr*>(&peer_addr), &sockaddr_len);
							LogErr("WioT", "[CONMAN] No data from %d.%d.%d.%d:%d. Connection closed.",
								(peer_addr.sin_addr.s_addr) & 0xff,
								(peer_addr.sin_addr.s_addr >> 8) & 0xff,
								(peer_addr.sin_addr.s_addr >> 16) & 0xff,
								(peer_addr.sin_addr.s_addr >> 24) & 0xff,
								ntohs(peer_addr.sin_port));
							close(iter->fd);
						}
						iter = m_Incomings.erase(iter);
					}
					else
					{
						fds[nNbIncomings].fd = iter->fd;
						fds[nNbIncomings].events = POLLIN;
						fds[nNbIncomings].revents = 0;
						++nNbIncomings;
						iter++;
					}
				}
			}
		}

		if (nNbIncomings == 0)
		{
			// no incoming connection, so wait...
			boost::mutex::scoped_lock lock(m_Mutex);
			boost::xtime xt;
			boost::xtime_get(&xt, boost::TIME_UTC_);
			xt.sec += WaitThreadTimeout;
			m_NewIncoming.timed_wait(lock, xt);
			// either there is or there is not a connection, we loop...
			continue;
		}

		int res = poll(fds.data(), nNbIncomings, 1000);
		if (res > 0)
		{
			// some connections are in attempt
			for (int i = 0; i < nNbIncomings; ++i)
			{
				if (fds[i].revents & POLLIN &&
					HandleConnection(i))
				{
					boost::mutex::scoped_lock lock(m_Mutex);
					// mark to delete
					m_Incomings[i].conn_date = 0;
				}
			}
		}
	}


}

bool CConnectionManager::HandleConnection(int nConnIdx)
{
	TWaitingAttempt tmp;

	{
		boost::mutex::scoped_lock lock(m_Mutex);
		if (nConnIdx < 0 || nConnIdx >= static_cast<int>(m_Incomings.size()))
		{
			return true; // return true since no longer need to manage the connection
		}
		tmp = m_Incomings[nConnIdx];
	}

	return tmp.pPool->HandleConnection(tmp.fd, tmp.data);
}
