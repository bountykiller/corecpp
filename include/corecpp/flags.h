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
	constexpr flags operator +(EnumT e) const { return flags( m_value | static_cast<underlying_type>(e)); }
	constexpr flags operator -(EnumT e) const { return flags( m_value & ~static_cast<underlying_type>(e)); }
	constexpr flags operator |(EnumT e) const { return flags( m_value | static_cast<underlying_type>(e)); }
	constexpr flags operator &(EnumT e) const { return flags( m_value & static_cast<underlying_type>(e)); }
	constexpr flags operator ^(EnumT e) const { return flags( m_value ^ static_cast<underlying_type>(e)); }

	constexpr flags operator +(flags f) const { return flags(m_value | f.m_value); }
	constexpr flags operator -(flags f) const { return flags(m_value & ~f.m_value); }
	constexpr flags operator |(flags f) const { return flags(m_value | f.m_value); }
	constexpr flags operator &(flags f) const { return flags(m_value & f.m_value); }
	constexpr flags operator ^(flags f) const { return flags(m_value ^ f.m_value); }

	flags& operator =(EnumT e) { m_value = static_cast<underlying_type>(e); return *this; }
	flags& operator +=(EnumT e) { m_value |= static_cast<underlying_type>(e); return *this; }
	flags& operator -=(EnumT e) { m_value &= ~static_cast<underlying_type>(e); return *this; }
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
	constexpr bool operator ==(flags f) const
	{
		return m_value == f.m_value;
	}
	constexpr bool operator ==(EnumT e) const
	{
		return m_value == static_cast<underlying_type>(e);
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

	constexpr underlying_type get() const
	{
		return m_value;
	}
	flags& set(EnumT e)
	{
		m_value |= static_cast<underlying_type>(e);
		return *this;
	}
	template <typename... Args>
	flags& set(EnumT e, Args&&... values)
	{
		m_value |= static_cast<underlying_type>(e);
		set(std::forward<Args>(values)...);
		return *this;
	}
	flags& unset(EnumT e)
	{
		m_value &= ~static_cast<underlying_type>(e);
		return *this;
	}
	template <typename... Args>
	flags& unset(EnumT e, Args&&... values)
	{
		m_value &= ~static_cast<underlying_type>(e);
		unset(std::forward<Args>(values)...);
		return *this;
	}

	constexpr bool is_set(EnumT e) const
	{
		return ((m_value & e) == e);
	}
	template <typename... Args>
	constexpr bool is_all_set(EnumT e, Args&&... values) const
	{
		return is_set(e) && is_all_set(std::forward<Args>(values)...);
	}
	template <typename... Args>
	constexpr bool is_one_set(EnumT e, Args&&... values) const
	{
		return is_set(e) || is_one_set(std::forward<Args>(values)...);
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
