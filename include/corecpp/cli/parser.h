#ifndef CORECPP_COMMAND_LINE_PARSER_H_
#define CORECPP_COMMAND_LINE_PARSER_H_

#include <codecvt>
#include <locale>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>

#include <corecpp/except.h>

#include <corecpp/serialization/common.h>

namespace corecpp
{
	struct value_error : std::invalid_argument
	{
		value_error(const char* message)
		: invalid_argument(message)
		{}
		value_error(const std::string& message)
		: invalid_argument(message)
		{}
	};

	/**
	 * \brief abstraction of command line arguments
	 */
	class command_line final
	{
		static corecpp::diagnostic::event_producer& logger();

		int m_argc;
		char** m_argv;
		int m_pos = 0;
		std::vector<int> m_commands;	/* Will store the index of the commands once they are read.
										 * Necessary to allow the parser to show the full command
										 * on usage calls.
										 */
	public:
		command_line(int argc, char** argv) noexcept
		: m_argc(argc), m_argv(argv), m_pos(0)
		{
		}
		const char* program() const noexcept
		{
			return m_argv[0];
		}
		/**
		 * \brief read next command line argument without consuming it.
		 * \return the next argument as a char* (inherited from C). nullptr if no more arguments to read.
		 */
		const char* peek() const noexcept
		{
			if (m_pos >= m_argc)
				return nullptr;
			return m_argv[m_pos+1];
		}
		/**
		 * \brief read next command line argument and advance by 1
		 * \return the next argument as a char* (inherited from C). nullptr if no more arguments to read.
		 */
		const char* read() noexcept
		{
			if (m_pos >= m_argc)
				return nullptr;
			logger().info(corecpp::concat<std::string>({"read ", m_argv[m_pos+1]}), __FILE__, __LINE__);
			return m_argv[++m_pos];
		}
		/**
		 * \brief register an argument as being identified as a (sub-)command
		 */
		void store_command(void)
		{
			m_commands.push_back(m_pos);
		}
		std::string commands() const
		{
			std::string res;
			for(int command : m_commands)
				res.append(" ").append(m_argv[command]);
			return res;
		}
	};


	/**
	 * \brief class intented to parse one parameter as an option value received from the command line
	 * \note using a real parser here based on the deserialisation framework allows
	 * me to parse more easily parameters into std::vector or std::optional
	 * \warning complex types are not supported and will probably never be supported
	 * \implements deserialiser this concept is only partially implemented (see above)
	 */
	class long_option_parser
	{
		std::istringstream m_stream;

		template <typename IntegralT, typename = std::enable_if<std::is_integral<IntegralT>::value, IntegralT>>
		static void deserialize(std::istream& stream, IntegralT& value)
		{
			stream >> value;
		}

		template <typename StringT>
		static void convert_string(const std::string& parameter, StringT& value)
		{
			using char_type = typename StringT::value_type;
			std::wstring_convert<std::codecvt_utf8<char_type>, char_type> converter;
			value = converter.from_bytes(parameter);
		}

	public:
		explicit long_option_parser(const std::string& parameter)
		: m_stream(parameter)
		{
			m_stream.exceptions(std::ios_base::failbit | std::ios_base::badbit);
		}

