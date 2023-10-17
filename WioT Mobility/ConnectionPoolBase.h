#pragma once
#include "IConnectionPool.h"
#include <boost/thread.hpp>
#include "boost/thread/mutex.hpp"
#include "boost/thread/condition.hpp"


class IProtocol;

typedef int LPoolIdx;
class CConnectionPoolBase
    : public IConnectionPool
{
public:
    CConnectionPoolBase(LPoolIdx lPoolIdx);
    virtual ~CConnectionPoolBase();

    bool Start();
    bool Stop();

    LPoolIdx GetPoolIdx() const
    {
        return m_lPoolIdx;
    }

    virtual bool HandleConnection(socket_t sock, uintptr_t data) = 0;

    static bool WaitForAllPoolStopped();

protected:
    bool WantStop() const
    {
        return m_bWantStop;
    }

private:
    LPoolIdx m_lPoolIdx;
    boost::atomic<bool> m_bWantStop;
    boost::thread m_Thread;

    void Thread();
    virtual void DoWork() = 0;

    static volatile int ms_nStartedCount;
    static boost::mutex ms_Mutex;
    static boost::condition ms_NoMorePoolRunning;

    friend struct TLauncher;
};

