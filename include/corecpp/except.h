#ifndef CORE_CPP_EXCEPT_H
#define CORE_CPP_EXCEPT_H

#include <stdexcept>
#if __cplusplus > 201703L
#include <source_location>
#endif

#include <corecpp/algorithm.h>

/* compiler-dependent stuff */
#include <cxxabi.h>


namespace corecpp
{

	template <typename ExceptionT>
	class error : public ExceptionT
	{
#if __cplusplus > 201703L
		std::source_location m_location;

		template <typename StringT>
		std::string build_message(StringT&& message) noexcept
		{
			return corecpp::concat<std::string>({" at "s, std::string { m_location.file_name() },
												":"s, std::to_string(m_location.line),
												" "s, std::forward<StringT>(message)});
		}
#endif

	public:
		template <typename StringT>
#if __cplusplus > 201703L
		error(StringT&& message, const std::source_location& location = std::source_location::current())
		: ExceptionT(build_message(std::forward<StringT>(message)), m_location(location)
#else
		error(StringT&& message)
		: ExceptionT(std::forward<StringT>(message))
#endif
		{}

#if __cplusplus > 201703L
		constexpr const source_location& location() const noexcept final
		{
			return m_location;
		}
#endif
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

	template<typename ExceptionT, typename... ArgsT>
	[[ noreturn ]] void throws (ArgsT&&... args)
	{
		throw ExceptionT { std::forward<ArgsT>(args)... };
	}
}

#endif
