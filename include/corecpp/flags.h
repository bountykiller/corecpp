#ifndef CORE_CPP_FLAGS_H
#define CORE_CPP_FLAGS_H

#include <iostream>
#include <type_traits>

namespace corecpp
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
	explicit constexpr flags(underlying_type e = 0) : m_value(e)
	{}
	constexpr flags operator +(EnumT e) const { return flags( m_value + static_cast<underlying_type>(e)); } /* or should it be m_value | static_cast<underlying_type>(e));  ?*/
	constexpr flags operator -(EnumT e) const { return flags( m_value - static_cast<underlying_type>(e)); } /* or should it be m_value & ~static_cast<underlying_type>(e)); ?*/
	constexpr flags operator |(EnumT e) const { return flags( m_value | static_cast<underlying_type>(e)); }
	constexpr flags operator &(EnumT e) const { return flags( m_value & static_cast<underlying_type>(e)); }
	constexpr flags operator ^(EnumT e) const { return flags( m_value ^ static_cast<underlying_type>(e)); }

	constexpr flags operator +(flags f) const { return flags(m_value + f.m_value); }
	constexpr flags operator -(flags f) const { return flags(m_value - f.m_value); }
	constexpr flags operator |(flags f) const { return flags(m_value | f.m_value); }
	constexpr flags operator &(flags f) const { return flags(m_value & f.m_value); }
	constexpr flags operator ^(flags f) const { return flags(m_value ^ f.m_value); }

	flags& operator =(EnumT e) { m_value = static_cast<underlying_type>(e); return *this; }
	flags& operator +=(EnumT e) { m_value += static_cast<underlying_type>(e); return *this; }
	flags& operator -=(EnumT e) { m_value -= static_cast<underlying_type>(e); return *this; }
	flags& operator |=(EnumT e) { m_value |= static_cast<underlying_type>(e); return *this; }
	flags& operator &=(EnumT e) { m_value &= static_cast<underlying_type>(e); return *this; }
	flags& operator ^=(EnumT e) { m_value ^= static_cast<underlying_type>(e); return *this; }

	flags& operator =(flags f) { m_value = f.m_value; return *this; }
	flags& operator +=(flags f) { m_value += f.m_value; return *this; }
	flags& operator -=(flags f) { m_value -= f.m_value; return *this; }
	flags& operator |=(flags f) { m_value |= f.m_value; return *this; }
	flags& operator &=(flags f) { m_value &= f.m_value; return *this; }
	flags& operator ^=(flags f) { m_value ^= f.m_value; return *this; }

	flags& operator =(underlying_type value) { m_value = value; return *this; }

	constexpr flags operator ~() const { return ~m_value; }

	constexpr underlying_type get() const
	{
		return m_value;
	}
	constexpr bool operator !() const
	{
		return !m_value;
	}
	explicit constexpr operator bool() const
	{
		return m_value;
	}
	explicit constexpr operator underlying_type() const
	{
		return m_value;
	}
};

template <typename EnumT>
std::ostream& operator << (std::ostream& s, flags<EnumT> f)
{
	return s << f.get();
}
template<typename E>
constexpr typename std::enable_if<std::is_enum<E>::value, flags<E>>::type
operator + (E e, E e2)
{
	return flags<E>(e) + flags<E>(e2);
}

template<typename E>
constexpr typename std::enable_if<std::is_enum<E>::value, flags<E>>::type
operator - (E e, E e2)
{
	return flags<E>(e) - flags<E>(e2);
}

template<typename E>
constexpr typename std::enable_if<std::is_enum<E>::value, flags<E>>::type
operator | (E e, E e2)
{
	return flags<E>(e) | flags<E>(e2);
}

template<typename E>
constexpr typename std::enable_if<std::is_enum<E>::value, flags<E>>::type
operator & (E e, E e2)
{
	return flags<E>(e) & flags<E>(e2);
}

template<typename E>
constexpr typename std::enable_if<std::is_enum<E>::value, flags<E>>::type
operator ^ (E e, E e2)
{
	return flags<E>(e) ^ flags<E>(e2);
}

template<typename E>
constexpr typename std::enable_if<std::is_enum<E>::value, flags<E>>::type
operator ~ (E e)
{
	return ~flags<E>(e);
}


template<typename E>
constexpr flags<E> make_flags(E e)
{
	return flags<E>(e);
}

}

#endif
