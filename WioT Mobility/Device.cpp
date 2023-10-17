#include "WioTMobility.h"
#include "Device.h"
#include "IProtocol.h"
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <algorithm>
#include "DeviceRequest.h"
#include "DeviceManager.h"


int CDevice::ms_DefaultRequestTimeout = 10000; // 10s
int CDevice::ms_NoReceptionTimeout = 300; // 5min
//int CDevice::ms_NoReceptionTimeout = 30; // 5min

CDevice::CDevice()
	: m_nPoolIdx(-1)
    , m_pSocket(nullptr)
    , m_pProtocol(nullptr)
    , m_pContext(nullptr)
    , m_nRequestTimeout{ ms_DefaultRequestTimeout }
    , m_nRefCount{1}
{
    m_dtLastReceptionTime = m_dtConnectionTime = time(nullptr);
    m_dtLastEmissionTime = 0;
}

CDevice::~CDevice()
{
    Close();
    if (m_pContext)
    {
        delete m_pContext;
    }
    if (m_CurrentRequest)
    {
        m_CurrentRequest->ProcessFailed("Device disconnected");
    }

    while (!m_msgQueue.empty())
    {
        CAutoPtr<CDeviceRequest> pRequest;
        pRequest = m_msgQueue.front();
        m_msgQueue.pop();
        pRequest->ProcessFailed("Device disconnected");
    }
}

CDevice* CDevice::New(CSocket* pSocket, const IProtocol* pProtocol)
{
	if (!pProtocol || !pSocket)
	{
		return nullptr;
	}

	CDevice* pDevice = new CDevice;
	if (!pDevice)
	{
		return nullptr;
	}
	pDevice->m_pSocket = pSocket;
    pDevice->m_pProtocol = pProtocol;
    pProtocol->Attach(pDevice);

	return pDevice;
}

bool CDevice::SetDeviceId(const std::string& DeviceId)
{
    if (m_DeviceId.length() > 0)
    {
        return false;
    }
    m_DeviceId = DeviceId;
    DeviceManager.OnSetDeviceId(this);
    return true;
}

