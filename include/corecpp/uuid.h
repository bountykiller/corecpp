#ifndef CORECPP_UUID_H
#define CORECPP_UUID_H

#include <inttypes.h>
#include <array>

namespace corecpp
{

class uuid final
{
	/* TODO:
	 * implement it using a __i128 for platforms that supports it
	 */
	union
	{
		std::array<uint8_t, 16> byte_array;
		std::array<uint16_t, 8> word_array;
		std::array<uint32_t, 4> int_array;
		std::array<uint64_t, 2> long_array;
		unsigned __int128 long_128;
	} m_data;
public:
	uuid(const std::array<uint8_t, 16> data) noexcept : m_data { data }
	{}
	uuid(const uuid& other) noexcept : m_data(other.m_data)
	{}
	uuid& operator= (const uuid& other) noexcept
	{
		m_data = other.m_data;
		return *this;
	}
	bool is_nil() const noexcept
	{
		return m_data.long_128 == 0;
	}
};

}

#endif
