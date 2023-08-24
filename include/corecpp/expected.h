#ifndef CORECPP_EXPECTED_H
#define CORECPP_EXPECTED_H

#include <utility>

#include <corecpp/variant.h>

namespace corecpp
{
	template <typename T, typename ErrT>
	class expected
	{
		using value_type = corecpp::variant<T, ErrT>;
		value_type m_value;
		template <typename U, typename ErrU>
		friend std::ostream& operator << (std::ostream& os, const expected<U, ErrU>& e);
	public:
		expected() = delete; /* no default ctor */

		expected(ErrT&& arg)
		: m_value { arg }
		{
		}

		template<typename... ArgsT>
		expected(ArgsT&&... args)
		: m_value { T { std::forward<ArgsT>(args)... } }
		{
		}

		bool operator!() const noexcept
		{
			return m_value.index() != (value_type::template index_of_v<T>);
		}

		operator bool() const noexcept
		{
			return m_value.index() == (value_type::template index_of_v<T>);
		}

		bool has_value() const noexcept
		{
			return m_value.index() == (value_type::template index_of_v<T>);
		}

		const T& value_or(const T& def) const noexcept
		{
			return (has_value() ? m_value.get() : def);
		}

		const ErrT& error() const
		{
			if ( has_value() )
				throw corecpp::bad_access("not in error state");
			return m_value.template get<ErrT>();
		}
	};

	template <typename T, typename ErrT>
	std::ostream& operator << (std::ostream& os, const expected<T, ErrT>& e)
	{
		return e.apply([&os](auto x) { return os << x; });
	}

}

#endif
