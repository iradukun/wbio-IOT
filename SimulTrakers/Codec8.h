#pragma once

#include <cstdint>
#include <string>

enum class ECommand
{
	Invalid = 0x00,
	DataCommand=0x01,
	Configuration=0x02,
	Services=0x03,
	SystemControl=0x04,
	FirmwareUpdate=0x7e,
	NegativeResponse=0x7f,
	FactoryTest=0xfe,
};

class CCodec8
{
public:
	CCodec8(const char *zDeviceId, SOCKET socket);
	~CCodec8();

	bool Start();
	bool Stop();

	const uint8_t* GetFrame() const;
	unsigned GetFrameSize() const;

	bool SendPosition(const double fLongitude, const double fLattitude, const double fAltitude, const int nPriority, const double fSpeed);
	bool 

private:

	template<class T>
	void Push(T data);

	void Raz();
	bool Request(int nResultSize = 4);

	static uint16_t Crc16(const uint8_t* buffer, int len);

	uint8_t m_pFrame[4096];
	int m_nFrameSize;
	SOCKET m_Socket;
	std::string m_zDeviceId;
};
