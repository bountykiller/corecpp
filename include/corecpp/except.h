#ifndef CORE_CPP_EXCEPT_H
#define CORE_CPP_EXCEPT_H

#include <format>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <source_location>

#include <corecpp/algorithm.h>

/* compiler-dependent stuff */
#include <cxxabi.h>


namespace corecpp
{

	template <typename ExceptionT>
	class error : public ExceptionT
	{
		std::source_location m_location;

	public:
		template <typename StringT>
		error(StringT&& message, const std::source_location& location = std::source_location::current()) :
			ExceptionT(std::format(" at {}:{} {} ", location.file_name(), location.line(), std::forward<StringT>(message))),
			m_location(location)
		{}

		constexpr const std::source_location& location() const noexcept
		{
			return m_location;
		}
	};

	struct unimplemented : public error<std::logic_error>
	{
		template<typename... ArgsT>
		unimplemented(ArgsT&&... args)
		: error<std::logic_error> { std::forward<ArgsT>(args)... }
		{ };
	};

	class bad_access : public error<std::logic_error>
	{
		template<typename ExpectedT, typename ActualT,  typename StringT>
		std::string build_message(StringT&& message)
		{

			const char* mangled_expected { typeid(ExpectedT).name() };
			const char* mangled_actual { typeid(ActualT).name() };
			if (!mangled_expected)
				return error<std::logic_error>::what();
			int status;
			std::unique_ptr<char> expected { abi::__cxa_demangle(mangled_expected, 0, 0, &status) };
			std::unique_ptr<char> actual { abi::__cxa_demangle(mangled_actual, 0, 0, &status) };
			return corecpp::concat<std::string>({ std::forward<StringT>(message), " ", expected, " expected, got ", actual });
		}

	public:
		template<typename... ArgsT>
		bad_access(ArgsT&&... args)
		: error<std::logic_error> { std::forward<ArgsT>(args)... }
		{ }

		template<typename ExpectedT, typename ActualT, typename StringT>
		bad_access(StringT&& message)
		: error<std::logic_error> { build_message<ExpectedT, ActualT>(std::forward<StringT>(message)) }
		{ }
	};

	template<typename ExpectedT, typename ActualT>
	class bad_type_access : public error<std::logic_error>
	{
		template<typename StringT>
		std::string build_message(StringT&& message)
		{

			const char* mangled_expected { typeid(ExpectedT).name() };
			const char* mangled_actual { typeid(ActualT).name() };
			if (!mangled_expected)
				return error<std::logic_error>::what();
			int status;
			std::unique_ptr<char> expected { abi::__cxa_demangle(mangled_expected, 0, 0, &status) };
			std::unique_ptr<char> actual { abi::__cxa_demangle(mangled_actual, 0, 0, &status) };
			return corecpp::concat<std::string>({ std::forward<StringT>(message), " ", expected.get(), " expected, got ", actual.get() });
		}

	public:
		template<typename StringT>
		bad_type_access(StringT&& message)
		: error<std::logic_error> { build_message(std::forward<StringT>(message)) }
		{ }
	};

	struct lexical_error : public error<std::runtime_error>
	{
		template<typename... ArgsT>
		lexical_error(ArgsT&&... args)
		: error<std::runtime_error> { std::forward<ArgsT>(args)... }
		{ };
	};

	struct syntax_error : public error<std::runtime_error>
	{
		template<typename... ArgsT>
		syntax_error(ArgsT&&... args)
		: error<std::runtime_error> { std::forward<ArgsT>(args)... }
		{ };
	};

	struct semantic_error : public error<std::runtime_error>
	{
		template<typename... ArgsT>
		semantic_error(ArgsT&&... args)
		: error<std::runtime_error> { std::forward<ArgsT>(args)... }
		{ };
	};

	struct format_error : public error<std::invalid_argument>
	{
		template<typename... ArgsT>
		format_error(ArgsT&&... args)
		: error<std::invalid_argument> { std::forward<ArgsT>(args)... }
		{ };
	};

	struct unexpected_error : public error<std::logic_error>
	{
		template<typename... ArgsT>
		unexpected_error(ArgsT&&... args)
		: error<std::logic_error> { std::forward<ArgsT>(args)... }
		{ };
	};


	template<typename ExceptionT, typename... ArgsT>
	[[ noreturn ]] void throws (ArgsT&&... args)
	{
		throw ExceptionT { std::forward<ArgsT>(args)... };
	}

	[[ noreturn ]] void throw_system_error(int err, const std::error_category& cat);

	[[ noreturn ]] static inline void unreachable()
	{
		corecpp::throws<std::logic_error>("unreachable");
	}
}

#endif
