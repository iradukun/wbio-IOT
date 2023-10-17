#pragma once
#include <cstdint>
#include <string>
#include <stdexcept>

class CValue
{
public:
	CValue();
	CValue(const CValue &other);
	CValue &operator=(const CValue& other);
	~CValue();

	CValue(uint8_t val);
	CValue(uint16_t val);
	CValue(uint32_t val);
	CValue(uint64_t val);
	CValue(int8_t val);
	CValue(int16_t val);
	CValue(int32_t val);
	CValue(int64_t val);
	CValue(float val);
	CValue(double val);
	CValue(const char* val, size_t len = -1);
	CValue(const std::string& val);

	enum class EType
	{
		Unknow,
		UInt8,
		UInt16,
		UInt32,
		UInt64,
		Int8,
		Int16,
		Int32,
		Int64,
		Flt32,
		Flt64,
		String,
	};
	EType GetType() const
	{
		return m_eType;
	}

	template<typename T>
	T GetValue() const;
/* {
		throw std::runtime_error("Wrong type");
	}*/

	std::string ToString() const;

private:
	EType m_eType;
	union
	{
		uint8_t u8;
		uint16_t u16;
		uint32_t u32;
		uint64_t u64;
		int8_t s8;
		int16_t s16;
		int32_t s32;
		int64_t s64;
		float f32;
		double f64;
		char* str;
	} m_Value;

	void Free();
};

#ifndef VALUE_INSTANCIATION
extern template uint8_t CValue::GetValue<uint8_t>() const;
extern template uint16_t CValue::GetValue<uint16_t>() const;
extern template uint32_t CValue::GetValue<uint32_t>() const;
extern template uint64_t CValue::GetValue<uint64_t>() const;
extern template int8_t CValue::GetValue<int8_t>() const;
extern template int16_t CValue::GetValue<int16_t>() const;
extern template int32_t CValue::GetValue<int32_t>() const;
extern template int64_t CValue::GetValue<int64_t>() const;
extern template float CValue::GetValue<float>() const;
extern template double CValue::GetValue<double>() const;
extern template const char *CValue::GetValue<const char *>() const;
#endif