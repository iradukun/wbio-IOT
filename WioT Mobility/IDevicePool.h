#pragma once

class IProtocol;
class IDevicePool
{
public:
	virtual bool AddDevice(socket_t socket, const IProtocol *pProtocol) = 0;
	virtual int GetDeviceCount() = 0;
	virtual bool CanAcceptAnotherDevice() = 0;
};

