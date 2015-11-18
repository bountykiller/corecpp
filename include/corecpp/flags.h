#ifndef CORE_CPP_FLAGS_H
#define CORE_CPP_FLAGS_H

#include <type_traits>

namespace corecpp
{

template<typename _Enum>
class flags
{
public:
	using underlying_type = typename std::underlying_type<_Enum>::type;
private:
	underlying_type m_value;
public:
	flags(const flags& value) = default;
	constexpr flags(_Enum e) : m_value (static_cast<underlying_type>(e))
	{}
	explicit constexpr flags(underlying_type e) : m_value(e)
	{}
	constexpr flags operator +(_Enum e) const { return flags( m_value + static_cast<underlying_type>(e)); }
	constexpr flags operator -(_Enum e) const { return flags( m_value - static_cast<underlying_type>(e)); }
	constexpr flags operator |(_Enum e) const { return flags( m_value | static_cast<underlying_type>(e)); }
	constexpr flags operator &(_Enum e) const { return flags( m_value & static_cast<underlying_type>(e)); }

	constexpr flags operator +(flags f) const { return flags(m_value + f.m_value); }
	constexpr flags operator -(flags f) const { return flags(m_value - f.m_value); }
	constexpr flags operator |(flags f) const { return flags(m_value | f.m_value); }
	constexpr flags operator &(flags f) const { return flags(m_value & f.m_value); }

	flags& operator =(_Enum e) { m_value = static_cast<underlying_type>(e); return *this;}
	flags& operator +=(_Enum e) { m_value += static_cast<underlying_type>(e); return *this; }
	flags& operator -=(_Enum e) { m_value -= static_cast<underlying_type>(e); return *this; }
	flags& operator |=(_Enum e) { m_value |= static_cast<underlying_type>(e); return *this; }
	flags& operator &=(_Enum e) { m_value &= static_cast<underlying_type>(e); return *this; }

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

template<typename E>
constexpr flags<E> make_flags(E e)
{
	return flags<E>(e);
}

}

#endif
