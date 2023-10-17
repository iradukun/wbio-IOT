#pragma once
#include <list>
#include <unordered_map>
#include <string>
#include "DevicePoolBase.h"
#include "boost/thread/mutex.hpp"
#include "Socket.h"

typedef int LServerIdx;

class IDeviceSink
{
public:
	virtual bool OnDevice(CDevice* pDevice) = 0;
};

class CDevicePoolBase;
class CDeviceManager
{
private:
	CDeviceManager();
	~CDeviceManager();

public:
	bool AddDevice(CSocket *pSocket, const IProtocol *pProtocol);

	bool CloseAll();

	bool OnDisconnectedDevice(CDevice* pDevice);

	int CurrentDeviceCount();

	CDevice* GetDeviceById(const char *zIMEI);

	bool EnumerateDevice(IDeviceSink *pSink);

	bool OnSetDeviceId(CDevice* pDevice);

	static CDeviceManager& GetInstance();

	int GetActivePoolCount() const;
private:
	mutable boost::mutex m_Mutex;
	std::list<CDevicePoolBase> m_Pools;
	std::list<CDevice*> m_Devices;
	LPoolIdx m_lPoolIdx;
};

#define DeviceManager CDeviceManager::GetInstance()
