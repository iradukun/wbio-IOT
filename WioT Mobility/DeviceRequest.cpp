#include "WioTMobility.h"
#include "DeviceRequest.h"
#include "Device.h"


constexpr double DefaultRequestTimeout = 60.;

CDeviceRequest::CDeviceRequest(CDevice* pDevice)
    : m_bSucceeded(false)
    , m_pDevice(pDevice)
    , m_dtProcessTime(0)
    , m_fTimeout(DefaultRequestTimeout)
    , m_nRefCount(1)
{
}

CDeviceRequest::~CDeviceRequest()
{
}

void CDeviceRequest::ProcessSucceeded(const std::string& zResponse)
{
    m_bSucceeded = true;
    m_zResponse = zResponse;

    {
        boost::mutex::scoped_lock lock(m_Mutex);
        m_RequestEnded.notify_one();
    }
    if (m_pDevice)
    {
        m_pDevice->OnRequestEnded();
    }
}

void CDeviceRequest::ProcessFailed(const std::string& zMsgError)
{
    m_bSucceeded = true;
    m_zResponse = zMsgError;

    {
        boost::mutex::scoped_lock lock(m_Mutex);
        m_RequestEnded.notify_one();
    }
    if (m_pDevice)
    {
        m_pDevice->OnRequestEnded();
    }
}

bool CDeviceRequest::WaitForResponse(int nTimeoutMs)
{
    boost::posix_time::milliseconds timeout(nTimeoutMs);

    boost::mutex::scoped_lock lock(m_Mutex);

    return m_RequestEnded.timed_wait(lock, timeout);
}

int CDeviceRequest::AddRef()
{
    boost::mutex::scoped_lock lock(m_Mutex);

    return ++m_nRefCount;
}

bool CDeviceRequest::Release()
{
    {
        boost::mutex::scoped_lock lock(m_Mutex);

        if (--m_nRefCount)
        {
            return true;
        }
    }
    delete this;
    return false;
}

void CDeviceRequest::SetProcessTime()
{
    m_dtProcessTime = time(nullptr);
}

bool CDeviceRequest::Timedout() const
{
    const time_t now = time(nullptr);

    return difftime(now, m_dtProcessTime) > m_fTimeout;
}
