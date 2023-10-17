#pragma once
#include "Device.h"

class IProtocol;
class CDevice;
struct TDeviceFunction;

class IProtocol
{
public:
	virtual const char* GetName() const = 0;

	virtual bool Attach(CDevice* pDevice) const = 0;
	virtual bool Detach(CDevice* pDevice) const = 0;

	virtual bool Decode(CDevice* pDevice) const = 0;

	virtual const TDeviceFunction* GetDeviceFunctions(int& nNbFuntions) const = 0;
};

