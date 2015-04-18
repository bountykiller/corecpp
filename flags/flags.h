#ifndef EXPERIMENTALPP_FLAGS_H
#define EXPERIMENTALPP_FLAGS_H

#include <type_traits>

namespace experimental
{

template<typename EnumT>
class flags
{
public:
	using underlying_type = typename std::underlying_type<EnumT>::type;
private:
	underlying_type m_value;
public:
	flags(const flags& value) = default;
	constexpr flags(EnumT e) : m_value (static_cast<underlying_type>(e))
	{}
	explicit constexpr flags(underlying_type e) : m_value(e)
	{}
	constexpr flags operator +(EnumT e) const { return flags( m_value + static_cast<underlying_type>(e)); }
	constexpr flags operator -(EnumT e) const { return flags( m_value - static_cast<underlying_type>(e)); }
	constexpr flags operator |(EnumT e) const { return flags( m_value | static_cast<underlying_type>(e)); }
	constexpr flags operator &(EnumT e) const { return flags( m_value & static_cast<underlying_type>(e)); }

	constexpr flags operator +(flags f) const { return flags(m_value + f.m_value); }
	constexpr flags operator -(flags f) const { return flags(m_value - f.m_value); }
	constexpr flags operator |(flags f) const { return flags(m_value | f.m_value); }
	constexpr flags operator &(flags f) const { return flags(m_value & f.m_value); }

	flags& operator =(EnumT e) { m_value = static_cast<underlying_type>(e); return *this;}
	flags& operator +=(EnumT e) { m_value += static_cast<underlying_type>(e); return *this; }
	flags& operator -=(EnumT e) { m_value -= static_cast<underlying_type>(e); return *this; }
	flags& operator |=(EnumT e) { m_value |= static_cast<underlying_type>(e); return *this; }
	flags& operator &=(EnumT e) { m_value &= static_cast<underlying_type>(e); return *this; }

	flags& operator =(flags f) { m_value = f.m_value; return *this; }
	flags& operator +=(flags f) { m_value += f.m_value; return *this; }
	flags& operator -=(flags f) { m_value -= f.m_value; return *this; }
	flags& operator |=(flags f) { m_value |= f.m_value; return *this; }
	flags& operator &=(flags f) { m_value &= f.m_value; return *this; }
	
	constexpr underlying_type get() const
	{
		return m_value;
	}
	constexpr operator bool() const
	{
		return m_value;
	}
	explicit constexpr operator underlying_type() const
	{
		return m_value;
	}
};

template<typename EnumT>
constexpr flags<EnumT> make_flags(EnumT e)
{
	return flags<EnumT>(e);
}

}

#endif
