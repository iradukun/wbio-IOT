#pragma once
#include "Buffer.h"
#include "Socket.h"
#include "AutoPtr.h"
#include <queue>
#include "boost/thread/mutex.hpp"

class IProtocol;

class CProtocolContext
{
public:
	virtual ~CProtocolContext() {}
};
constexpr int BufferLength = 8192;

typedef CBuffer<BufferLength> CDeviceBuffer;

class CDeviceRequest;

template<typename T>
class custom_queue : public std::queue<T>
{
public:

	bool remove(const T& value)
	{
		auto it = std::find(this->c.begin(), this->c.end(), value);
		if (it != this->c.end())
		{
			this->c.erase(it);
			return true;
		}
		return false;
	}
};


class CDevice
{
private:
	CDevice();
	~CDevice();

public:
	static CDevice* New(CSocket *pSocket, const IProtocol* pProtocol);

	bool SetDeviceId(const std::string& DeviceId);
	const std::string& GetDeviceId() const
	{
		return m_DeviceId;
	}

	bool SetDeviceType(const std::string& DeviceType);
	const std::string& GetDeviceType() const
	{
		return m_DeviceType;
	}

	int AddRef();
	bool Release();

	bool OnIncomingData();
	bool OnIdle(const time_t now);

	bool Send(const void* pMessage, const size_t nLength);

	bool Close();

	CSocket * GetSocket() const
	{
		return m_pSocket;
	}

	CDeviceBuffer& GetBuffer()
	{
		return m_Buffer;
	}
	
	CProtocolContext* GetContext() const
	{
		return m_pContext;
	}
	
	const IProtocol* GetProtocol() const
	{
		return m_pProtocol;
	}
	
	void SetContext(CProtocolContext*pContext)
	{
		m_pContext = pContext;
	}

	bool PushRequest(CDeviceRequest* pRequest);
	bool CancelRequest(CDeviceRequest* pRequest);
	bool OnRequestEnded();

	CAutoPtr<CDeviceRequest> GetCurrentRequest();

	void SetRequestTimeout(int nRequestTimeout);
	int GetRequestTimeout() const
	{
		return m_nRequestTimeout;
	}

	std::string GetAddressIP() const;

	time_t GetConnectionTime() const
	{
		return m_dtConnectionTime;
	}

	time_t GetLastEmissionTime() const
	{
		return m_dtLastEmissionTime;
	}

	time_t GetLastReceptionTime() const
	{
		return m_dtLastReceptionTime;
	}

	static int ms_NoReceptionTimeout;
	static int ms_DefaultRequestTimeout;

	void SetPoolIdx(int nPoolIdx)
	{
		m_nPoolIdx = nPoolIdx;
	}
	int GetPoolIdx() const
	{
		return m_nPoolIdx;
	}
private:
	std::string m_DeviceId;
	std::string m_DeviceType;
	int m_nPoolIdx; // for information
	CSocket* m_pSocket;
	const IProtocol* m_pProtocol;
	CProtocolContext* m_pContext;
	CDeviceBuffer m_Buffer;
	int m_nRequestTimeout;
	time_t m_dtConnectionTime;
	time_t m_dtLastReceptionTime;
	time_t m_dtLastEmissionTime;

	boost::mutex m_Mutex;
	custom_queue< CAutoPtr<CDeviceRequest> > m_msgQueue;
	CAutoPtr<CDeviceRequest> m_CurrentRequest;

	volatile int m_nRefCount{ 0 };
};