		void deserialize(bool& value)
		{
			if (m_stream.str().empty())
			{
				/* if no value given, the presence of the option is synonym of a true value */
				value = true;
				return;
			}

			/* here I make a copy of the received string, put it in lower case
			 * and compare it to several boolean representation
			 * Note: not the best performing, but not intented to
			 */
			std::string lcase = m_stream.str();
			std::transform(lcase.begin(), lcase.end(), lcase.begin(), ::tolower);

			value = ( lcase == "true"
				|| lcase == "on"
				|| lcase == "yes"
				|| lcase == "1");
			/* required for checking parameter validity */
			if (!value
				&& lcase != "false"
				&& lcase != "off"
				&& lcase != "no"
				&& lcase != "0")
				corecpp::throws<corecpp::value_error>("boolean expected");
		}
		void deserialize(int8_t& value)
		{
			deserialize(m_stream, value);
		}
		void deserialize(int16_t& value)
		{
			deserialize(m_stream, value);
		}
		void deserialize(int32_t& value)
		{
			deserialize(m_stream, value);
		}
		void deserialize(int64_t& value)
		{
			deserialize(m_stream, value);
		}
		void deserialize(uint8_t& value)
		{
			deserialize(m_stream, value);
		}
		void deserialize(uint16_t& value)
		{
			deserialize(m_stream, value);
		}
		void deserialize(uint32_t& value)
		{
			deserialize(m_stream, value);
		}
		void deserialize(uint64_t& value)
		{
			deserialize(m_stream, value);
		}
		/* TODO
		void deserialize(char16_t value)
		{
			deserialize(m_stream, value);
		}
		*/
		void deserialize(float& value)
		{
			deserialize(m_stream, value);
		}
		void deserialize(double& value)
		{
			deserialize(m_stream, value);
		}
		void deserialize(std::string& value)
		{
			value = m_stream.str(); /* no further action needed as char unescaping should have been done by the shell */
		}
		void deserialize(std::wstring& value)
		{
			convert_string(m_stream.str(), value);
		}
		void deserialize(std::u16string& value)
		{
			convert_string(m_stream.str(), value);
		}
		void deserialize(std::u32string& value)
		{
			convert_string(m_stream.str(), value);
		}
		template <typename ValueT, typename Enable = void>
		void deserialize(ValueT& value)
		{
			deserialize_impl<long_option_parser, ValueT> impl;
			impl(*this, value);
		}

		/* "Low level" methods */
		template <typename ValueT>
		void begin_array()
		{
			// nothing to do
		}
		void end_array()
		{
			// nothing to do
		}
		template <typename ValueT>
		void read_array(ValueT& value)
		{
			std::stringstream buffer;
			do
			{
				for (char c = m_stream.get();
					c != ',' && c != std::char_traits<char>::eof();
					c = m_stream.get())
				{
					buffer << c;
				}
				value.emplace_back();
				deserialize(buffer, value.back());
				buffer.str("");
			}
			while (!m_stream.eof());
		}
		/* required for variant */
		template <typename ValueT>
		void read_element(ValueT& value)
		{
			deserialize(value);
		}

		/*
		Unsupported methods

		template <typename FuncT>
		void read_element_cb(FuncT func)
		{
			static_assert(false, "unable to read element cb from command line");
		}
		void deserialize(std::nullptr_t)
		{
			static_assert(false, "unable to read nullptr from command line");
		}

		template <typename ValueT>
		void begin_object()
		{
			static_assert(false, "unable to read object from command line");
		}
		void end_object()
		{
			static_assert(false, "unable to read object from command line");
		}

		template <typename ValueT>
		void read_object(ValueT& value)
		{
			static_assert(false, "unable to read object from command line");
		}
		template <typename ValueT, typename PropertiesT>
		void read_object(ValueT& value, const PropertiesT& properties)
		{
			static_assert(false, "unable to read object from command line");
		}
		void read_object_cb(std::function<void(const std::wstring&)> func)
		{
			static_assert(false, "unable to read object from command line");
		}
		template <typename ValueT>
		void read_associative_array(ValueT& value)
		{
			static_assert(false, "unable to read associative array from command line");
		}
		*/
	};


	/**
	 * \brief class intented to parse each parameter as an option value received from the command line
	 * \note using a real parser here based on the deserialisation framework allows
	 * me to parse more easily parameters into std::vector or std::optional
	 * \warning complex types are not supported and will probably never be supported
	 * \implements deserialiser this concept is only partially implemented (see above)
	 */
	class short_option_parser
	{
		command_line& m_line;

		template<typename IntegralT, typename = std::enable_if<std::is_integral<IntegralT>::value, IntegralT>>
		void deserialize_integral(IntegralT& value)
		{
			const char* parameter = m_line.read();
			std::istringstream iss(parameter);
			iss >> value;
		}
		template<typename FloatT, typename = std::enable_if<std::is_integral<FloatT>::value, FloatT>>
		void deserialize_float(FloatT& value)
		{
			const char* parameter = m_line.read();
			std::istringstream iss(parameter);
			iss >> value; //OK?
		}
		template <typename StringT>
		void read_string(StringT& value)
		{
			using char_type = typename StringT::value_type;
			std::string parameter = m_line.read();
			std::wstring_convert<std::codecvt_utf8<char_type>, char_type> converter;
			value = converter.from_bytes(parameter);
		}

