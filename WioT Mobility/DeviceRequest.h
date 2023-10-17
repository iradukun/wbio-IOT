#pragma once
#include <string>
#include "boost/thread/mutex.hpp"
#include "boost/thread/condition.hpp"
#include "AutoPtr.h"
#include "Device.h"

class CDeviceRequest
{
public:
	CDeviceRequest(CDevice *pDevice);
	virtual ~CDeviceRequest();

	// return true if request failed or if request is finished (not an asynchrone request)
	virtual bool ProcessRequest() = 0;

	void ProcessSucceeded(const std::string& zResponse);
	void ProcessFailed(const std::string& zMsgError);

	bool WaitForResponse(int nTimeoutMs);

	int AddRef();
	bool Release();

	CDevice* GetDevice() const
	{
		return m_pDevice;
	}

	std::string GetResponse() const
	{
		return m_zResponse;
	}

	void SetProcessTime();
	bool Timedout() const;
private:
	CDeviceRequest() = delete;
	CDeviceRequest(const CDeviceRequest &) = delete;
	CDeviceRequest&operator=(const CDeviceRequest &) = delete;

	boost::mutex m_Mutex;
	boost::condition m_RequestEnded;
	std::string m_zResponse;
	bool m_bSucceeded;
	CAutoPtr<CDevice> m_pDevice;
	time_t m_dtProcessTime;
	double m_fTimeout;

	volatile int m_nRefCount;
};

