#include "WioTMobility.h"
#include "DeviceManager.h"
#include "DevicePoolBase.h"
#include "Device.h"
#include "AutoPtr.h"
#include <fcntl.h>
#include <algorithm>

CDeviceManager::CDeviceManager()
	: m_lPoolIdx(0)
{
}

CDeviceManager::~CDeviceManager()
{
	boost::mutex::scoped_lock lock(m_Mutex);

	for (auto pDevice : m_Devices)
	{
		pDevice->Release();
	}
}

bool CDeviceManager::CloseAll()
{
	{
		boost::mutex::scoped_lock lock(m_Mutex);

		for (CDevicePoolBase& pool : m_Pools)
		{
			pool.Stop();
		}
	}

	CDevicePoolBase::WaitForAllPoolStopped();

	boost::mutex::scoped_lock lock(m_Mutex);

	m_Pools.clear();

	return true;
}

bool CDeviceManager::OnDisconnectedDevice(CDevice* pDevice)
{
	if (pDevice)
	{
		boost::mutex::scoped_lock lock(m_Mutex);

		for (auto iter = m_Devices.begin(); iter != m_Devices.end(); ++iter)
		{
			if (*iter == pDevice)
			{
				LogInf("WioT", "[DEVMAN] Device \"%s\" was dropped from the list of active devices.",
					pDevice->GetDeviceId().c_str());
				m_Devices.erase(iter);
				pDevice->Release();
				return true;
			}
		}
	}
	LogErr("WioT", "[DEVMAN] Device \"%s\" was not found in the list of active devices.",
		pDevice->GetDeviceId().c_str());
	return false;
}

int CDeviceManager::CurrentDeviceCount()
{
	boost::mutex::scoped_lock lock(m_Mutex);
	return static_cast<int>(m_Devices.size());
}

CDevice* CDeviceManager::GetDeviceById(const char *zIMEI)
{
	if (zIMEI)
	{
		boost::mutex::scoped_lock lock(m_Mutex);

		auto finder = [zIMEI](CDevice* pDevice) { return pDevice->GetDeviceId().compare(zIMEI) == 0; };
		auto iter = std::find_if(m_Devices.begin(), m_Devices.end(), finder);
		if (iter != m_Devices.end())
		{
			CDevice* pDevice = *iter;
			pDevice->AddRef();
			return pDevice;
		}
	}
	return nullptr;
}

bool CDeviceManager::EnumerateDevice(IDeviceSink* pSink)
{
	if (!pSink)
	{
		return false;
	}

	boost::mutex::scoped_lock lock(m_Mutex);

	for (auto device : m_Devices)
	{
		if (!pSink->OnDevice(device))
		{
			return true;
		}
	}
	return true;
}

bool CDeviceManager::OnSetDeviceId(CDevice* pDevice)
{
	if (!pDevice || pDevice->GetDeviceId().empty())
	{
		return false;
	}

	const std::string& zDeviceId = pDevice->GetDeviceId();
	if (zDeviceId.empty())
	{
		return false;
	}

	boost::mutex::scoped_lock lock(m_Mutex);

	for (auto iter=m_Devices.begin();iter!=m_Devices.end();)
	{
		CDevice* device = *iter;
		if ((device != pDevice) && (zDeviceId == device->GetDeviceId()))
		{
			LogWarn("WioT", "[DEVMAN] Device \"%s\" already in the list. Closing the previous instance.", zDeviceId.c_str());
			pDevice->Close();
			iter = m_Devices.erase(iter);
			device->Release();
		}
		else
		{
			iter++;
		}
	}
	return true;
}

bool CDeviceManager::AddDevice(CSocket *pSocket, const IProtocol* pProtocol)
{
	// First : pass the socket to non blocking
	// Should be at another place but don't want to make to much operation under mutex protection
	// 
	if (!pSocket->SetNonBlocking())
	{
		LogWarn("WioT", "[DEVMAN] Could not set non blocking flag on socket.");
	}

	bool bNewPool = false;
	CDevicePoolBase* pSelectedPool = nullptr;
	{
		boost::mutex::scoped_lock lock(m_Mutex);

		for (CDevicePoolBase& pool : m_Pools)
		{
			if (pool.CanAcceptAnotherDevice())
			{
				if (!pSelectedPool || pSelectedPool->GetDeviceCount() > pool.GetDeviceCount())
				{
					pSelectedPool = &pool;
				}
			}
		}
		if (!pSelectedPool)
		{
			m_Pools.emplace_back(++m_lPoolIdx);
			pSelectedPool = &m_Pools.back();
			bNewPool = true;
		}
	}

	if (bNewPool)
	{
		pSelectedPool->Start();
	}

	CAutoPtr<CDevice> pDevice(CDevice::New(pSocket, pProtocol), false);
	if (!pSelectedPool->AddDevice(pDevice))
	{
		return false;
	}
	
	LogInf("WioT", "[DEVMAN] New device added to device pool %d", pSelectedPool->GetPoolIdx());

	boost::mutex::scoped_lock lock(m_Mutex);
	m_Devices.push_back(pDevice);
	pDevice.Detach();

	return true;
}

CDeviceManager& CDeviceManager::GetInstance()
{
	static CDeviceManager s_Instance;
	return s_Instance;
}

int CDeviceManager::GetActivePoolCount() const
{
	boost::mutex::scoped_lock lock(m_Mutex);
	return static_cast<int>(m_Pools.size());
}
