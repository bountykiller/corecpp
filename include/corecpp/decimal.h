#ifndef CORECPP_DECIMAL_H
#define CORECPP_DECIMAL_H

#include <string>


namespace corecpp
{

namespace
{

template<typename T, T a, unsigned int b>
struct _power
{
	static constexpr auto value = a * _power<T, a, b -1>::value;
};

template<typename T, T a>
struct _power<T, a, 1>
{
	static constexpr auto value = a;
};

}

template<std::size_t scale>
class decimal
{
	long long m_value;
	static constexpr auto scale_factor = _power<std::size_t, 10, scale>::value;
	static_assert(scale > 0, "scale should be > 0");
public:
	decimal() = default;
	decimal(const decimal& other) : m_value(other.m_value)
	{
	}
	decimal(long long value) : m_value(scale_factor * value)
	{}
	decimal(double value) : m_value(scale_factor * value)
	{}
	decimal& operator=(decimal) = default;
	decimal& operator+=(decimal other)
	{
		m_value += other.m_value;
		return *this;
	}
	decimal operator+(decimal other) const
	{
		return decimal(*this) += other;
	}
	
	decimal& operator-=(decimal other)
	{
		m_value -= other.m_value;
		return *this;
	}
	decimal operator-(decimal other) const
	{
		return decimal(*this) -= other;
	}

	decimal& operator*=(decimal other)
	{
		(m_value *= other.m_value) / scale_factor;
		return *this;
	}
	decimal operator*(decimal other) const
	{
		return decimal(*this) *= other;
	}

	decimal& operator/=(decimal other)
	{
		m_value = (m_value * scale_factor) / other.m_value;
		return *this;
	}
	decimal operator/(decimal other) const
	{
		return decimal(*this) /= other;
	}
};
	
}

#endif