bool CDevice::SetDeviceType(const std::string& DeviceType)
{
    if (m_DeviceType.length() > 0)
    {
        return false;
    }
    m_DeviceType = DeviceType;
    return true;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnonnull-compare"
int CDevice::AddRef()
{
	if (!this)
	{
		return 0;
	}
	return __sync_add_and_fetch(&m_nRefCount, 1);
}

bool CDevice::Release()
{
	if (this)
	{
		if (__sync_sub_and_fetch(&m_nRefCount, 1) > 0)
		{
			return true;
		}
		delete this;
	}
	return false;
}
#pragma GCC diagnostic pop

static void LogData(const CDevice *pDevice, const char* zTitle, const uint8_t* pData, ssize_t nLength)
{
    if (Sagitech::GetLogLevel() >= Sagitech::ELogLevel::Debug)
    {
        char txt[514];
        nLength = std::min(nLength, ssize_t(256));
        for (ssize_t i = 0; i < nLength; ++i)
        {
            sprintf(txt + i * 2, "%02x", pData[i]);
        }
        LogDebug("WioT", "[%s] %s : %s", pDevice->GetDeviceId().c_str(), zTitle, txt);
    }
}

bool CDevice::OnIncomingData()
{
    if (!m_pSocket)
    {
        return false;
    }

    if (m_Buffer.GetFreeCount() == 0)
    {
        LogWarn("WioT", "Device buffer is full. Discarding all unused data");
        m_Buffer.Consume();
    }

    int ret = static_cast<int>(m_pSocket->Receive(m_Buffer.Buffer() + m_Buffer.GetAvailableCount(), 
                                                  m_Buffer.GetFreeCount()));
    if (ret > 0)
    {
        m_dtLastReceptionTime = time(nullptr);
        m_Buffer.Appended(ret);

        LogData(this, "Incoming data", m_Buffer.Buffer(), ret);

        if (!m_pProtocol->Decode(this))
        {
            LogInf("WioT", "Protocol decode error");
            return false;
        }

        return true;
    }

    if (ret == 0)
    {
        LogInf("WioT", "Device %s close connection", m_DeviceId.empty() ? "<unknow>" : m_DeviceId.c_str());
    }
    else
    {
        const int err = m_pSocket->GetErrorCode(ret);
        const std::string zErr = m_pSocket->GetErrorString(err);
        LogErr("WioT", "Error while receiving data from device %s.\nError %d : %s\nClosing connection...",
            m_DeviceId.empty() ? "<unknow>" : m_DeviceId.c_str(), 
            err, zErr.c_str());
    }

    return false;
}

bool CDevice::OnIdle(const time_t now)
{
    CAutoPtr<CDeviceRequest> pCurrent;
    {
        boost::mutex::scoped_lock lock(m_Mutex);
        if (m_CurrentRequest.Empty() &&
            !m_msgQueue.empty())
        {
            pCurrent = m_msgQueue.front();
            m_msgQueue.pop();
        }
    }
    if (!pCurrent.Empty() &&
        pCurrent->ProcessRequest())
    {
        boost::mutex::scoped_lock lock(m_Mutex);
        m_CurrentRequest = pCurrent;
        m_CurrentRequest->SetProcessTime();
    }


    CAutoPtr<CDeviceRequest> pToCancel;
    {
        boost::mutex::scoped_lock lock(m_Mutex);
        if (!m_CurrentRequest.Empty() &&
            m_CurrentRequest->Timedout())
        {
            pToCancel = m_CurrentRequest;
            m_CurrentRequest = nullptr;
        }
    }

    if (!pToCancel.Empty())
    {
        LogErr("WioT", "Device %s : Request timeout.", GetDeviceId().c_str());
        pToCancel->ProcessFailed("Timeout");
    }

    if (now - m_dtLastReceptionTime > ms_NoReceptionTimeout)
    {
        LogErr("WioT", "Device %s : Too long without receiving any data. Disconnecting...", GetDeviceId().c_str());
        return false;
    }

    return true;
}

bool CDevice::Send(const void* pMessage, const size_t nLength)
{
    if (!m_pSocket)
    {
        return false;
    }

    LogData(this, "Outcoming data", reinterpret_cast<const uint8_t*>(pMessage), nLength);

    size_t ret=m_pSocket->Send(pMessage, nLength);
    if (ret != nLength)
    {
        return false;
    }
    m_dtLastEmissionTime = time(nullptr);

	return true;
}

bool CDevice::Close()
{
    if (m_pProtocol)
    {
        m_pProtocol->Detach(this);
        m_pProtocol = nullptr;
    }

    if (m_pSocket)
    {
        m_pSocket->Close();
        delete m_pSocket;
        m_pSocket = nullptr;
    }

    LogInf("WioT", "Device \"%s\" closed.", m_DeviceId.c_str());

    return true;
}

bool CDevice::PushRequest(CDeviceRequest* pRequest)
{
    if (!pRequest)
    {
        return false;
    }

    boost::mutex::scoped_lock lock(m_Mutex);
    m_msgQueue.emplace(pRequest);

    return true;
}

bool CDevice::CancelRequest(CDeviceRequest* pRequest)
{
    if (!pRequest)
    {
        return false;
    }

    boost::mutex::scoped_lock lock(m_Mutex);

    m_msgQueue.remove(pRequest);

    return false;
}

bool CDevice::OnRequestEnded()
{
    boost::mutex::scoped_lock lock(m_Mutex);
    m_CurrentRequest = nullptr;
    return true;
}

CAutoPtr<CDeviceRequest> CDevice::GetCurrentRequest()
{
    boost::mutex::scoped_lock lock(m_Mutex);
    return m_CurrentRequest;
}

void CDevice::SetRequestTimeout(int nRequestTimeout)
{
    m_nRequestTimeout = nRequestTimeout;
}

std::string CDevice::GetAddressIP() const
{
    std::string ip;
    if (m_pSocket)
    {
        ip = m_pSocket->GetAddressIP();
    }
    return ip;
}
