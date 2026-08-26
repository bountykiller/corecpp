#ifndef CORECPP_EXPECTED_H
#define CORECPP_EXPECTED_H

#include <format>
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

	/**
	 * Make the program crash if an error is detected
	 *
	 * \details if an error is detected, this method will throw an exception.
	 * Since she'd marked as noexcept, this will result in std::terminate being called
	 */
	static inline void ensure(bool condition, const std::source_location& location = std::source_location::current()) noexcept
	{
		if (!condition) [[unlikely]]
			throws<unexpected_error>("The unexpected happenend!!!", location);
	}

	/**
	 * Make the program crash if an error is detected
	 *
	 * \details if an error is detected, this method will throw an exception.
	 * Since she'd marked as noexcept, this will result in std::terminate being called
	 */
	static inline void ensure(bool condition, const char *what, const std::source_location& location = std::source_location::current()) noexcept
	{
		if (!condition) [[unlikely]]
			throws<unexpected_error>(what, location);
	}

	/**
	 * Make the program crash if an error is detected
	 *
	 * \details if an error is detected, this method will throw an exception.
	 * Since she'd marked as noexcept, this will result in std::terminate being called
	 */
	template<typename TValue>
	static inline void ensure_equal(const TValue& actual, const TValue &expected, const std::source_location& location = std::source_location::current()) noexcept
	{
		if (actual != expected) [[unlikely]]
			throws<unexpected_error>(std::format("The unexpected happenend: got {} instead of {} !!!", expected, actual), location);
	}

	/**
	 * Make the program crash if an error is detected
	 *
	 * \details if an error is detected, this method will throw an exception.
	 * Since she'd marked as noexcept, this will result in std::terminate being called
	 */
	static inline void unsupported(bool condition) noexcept
	{
		if ( condition ) [[unlikely]]
			throws<unimplemented>(std::format("Unsupported case happened"));
	}
}

#endif
