#ifndef CORECPP_UUID_H
#define CORECPP_UUID_H

#include <inttypes.h>
#include <array>

#include <corecpp/algorithm.h>

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
	static const uuid nil;

	uuid(const std::array<uint8_t, 16>& data) noexcept : m_data { data }
	{}
	uuid(unsigned __int128 data) noexcept : m_data { .long_128 = data }
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
	template <typename SerializerT>
	void serialize(SerializerT& s) const
	{
		s.write_property("hi_value", m_data.long_array[1]);
		s.write_property("lo_value", m_data.long_array[0]);
	}
	template <typename DeserializerT>
	void deserialize(DeserializerT& d, const std::string& property)
	{
		if (property == "hi_value")
			d.deserialize(m_data.long_array[1]);
		else if (property == "lo_value")
			d.deserialize(m_data.long_array[0]);
		else
			throw std::runtime_error(corecpp::concat<std::string>({"invalid property ", property}));
	}
};

const uuid uuid::nil { 0 };


}

#endif
