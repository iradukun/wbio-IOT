#include <cstdlib>
#include <cstdio>
#include <ctime>
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include "Codec8Extended.h"

typedef uint16_t IOcount_t;

static __inline uint8_t NtoH(const uint8_t val)
{
	return val;
}

static __inline uint16_t NtoH(const uint16_t val)
{
	return ntohs(val);
}

static __inline uint32_t NtoH(const uint32_t val)
{
	return ntohl(val);
}

static __inline uint64_t NtoH(const uint64_t val)
{
	const uint32_t low = static_cast<uint32_t>(val & 0xffffffff);
	const uint32_t high = static_cast<uint32_t>((val >> 32) & 0xffffffff);
	return static_cast<uint64_t>(ntohl(low)) << 32 | static_cast<uint64_t>(ntohl(high));
}

CCodec8Extended::CCodec8Extended(const char* zDeviceId, SOCKET socket)
	: m_nFrameSize(0)
	, m_Socket(socket)
	, m_zDeviceId(zDeviceId)
{
}

CCodec8Extended::~CCodec8Extended()
{
}

template<typename T>
inline void CCodec8Extended::Push(T data)
{
	reinterpret_cast<T*>(m_pFrame + m_nFrameSize)[0] = NtoH(data);
	m_nFrameSize += sizeof(T);
}

template<>
inline void CCodec8Extended::Push(std::string data)
{
	strcpy(reinterpret_cast<char *>(m_pFrame + m_nFrameSize), data.c_str());
	m_nFrameSize += (int)data.length();
}

bool CCodec8Extended::Start()
{
	Raz();
	const uint16_t header = (uint16_t)m_zDeviceId.length();
	Push<uint16_t>(header);
	Push(m_zDeviceId);
	return Request(1);
}

bool CCodec8Extended::Stop()
{
	// shutdown the connection since no more data will be sent
	const int iResult = shutdown(m_Socket, SD_BOTH);
	if (iResult == SOCKET_ERROR)
	{
		printf("shutdown failed with error: %d\n", WSAGetLastError());
	}

	// cleanup
	closesocket(m_Socket);
	m_Socket = INVALID_SOCKET;
	return true;
}


const uint8_t* CCodec8Extended::GetFrame() const
{
	return m_pFrame;
}

unsigned CCodec8Extended::GetFrameSize() const
{
	return m_nFrameSize;
}

bool CCodec8Extended::SendPosition(const double fLongitude, const double fLattitude, const double fAltitude, const int nPriority, const double fSpeed)
{
	Raz();
	Push<uint32_t>(0); // Preamble
	Push<uint32_t>(0); // Data Field Length
	Push<uint8_t>(0x8E); // Codec ID
	Push<uint8_t>(1); // Nb of data 1

	time_t now = time(NULL);
	Push<uint64_t>(now * 1000);	// Timestamp
	Push<uint8_t>(nPriority);	// Priority
	// GPS data
	Push<uint32_t>(static_cast<uint32_t>(fLongitude * 10000000.));	
	Push<uint32_t>(static_cast<uint32_t>(fLattitude * 10000000.));
	Push<uint16_t>(static_cast<uint16_t>(fAltitude));
	Push<uint16_t>(static_cast<uint16_t>(0));
	Push<uint8_t>(static_cast<uint8_t>(1));
	Push<uint16_t>(static_cast<uint16_t>(fSpeed));

	// EventIO
	Push<IOcount_t>(1);	// Event IO ID
	Push<IOcount_t>(6);	// N of Total IO
	Push<IOcount_t>(2);	// N1 of One Byte IO
	Push<IOcount_t>(15);
	Push<uint8_t>(3);
	Push<IOcount_t>(1);
	Push<uint8_t>(1);
	Push<IOcount_t>(1);	// N2 of Two Byte IO
	Push<IOcount_t>(42);
	Push<uint16_t>(0x5e0f);
	Push<IOcount_t>(1);	// N4 of Four Byte IO
	Push<IOcount_t>(0xf1);
	Push<uint32_t>(0x601a);
	Push<IOcount_t>(1);	// N8 of Height Byte IO
	Push<IOcount_t>(0x4e);	// N4 of Four Byte IO
	Push<uint64_t>(0x9876543210);	// N4 of Four Byte IO

	Push<IOcount_t>(1);	// Nx of x Byte IO
	Push<IOcount_t>(0x4e);	// N4 of Four Byte IO
	Push<IOcount_t>(17);	// N4 of Four Byte IO
	Push<uint8_t>(1);	// N4 of Four Byte IO
	Push<uint8_t>(2);	// N4 of Four Byte IO
	Push<uint8_t>(3);	// N4 of Four Byte IO
	Push<uint8_t>(4);	// N4 of Four Byte IO
	Push<uint8_t>(5);	// N4 of Four Byte IO
	Push<uint8_t>(6);	// N4 of Four Byte IO
	Push<uint8_t>(7);	// N4 of Four Byte IO
	Push<uint8_t>(8);	// N4 of Four Byte IO
	Push<uint8_t>(9);	// N4 of Four Byte IO
	Push<uint8_t>(10);	// N4 of Four Byte IO
	Push<uint8_t>(11);	// N4 of Four Byte IO
	Push<uint8_t>(12);	// N4 of Four Byte IO
	Push<uint8_t>(13);	// N4 of Four Byte IO
	Push<uint8_t>(14);	// N4 of Four Byte IO
	Push<uint8_t>(15);	// N4 of Four Byte IO
	Push<uint8_t>(16);	// N4 of Four Byte IO
	Push<uint8_t>(17);	// N4 of Four Byte IO

	Push<uint8_t>(1); // Nb of data 2

	reinterpret_cast<uint32_t*>(m_pFrame)[1] = NtoH(static_cast<uint32_t>(m_nFrameSize - 8));
	Push<uint32_t>(Crc16(m_pFrame+8, m_nFrameSize - 8)); // CRC

	return Request();
}


