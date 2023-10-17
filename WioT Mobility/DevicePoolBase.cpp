#include "WioTMobility.h"
#include "DeviceManager.h"
#include "DevicePoolBase.h"
#include "Device.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <cerrno>
#include <cstring>

volatile int CDevicePoolBase::ms_nStartedCount;
boost::mutex CDevicePoolBase::ms_Mutex;
boost::condition CDevicePoolBase::ms_NoMorePoolRunning;

CDevicePoolBase::CDevicePoolBase(LPoolIdx lPoolIdx)
    : m_lPoolIdx(lPoolIdx)
    , m_bWantStop(false)
    , m_nNbDevices{0}
{
    for (int i = 0; i < MaxDeviceInPool; ++i)
    {
        m_Devices[i] = nullptr;
    }

}

CDevicePoolBase::~CDevicePoolBase()
{
    for (int i = 0; i < MaxDeviceInPool; ++i)
    {
        if (m_Devices[i])
        {
            m_Devices[i]->Release();
            m_Devices[i] = nullptr;
        }
    }
}

bool CDevicePoolBase::Start()
{
    struct TLauncher
    {
        CDevicePoolBase* pPool;

        TLauncher(CDevicePoolBase* __pPool) : pPool(__pPool)
        {}

        void operator()()
        {
            pPool->Thread();
        }
    } launcher(this);

    m_Thread = boost::thread{ launcher };

    return true;
}

bool CDevicePoolBase::Stop()
{
    m_bWantStop = true;
    return true;
}

bool CDevicePoolBase::WaitForAllPoolStopped()
{
    {
        boost::mutex::scoped_lock lock(ms_Mutex);
        if (ms_nStartedCount > 0)
        {
            do
            {
                LogDebug("WioT", "Left %d pool thread running", ms_nStartedCount);
                boost::xtime xt;
                boost::xtime_get(&xt, boost::TIME_UTC_);
                xt.sec += WaitThreadTimeout;
                if (ms_NoMorePoolRunning.timed_wait(lock, xt))
                {
                    break;
                }
                LogWarn("WioT", "Waiting for %d device pools to stop...", ms_nStartedCount);
            } while (ms_nStartedCount > 0);
        }
        else
        {
            LogDebug("WioT", "No device pool thread running");
        }
    }

    LogInf("WioT", "All device pools are stopped");

    return true;
}

void CDevicePoolBase::Thread()
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

void CDevicePoolBase::DoWork()
{
    LogInf("WioT", "[SENSPOOL%d] Entering thread", GetPoolIdx());

    while (!WantStop())
    {
        if (!CheckForIncommingData())
        {
            break;
        }
    }

    CloseAll();

    LogInf("WioT", "[SENSPOOL%d] Leaving thread", GetPoolIdx());
}

bool CDevicePoolBase::CheckForIncommingData()
{
    pollfd fds[MaxDeviceInPool];

    int nfds = 0;
    for (int i = 0; i < MaxDeviceInPool; ++i)
    {
        if (m_Devices[i] &&
            m_Devices[i]->GetSocket())
        {
            socket_t socket = m_Devices[i]->GetSocket()->GetSocket();
            fds[nfds].fd = socket;
            fds[nfds].events = POLLIN;
            fds[nfds].revents = 0;
            nfds++;
        }
    }

    if (nfds == 0)
    {
        // no device in this pool, so let's sleep
        boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
        return true;
    }

    int res = poll(fds, nfds, 1000);
    if (res > 0)
    {
        // there is incomming data...
        for (int i = 0, n = 0; res > 0 && i < MaxDeviceInPool; ++i)
        {
            if (m_Devices[i] &&
                m_Devices[i]->GetSocket())
            {
                socket_t socket = m_Devices[i]->GetSocket()->GetSocket();
                if (socket == fds[n].fd)
                {
                    if (fds[n].revents & POLLIN)
                    {
                        if (!m_Devices[i]->OnIncomingData())
                        {
                            CDevice* pDevice = nullptr;
                            {
                                boost::mutex::scoped_lock lock(ms_Mutex);

                                pDevice = m_Devices[i];
                                m_Devices[i] = nullptr;
                                m_nNbDevices--;
                            }
                            if (pDevice)
                            {
                                LogWarn("WioT", "Closing device \"%s\" because of reception error or disconnection.", pDevice->GetDeviceId().c_str());

                                pDevice->Close();
                                DeviceManager.OnDisconnectedDevice(pDevice);
                                pDevice->Release();
                            }
                        }
                        res--;
                    }
                    n++;
                }
            }
        }
    }

    const time_t now = time(nullptr);
    for (int i = 0;i< MaxDeviceInPool; ++i)
    {
        CDevice* pDevice = nullptr;
        {
            boost::mutex::scoped_lock lock(ms_Mutex);

            pDevice = m_Devices[i];
        }
        if (pDevice)
        {
            if (!pDevice->GetSocket() ||
                !pDevice->OnIdle(now))
            {
                LogWarn("WioT", "Closing device \"%s\" because of too many time without incoming data.", pDevice->GetDeviceId().c_str());
                // device closed
                {
                    boost::mutex::scoped_lock lock(ms_Mutex);

                    m_Devices[i] = nullptr;
                    m_nNbDevices--;
                }
                DeviceManager.OnDisconnectedDevice(pDevice);
                pDevice->Release();
                continue;
            }
        }
    }

    return true;
}

bool CDevicePoolBase::CloseAll()
{
    boost::mutex::scoped_lock lock(m_Mutex);

    // find a free place
    for (int i = 0; i < MaxDeviceInPool; ++i)
    {
        if (m_Devices[i])
        {
            m_Devices[i]->Close();
            m_Devices[i]->Release();
            m_Devices[i] = nullptr;
        }
    }
    m_nNbDevices = 0;

    return true;
}

bool CDevicePoolBase::AddDevice(CDevice* pDevice)
{
    if (m_bWantStop || !pDevice)
    {
        return false;
    }

    boost::mutex::scoped_lock lock(m_Mutex);
    if (m_nNbDevices >= MaxDeviceInPool)
    {
        return false;
    }

    // find a free place
    for (int i = 0; i < MaxDeviceInPool; ++i)
    {
        if (!m_Devices[i])
        {
            pDevice->AddRef();
            m_Devices[i] = pDevice;
            m_nNbDevices++;
            pDevice->SetPoolIdx(GetPoolIdx());
            return true;
        }
    }
    return false;
}

int CDevicePoolBase::GetDeviceCount()
{
    boost::mutex::scoped_lock lock(m_Mutex);
    return m_nNbDevices;
}

bool CDevicePoolBase::CanAcceptAnotherDevice()
{
    if (m_bWantStop)
    {
        return false;
    }
    boost::mutex::scoped_lock lock(m_Mutex);
    return m_nNbDevices < (MaxDeviceInPool - 4);
}
