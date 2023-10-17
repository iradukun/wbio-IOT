#include <cstring>
#include <sstream>
#define VALUE_INSTANCIATION
#include "Value.h"

template<> uint8_t CValue::GetValue<uint8_t>() const
{
	if (m_eType != EType::UInt8)
	{
		throw std::runtime_error("Wrong type");
	}
	return m_Value.u8;
}
template<> uint16_t CValue::GetValue<uint16_t>() const
{
	if (m_eType != EType::UInt16)
	{
		throw std::runtime_error("Wrong type");
	}
	return m_Value.u16;
}
template<> uint32_t CValue::GetValue<uint32_t>() const
{
	if (m_eType != EType::UInt32)
	{
		throw std::runtime_error("Wrong type");
	}
	return m_Value.u32;
}
template<> uint64_t CValue::GetValue<uint64_t>() const
{
	if (m_eType != EType::UInt64)
	{
		throw std::runtime_error("Wrong type");
	}
	return m_Value.u64;
}
template<> int8_t CValue::GetValue<int8_t>() const
{
	if (m_eType != EType::Int8)
	{
		throw std::runtime_error("Wrong type");
	}
	return m_Value.s8;
}
template<> int16_t CValue::GetValue<int16_t>() const
{
	if (m_eType != EType::Int16)
	{
		throw std::runtime_error("Wrong type");
	}
	return m_Value.s16;
}
template<> int32_t CValue::GetValue<int32_t>() const
{
	if (m_eType != EType::Int32)
	{
		throw std::runtime_error("Wrong type");
	}
	return m_Value.s32;
}
template<> int64_t CValue::GetValue<int64_t>() const
{
	if (m_eType != EType::Int64)
	{
		throw std::runtime_error("Wrong type");
	}
	return m_Value.s64;
}
template<> float CValue::GetValue<float>() const
{
	if (m_eType != EType::Flt32)
	{
		throw std::runtime_error("Wrong type");
	}
	return m_Value.f32;
}
template<> double CValue::GetValue<double>() const
{
	if (m_eType != EType::Flt64)
	{
		throw std::runtime_error("Wrong type");
	}
	return m_Value.f64;
}
template<> const char *CValue::GetValue<const char *>() const
{
	if (m_eType != EType::String)
	{
		throw std::runtime_error("Wrong type");
	}
	return m_Value.str;
}

CValue::CValue()
	: m_eType{EType::Unknow}
{
}

CValue::CValue(const CValue& other)
	: m_eType{ EType::Unknow }
{
	*this = other;
}

CValue& CValue::operator=(const CValue& other)
{
	Free();
	m_eType = other.m_eType;
	if (m_eType == EType::String)
	{
		const size_t len = strlen(other.m_Value.str);
		m_Value.str = new char[len + 1];
		strcpy(m_Value.str, other.m_Value.str);
	}
	else
	{
		m_Value.u64 = other.m_Value.u64;
	}
	return *this;
}

CValue::~CValue()
{
	Free();
}

std::string CValue::ToString() const
{
	std::ostringstream oss;

	switch (m_eType)
	{
	case EType::UInt8:
		oss << (int)m_Value.u8;
		break;
	case EType::UInt16:
		oss << m_Value.u16;
		break;
	case EType::UInt32:
		oss << m_Value.u32;
		break;
	case EType::UInt64:
		oss << m_Value.u64;
		break;
	case EType::Int8:
		oss << (int)m_Value.s8;
		break;
	case EType::Int16:
		oss << m_Value.s16;
		break;
	case EType::Int32:
		oss << m_Value.s32;
		break;
	case EType::Int64:
		oss << m_Value.s64;
		break;
	case EType::Flt32:
		oss << m_Value.f32;
		break;
	case EType::Flt64:
		oss << m_Value.f64;
		break;
	case EType::String:
		oss << m_Value.str;
		break;
	case EType::Unknow:
	default:
		oss << "--UNKNOW--";
		break;
	}

	return oss.str();
}

void CValue::Free()
{
	if (m_eType == EType::String)
	{
		delete[] m_Value.str;
	}
	m_eType = EType::Unknow;
	m_Value.u64 = 0;
}

CValue::CValue(uint8_t val)
	: m_eType{ EType::UInt8 }
{
	m_Value.u8 = val;
}

CValue::CValue(uint16_t val)
	: m_eType{ EType::UInt16 }
{
	m_Value.u16 = val;
}

CValue::CValue(uint32_t val)
	: m_eType{ EType::UInt32 }
{
	m_Value.u32 = val;
}

CValue::CValue(uint64_t val)
	: m_eType{ EType::UInt64 }
{
	m_Value.u64 = val;
}

CValue::CValue(int8_t val)
	: m_eType{ EType::Int8 }
{
	m_Value.s8 = val;
}

CValue::CValue(int16_t val)
	: m_eType{ EType::Int16 }
{
	m_Value.s16 = val;
}

CValue::CValue(int32_t val)
	: m_eType{ EType::Int32 }
{
	m_Value.s32 = val;
}

CValue::CValue(int64_t val)
	: m_eType{ EType::Int64 }
{
	m_Value.s64 = val;
}

CValue::CValue(float val)
	: m_eType{ EType::Flt32 }
{
	m_Value.f32 = val;
}

CValue::CValue(double val)
	: m_eType{ EType::Flt64 }
{
	m_Value.f64 = val;
}

CValue::CValue(const char* val, size_t len)
	: m_eType{ EType::String }
{
	if (len < 0)
	{
		len = strlen(val);
	}
	else
	{
		const size_t __len = val ? strlen(val) : 0;
		if (len > __len)
		{
			len = __len;
		}
	}
	m_Value.str = new char[len+1];
	if (val)
	{
		strcpy(m_Value.str, val);
	}
	m_Value.str[len] = '\0';
}

CValue::CValue(const std::string& val)
	: m_eType{ EType::String }
{
	const size_t len = val.length();
	m_Value.str = new char[len + 1];
	strcpy(m_Value.str, val.c_str());
}