void CCodec8Extended::Raz()
{
	m_nFrameSize = 0;
}

bool CCodec8Extended::Request(int nResultSize)
{
	// Send an initial buffer
	int iResult = send(m_Socket, reinterpret_cast<const char*>(m_pFrame), m_nFrameSize, 0);
	if (iResult == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(m_Socket);
		WSACleanup();
		return 1;
	}

//	printf("Bytes Sent: %ld\n", iResult);

	// Receive until the peer closes the connection
	do
	{
		char recvbuf[64];
		iResult = recv(m_Socket, recvbuf, nResultSize, 0);
		if (iResult > 0)
		{
		//	printf("Bytes received: %d\n", iResult);
		}
		else if (iResult == 0)
		{
			printf("Connection closed\n");
		}
		else
		{
			printf("recv failed with error: %d\n", WSAGetLastError());
		}

	} while (false);

	return true;
}

uint16_t CCodec8Extended::Crc16(const uint8_t* buffer, int len)
{
#if 1
	unsigned CRC = 0;

	for (int i = 0; i < len; ++i)
	{
		CRC ^= buffer[i];

		for (int j = 0; j < 8; ++j)
		{
			bool Carry = (CRC & 1) != 0;
			CRC >>= 1;
			if (Carry)
			{
				CRC ^= 0xa001;
			}
}
	}

	return static_cast<uint16_t>(CRC);
#else
	static const uint16_t table[] = {0, 4129, 8258, 12387, 16516, 20645, 24774, 28903, 33032, 37161, 41290, 45419, 49548, 53677, 57806, 61935, 4657, 528, 12915, 8786, 21173, 17044, 29431, 25302, 37689, 33560, 45947, 41818, 54205, 50076, 62463, 58334, 9314, 13379, 1056, 5121, 25830, 29895, 17572, 21637, 42346, 46411, 34088, 38153, 58862, 62927, 50604, 54669, 13907, 9842, 5649, 1584, 30423, 26358, 22165, 18100, 46939, 42874, 38681, 34616, 63455, 59390, 55197, 51132, 18628, 22757, 26758, 30887, 2112, 6241, 10242, 14371, 51660, 55789, 59790, 63919, 35144, 39273, 43274, 47403, 23285, 19156, 31415, 27286, 6769, 2640, 14899, 10770, 56317, 52188, 64447, 60318, 39801, 35672, 47931, 43802, 27814, 31879, 19684, 23749, 11298, 15363, 3168, 7233, 60846, 64911, 52716, 56781, 44330, 48395, 36200, 40265, 32407, 28342, 24277, 20212, 15891, 11826, 7761, 3696, 65439, 61374, 57309, 53244, 48923, 44858, 40793, 36728, 37256, 33193, 45514, 41451, 53516, 49453, 61774, 57711, 4224, 161, 12482, 8419, 20484, 16421, 28742, 24679, 33721, 37784, 41979, 46042, 49981, 54044, 58239, 62302, 689, 4752, 8947, 13010, 16949, 21012, 25207, 29270, 46570, 42443, 38312, 34185, 62830, 58703, 54572, 50445, 13538, 9411, 5280, 1153, 29798, 25671, 21540, 17413, 42971, 47098, 34713, 38840, 59231, 63358, 50973, 55100, 9939, 14066, 1681, 5808, 26199, 30326, 17941, 22068, 55628, 51565, 63758, 59695, 39368, 35305, 47498, 43435, 22596, 18533, 30726, 26663, 6336, 2273, 14466, 10403, 52093, 56156, 60223, 64286, 35833, 39896, 43963, 48026, 19061, 23124, 27191, 31254, 2801, 6864, 10931, 14994, 64814, 60687, 56684, 52557, 48554, 44427, 40424, 36297, 31782, 27655, 23652, 19525, 15522, 11395, 7392, 3265, 61215, 65342, 53085, 57212, 44955, 49082, 36825, 40952, 28183, 32310, 20053, 24180, 11923, 16050, 3793, 7920};
	uint16_t crc16 = 0;
	for (int i=0;i<len;++i)
	{
		crc16 = table[(crc16 >> 8 ^ buffer[i]) & 255] ^ crc16 << 8;
	}
	return crc16;
#endif
}
