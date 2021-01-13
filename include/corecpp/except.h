#ifndef CORE_CPP_EXCEPT_H
#define CORE_CPP_EXCEPT_H

#include <stdexcept>
#if __cplusplus > 201703L
#include <source_location>
#include <corecpp/algorithm.h>
#endif

namespace corecpp
{
	template <typename ExceptionT>
	class error : public ExceptionT
	{
#if __cplusplus > 201703L
		std::source_location m_location;
#endif

	public:
		template <typename StringT>
#if __cplusplus > 201703L
		error(StringT&& message, const std::source_location& location = std::source_location::current())
		: ExceptionT(std::forward<StringT>(message)), m_location(location)
#else
		error(StringT&& message)
		: ExceptionT(std::forward<StringT>(message))
#endif
		{}

#if __cplusplus > 201703L
		const source_location& location() const
		{
			return m_location;
		}

		std::string what() override
		{
			return corecpp::concat<std::string>({" at "s, std::string { m_location.file_name() },
				":"s, std::to_string(m_location.line),
				" "s, ExceptionT::what()});
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

	struct bad_access : public error<std::logic_error>
	{
		template<typename... ArgsT>
		bad_access(ArgsT&&... args)
		: error<std::logic_error> { std::forward<ArgsT>(args)... }
		{ };
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

	template<typename ExceptionT, typename... ArgsT>
	[[ noreturn ]] ExceptionT throws(ArgsT&&... args)
	{
		throw ExceptionT { std::forward<ArgsT>(args)... };
	}
}

#endif