	public:
		/**
		 * \brief say if the reader accepts the value to be an empty string
		 * \retval true if the reader expects the value to never be an empty string
		 * \retval false if the reader accept the value to be an empty string
		 */
		explicit short_option_parser(command_line& line) noexcept
		: m_line(line)
		{
		}
		void deserialize(int8_t& value)
		{
			deserialize_integral(value);
		}
		void deserialize(int16_t& value)
		{
			deserialize_integral(value);
		}
		void deserialize(int32_t& value)
		{
			deserialize_integral(value);
		}
		void deserialize(int64_t& value)
		{
			deserialize_integral(value);
		}
		void deserialize(uint8_t& value)
		{
			deserialize_integral(value);
		}
		void deserialize(uint16_t& value)
		{
			deserialize_integral(value);
		}
		void deserialize(uint32_t& value)
		{
			deserialize_integral(value);
		}
		void deserialize(uint64_t& value)
		{
			deserialize_integral(value);
		}
		void deserialize(char16_t value)
		{
			corecpp::throws<corecpp::unimplemented>("");
		}
		void deserialize(float& value)
		{
			deserialize_float(value);
		}
		void deserialize(double& value)
		{
			deserialize_float(value);
		}
		void deserialize(std::string& value)
		{
			value = m_line.read(); /* no further action needed as char unescaping should have been done by the shell */
		}
		void deserialize(std::wstring& value)
		{
			read_string(value);
		}
		void deserialize(std::u16string& value)
		{
			read_string(value);
		}
		void deserialize(std::u32string& value)
		{
			read_string(value);
		}
		template <typename ValueT, typename Enable = void>
		void deserialize(ValueT& value)
		{
			deserialize_impl<short_option_parser, ValueT> impl;
			impl(*this, value);
		}

		/* "Low level" methods */
		template <typename ValueT>
		void begin_array()
		{
			// nothing to do
		}
		void end_array()
		{
			// nothing to do
		}
		template <typename ValueT>
		void read_array(ValueT& value)
		{
			for (const char* parameter = m_line.peek();
				parameter[0] && parameter[0] != '-';
				)
			{
				value.emplace_back();
				deserialize(value.back()); //will consume the parameter
			}
		}
		/* required for variant */
		template <typename ValueT>
		void read_element(ValueT& value)
		{
			deserialize(value);
		}

		/*
		Unsupported methods

		void deserialize(bool& value)
		{
			static_assert(false, "boolean shouldn't be parsed one the command line when using short forms of parameters");
		}
		template <typename FuncT>
		void read_element_cb(FuncT func)
		{
			static_assert(false, "unable to read element cb from command line");
		}
		void deserialize(std::nullptr_t)
		{
			static_assert(false, "unable to read nullptr from command line");
		}

		template <typename ValueT>
		void begin_object()
		{
			static_assert(false, "unable to read object from command line");
		}
		void end_object()
		{
			static_assert(false, "unable to read object from command line");
		}

		template <typename ValueT>
		void read_object(ValueT& value)
		{
			static_assert(false, "unable to read object from command line");
		}
		template <typename ValueT, typename PropertiesT>
		void read_object(ValueT& value, const PropertiesT& properties)
		{
			static_assert(false, "unable to read object from command line");
		}
		void read_object_cb(std::function<void(const std::wstring&)> func)
		{
			static_assert(false, "unable to read object from command line");
		}
		template <typename ValueT>
		void read_associative_array(ValueT& value)
		{
			static_assert(false, "unable to read associative array from command line");
		}
		*/
	};





	/**
	 * \brief class intented to parse each parameter as a parameter received at the end of the command line
	 * \note using a real parser here based on the deserialisation framework allows
	 * me to parse more easily parameters into std::vector or std::optional
	 * \warning complex types are not supported and will probably never be supported
	 * \implements deserialiser this concept is only partially implemented (see above)
	 */
	class argument_parser
	{
		command_line& m_line;

		template<typename IntegralT, typename = std::enable_if<std::is_integral<IntegralT>::value, IntegralT>>
		void deserialize_integral(IntegralT& value)
		{
			const char* parameter = m_line.read();
			std::istringstream iss(parameter);
			iss >> value;
		}
		template<typename FloatT, typename = std::enable_if<std::is_integral<FloatT>::value, FloatT>>
		void deserialize_float(FloatT& value)
		{
			const char* parameter = m_line.read();
			std::istringstream iss(parameter);
			iss >> value; //OK?
		}

