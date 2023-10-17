#pragma once
#include <cstring>
#include <stdexcept>

template<int SIZE>
class CBuffer
{
public:
	CBuffer()
		: m_nAvailable{0}
	{}
	~CBuffer()
	{}

	int GetTotalLength() const
	{
		return SIZE;
	}

	int GetAvailableCount() const
	{
		return m_nAvailable;
	}

	int GetFreeCount() const
	{
		return SIZE - m_nAvailable;
	}

	const uint8_t* Buffer() const
	{
		return m_Buffer;
	}

	uint8_t* Buffer()
	{
		return m_Buffer;
	}

	bool Appended(int nLength)
	{
		if (nLength<0 || m_nAvailable + nLength > SIZE)
		{
			return false;
		}
		m_nAvailable += nLength;
		return true;
	}

	bool Append(const void* pData, int nLength)
	{
		if (!pData || nLength<0 || m_nAvailable + nLength > SIZE)
		{
			return false;
		}
		memcpy(m_Buffer + m_nAvailable, pData, nLength);
		m_nAvailable += nLength;
		return true;
	}

	void Consume(int nLength = -1)
	{
		if (nLength < 0 || nLength >= m_nAvailable)
		{
			m_nAvailable = 0;
			return;
		}
		if (nLength > 0)
		{
			void* pDst = m_Buffer;
			const void* pSrc = m_Buffer + nLength;
			const int nTotalLength = (m_nAvailable - nLength) * sizeof(uint8_t);
			std::memmove(pDst, pSrc, nTotalLength);
			m_nAvailable -= nLength;
		}
	}

	void ReadData(void *pDst, int nCount, int &nOffset) const
	{
		if (static_cast<int>(nOffset + nCount) > m_nAvailable)
		{
			throw std::overflow_error("Not enough data in buffer");
		}
		memcpy(pDst, m_Buffer + nOffset, nCount);
		nOffset += nCount;
	}

	template<typename T>
	T ReadValue(int &nOffset) const
	{
		if (static_cast<int>(nOffset + sizeof(T)) > m_nAvailable)
		{
			throw std::overflow_error("Not enough data in buffer");
		}
		T value=reinterpret_cast<const T*>(m_Buffer + nOffset)[0];
		nOffset += static_cast<int>(sizeof(T));
		return value;
	}

	template<typename T>
	T GetValue(int nOffset) const
	{
		return ReadValue<T>(nOffset);
	}

	template<typename T>
	T WriteValue(T value, int &nOffset)
	{
		if (static_cast<int>(nOffset + sizeof(T)) > m_nAvailable)
		{
			throw std::overflow_error("Not enough space in buffer");
		}
		reinterpret_cast<T*>(m_Buffer + nOffset)[0] = value;
		nOffset += static_cast<int>(sizeof(T));
		return value;
	}

	template<typename T>
	T SetValue(T value, int nOffset)
	{
		return WriteValue<T>(value, nOffset);
	}

	template<typename T>
	int AppendValue(T value)
	{
		if (static_cast<int>(m_nAvailable + sizeof(T)) > SIZE)
		{
			throw std::overflow_error("Not enough space in buffer");
		}
		reinterpret_cast<T*>(m_Buffer + m_nAvailable)[0] = value;
		m_nAvailable += static_cast<int>(sizeof(T));
		return m_nAvailable;
	}

	int AppendData(const void *pBuffer, int nCount)
	{
		if (static_cast<int>(m_nAvailable + nCount) > SIZE)
		{
			throw std::overflow_error("Not enough space in buffer");
		}
		memcpy(m_Buffer + m_nAvailable, pBuffer, nCount);
		m_nAvailable += nCount;
		return m_nAvailable;
	}

private:
	uint8_t m_Buffer[SIZE];
	int m_nAvailable;
};

