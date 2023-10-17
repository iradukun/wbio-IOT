#include "WioTMobility.h"
#include "ConnectionPoolBase.h"

volatile int CConnectionPoolBase::ms_nStartedCount;
boost::mutex CConnectionPoolBase::ms_Mutex;
boost::condition CConnectionPoolBase::ms_NoMorePoolRunning;

CConnectionPoolBase::CConnectionPoolBase(LPoolIdx lPoolIdx)
    : m_lPoolIdx(lPoolIdx)
    , m_bWantStop(false)
{
}

CConnectionPoolBase::~CConnectionPoolBase()
{
}

bool CConnectionPoolBase::Start()
{
    struct TLauncher
    {
        CConnectionPoolBase* pPool;

        TLauncher(CConnectionPoolBase* __pPool) : pPool(__pPool)
        {}

        void operator()()
        {
            pPool->Thread();
        }
    } launcher(this);

    m_Thread = boost::thread{ launcher };

    return true;
}

bool CConnectionPoolBase::Stop()
{
    m_bWantStop = true;
    return true;
}

bool CConnectionPoolBase::WaitForAllPoolStopped()
{
    {
        boost::mutex::scoped_lock lock(ms_Mutex);
        while (ms_nStartedCount > 0)
        {
            boost::xtime xt;
            boost::xtime_get(&xt, boost::TIME_UTC_);
            xt.sec += WaitThreadTimeout;
            if (ms_NoMorePoolRunning.timed_wait(lock, xt))
            {
                break;
            }
            LogWarn("WioT", "[CONNPOOL] Waiting for %d connection pools to stop...", ms_nStartedCount);
        }
    }

    LogInf("WioT", "[CONNPOOL] All connection pools are stopped");

    return false;
}

void CConnectionPoolBase::Thread()
{
    {
        boost::mutex::scoped_lock lock(ms_Mutex);
        ms_nStartedCount++;
    }

    DoWork();

    {
        boost::mutex::scoped_lock lock(ms_Mutex);

        if (--ms_nStartedCount == 0)
        {
            ms_NoMorePoolRunning.notify_one();
        }
    }
}