	public:
		/**
		 * \brief say if the reader accepts the value to be an empty string
		 * \retval true if the reader expects the value to never be an empty string
		 * \retval false if the reader accept the value to be an empty string
		 */
		explicit argument_parser(command_line& line) noexcept
		: m_line(line)
		{
		}

		void deserialize(bool& value)
		{
			/* here I make a copy of the received string, put it in lower case
			 * and compare it to several boolean representation
			 * Note: not the best performing, but not intented to
			 */
			std::string lcase = m_line.read();
			std::transform(lcase.begin(), lcase.end(), lcase.begin(), ::tolower);

			value = ( lcase == "true"
				|| lcase == "on"
				|| lcase == "yes"
				|| lcase == "1");
			/* required for checking parameter validity */
			if (!value
				&& lcase != "false"
				&& lcase != "off"
				&& lcase != "no"
				&& lcase != "0")
				corecpp::throws<corecpp::value_error>("boolean expected");
		}
		void deserialize(int8_t& value)
		{
			deserialize_integral(value);
		}
		void deserialize(int16_t& value)
		{
			deserialize_integral(value);
		}
		void deserialize(int32_t& value)
		{
			deserialize_integral(value);
		}
		void deserialize(int64_t& value)
		{
			deserialize_integral(value);
		}
		void deserialize(uint8_t& value)
		{
			deserialize_integral(value);
		}
		void deserialize(uint16_t& value)
		{
			deserialize_integral(value);
		}
		void deserialize(uint32_t& value)
		{
			deserialize_integral(value);
		}
		void deserialize(uint64_t& value)
		{
			deserialize_integral(value);
		}
		void deserialize(char16_t value)
		{
			corecpp::throws<corecpp::unimplemented>("");
		}
		void deserialize(float& value)
		{
			deserialize_float(value);
		}
		void deserialize(double& value)
		{
			deserialize_float(value);
		}
		void deserialize(std::string& value)
		{
			corecpp::throws<corecpp::unimplemented>("");
		}
		void deserialize(std::wstring& value)
		{
			corecpp::throws<corecpp::unimplemented>("");
			//value = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(arg_value);
		}
		void deserialize(std::u16string& value)
		{
			corecpp::throws<corecpp::unimplemented>("");
		}
		void deserialize(std::u32string& value)
		{
			corecpp::throws<corecpp::unimplemented>("");
		}
		template <typename ValueT, typename Enable = void>
		void deserialize(ValueT& value)
		{
			deserialize_impl<argument_parser, ValueT> impl;
			impl(*this, value);
		}

		/* "Low level" methods */
		template <typename ValueT>
		void begin_array()
		{
			// nothing to do
		}
		void end_array()
		{
			// nothing to do
		}
		template <typename ValueT>
		void read_array(ValueT& value)
		{
			for (const char* parameter = m_line.peek();
				parameter[0] && parameter[0] != '-';
				)
			{
				value.emplace_back();
				deserialize(value.back());
			}

		}
		/* required for variant */
		template <typename ValueT>
		void read_element(ValueT& value)
		{
			deserialize(value);
		}

		/*
		Unsupported methods

		template <typename FuncT>
		void read_element_cb(FuncT func)
		{
			static_assert(false, "unable to read element cb from command line");
		}
		void deserialize(std::nullptr_t)
		{
			static_assert(false, "unable to read nullptr from command line");
		}

		template <typename ValueT>
		void begin_object()
		{
			static_assert(false, "unable to read object from command line");
		}
		void end_object()
		{
			static_assert(false, "unable to read object from command line");
		}

		template <typename ValueT>
		void read_object(ValueT& value)
		{
			static_assert(false, "unable to read object from command line");
		}
		template <typename ValueT, typename PropertiesT>
		void read_object(ValueT& value, const PropertiesT& properties)
		{
			static_assert(false, "unable to read object from command line");
		}
		void read_object_cb(std::function<void(const std::wstring&)> func)
		{
			static_assert(false, "unable to read object from command line");
		}
		template <typename ValueT>
		void read_associative_array(ValueT& value)
		{
			static_assert(false, "unable to read associative array from command line");
		}
		*/
	};
}

#endif
