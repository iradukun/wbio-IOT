#pragma once
#include "LibLog.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <arpa/inet.h>

typedef int socket_t;
constexpr socket_t INVALID_SOCKET = -1;

constexpr int WaitThreadTimeout = 60; // 1min

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
	const uint32_t high = static_cast<uint32_t>((val>>32) & 0xffffffff);
	return static_cast<uint64_t>(ntohl(low)) << 32 | static_cast<uint64_t>(ntohl(high));
}

static __inline uint8_t HtoN(const uint8_t val)
{
	return val;
}

static __inline uint16_t HtoN(const uint16_t val)
{
	return htons(val);
}

static __inline uint32_t HtoN(const uint32_t val)
{
	return htonl(val);
}

static __inline uint64_t HtoN(const uint64_t val)
{
	const uint32_t low = static_cast<uint32_t>(val & 0xffffffff);
	const uint32_t high = static_cast<uint32_t>((val>>32) & 0xffffffff);
	return static_cast<uint64_t>(htonl(low)) << 32 | static_cast<uint64_t>(htonl(high));
}

#define CountOf(a)		static_cast<int>(sizeof(a)/sizeof((a)[0]))


enum class ESecurity
{
	NoSSL,
	AllowSSL,
	UseSSLOnly,
};
