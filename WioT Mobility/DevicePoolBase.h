#pragma once
#include <boost/thread.hpp>
#include "boost/thread/mutex.hpp"
#include "boost/thread/condition.hpp"
#include "Buffer.h"
#include <sys/socket.h>


constexpr int MaxDeviceInPool = 128;

typedef int LPoolIdx;
class IProtocol;
class CDevice;
class CDevicePoolBase
{
public:
    CDevicePoolBase(LPoolIdx lPoolIdx);
    virtual ~CDevicePoolBase();

    bool Start();
    bool Stop();

    LPoolIdx GetPoolIdx() const
    {
        return m_lPoolIdx;
    }

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
    void DoWork();

    static volatile int ms_nStartedCount;
    static boost::mutex ms_Mutex;
    static boost::condition ms_NoMorePoolRunning;

    friend struct TLauncher;

    CDevice *m_Devices[MaxDeviceInPool];
    volatile int m_nNbDevices;
    bool CheckForIncommingData();
    bool CloseAll();

public:
    boost::mutex m_Mutex;
    bool AddDevice(CDevice *pDevice);
    int GetDeviceCount();
    bool CanAcceptAnotherDevice();
};

