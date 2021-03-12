#ifndef CORECPP_COMMAND_LINE_H_
#define CORECPP_COMMAND_LINE_H_

#include <algorithm>
#include <codecvt>
#include <functional>
#include <locale>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <corecpp/algorithm.h>
#include <corecpp/diagnostic.h>
#include <corecpp/except.h>

#include <corecpp/serialization/common.h>

#include <corecpp/cli/parser.h>

/* TODO: Manage options groups
 */

namespace corecpp
{

	/**
	 * \brief interface used for reading command_line option
	 */
	class option_reader
	{
	protected:
		static corecpp::diagnostic::event_producer& logger();
	public:
		/**
		 * \brief say if the reader accepts the value to be an empty string
		 * \retval true if the reader expects the value to never be an empty string
		 * \retval false if the reader accept the value to be an empty string
		 */
		virtual ~option_reader()
		{}
		virtual void read(short_option_parser& parser) const = 0;
		virtual void read(long_option_parser& parser) const = 0;
		virtual void read(argument_parser& parser) const = 0;
	};

	/**
	 * \brief class intented to read most of the possible option from the command line
	 * require T to define the '>>' operator
	 */
	template <typename T, typename Enable = void>
	class generic_option : public option_reader
	{
		T& m_value;
	public:
		using value_type = T;
		generic_option(T& value) : m_value(value)
		{}
		void read(short_option_parser& parser) const override
		{
			logger().debug("reading ...", __FILE__, __LINE__);
			parser.deserialize(m_value);
		}
		void read(long_option_parser& parser) const override
		{
			logger().debug("reading ...", __FILE__, __LINE__);
			parser.deserialize(m_value);
		}
		void read(argument_parser& parser) const override
		{
			logger().debug("reading ...", __FILE__, __LINE__);
			parser.deserialize(m_value);
		}
	};

	/**
	 * \brief specialization for boolean. Will return true if the option is present without a value
	 */
	template <typename T>
	class generic_option<T, std::enable_if_t<std::is_same_v<T, bool>>> : public option_reader
	{
		bool& m_value;
	public:
		using value_type = bool;
		generic_option(bool& value) : m_value(value)
		{}
		void read([[maybe_unused]]short_option_parser& parser) const override
		{
			logger().debug("reading ...", __FILE__, __LINE__);
			// In short form, boolean is set to true if the parameter is present.
			// No more parsing is required
			m_value = true;
		};
		void read(long_option_parser& parser) const override
		{
			logger().debug("reading ...", __FILE__, __LINE__);
			parser.deserialize(m_value);
		}
		void read(argument_parser& parser) const override
		{
			logger().debug("reading ...", __FILE__, __LINE__);
			parser.deserialize(m_value);
		}
	};

	/**
	 *\brief parameters lies at the end of the command line
	 */
	class program_parameter
	{
		std::string m_name;
		std::string m_helpmsg;
		std::unique_ptr<option_reader> m_reader;
		bool m_optional;
	public:
		program_parameter(program_parameter&&) = default;
		template <typename T>
		program_parameter(std::string name, std::string helpmsg, T& value, bool optional = false)
		: m_name(name), m_helpmsg(helpmsg), m_reader(new generic_option<T>(value)), m_optional(optional)
		{
		}
		program_parameter& operator = (program_parameter&&) = default;
		const std::string& name() const
		{
			return m_name;
		}
		std::string helpmsg() const
		{
			return corecpp::concat<std::string>({ "\t", m_name, "\t", m_helpmsg });
		}
		bool is_required() const
		{
			return !m_optional;
		}
		void read(argument_parser& parser) const
		{
			m_reader->read(parser);
		};
	};

	/**
	 * \brief options when running a program or a specific command
	 */
	class program_option
	{
		char m_shortname;
		std::string m_name;
		std::string m_helpmsg;
		std::unique_ptr<option_reader> m_reader;
	public:
		program_option(program_option&&) = default;
		template <typename T>
		program_option(std::string name, std::string helpmsg, T& value)
		: m_shortname('\0'), m_name(name), m_helpmsg(helpmsg), m_reader(new generic_option<T>(value))
		{
		}
		template <typename T>
		program_option(char shortname, std::string name, std::string helpmsg, T& value)
		: m_shortname(shortname), m_name(name), m_helpmsg(helpmsg), m_reader(new generic_option<T>(value))
		{
		}
		program_option& operator = (program_option&&) = default;

		bool match(const std::string& name) const
		{
			return name == m_name;
		}
		bool match(char shortname) const
		{
			return shortname == m_shortname;
		}
		void read(short_option_parser& parser) const
		{
			try
			{
				m_reader->read(parser);
			}
			catch (std::ios_base::failure& e)
			{
				corecpp::throws<corecpp::format_error>(
					corecpp::concat<std::string>({ "invalid value for option '", std::string(1, m_shortname), "' : ", e.what() }));
			}
		}
		void read(long_option_parser& parser) const
		{
			try
			{
				m_reader->read(parser);
			}
			catch (std::ios_base::failure& e)
			{
				corecpp::throws<corecpp::format_error>(
					corecpp::concat<std::string>({ "invalid value for option '", m_name, "' : ", e.what() }));
			}
		};
		std::string helpmsg() const
		{
			if (!m_shortname)
				return corecpp::concat<std::string>({ "\t    --", m_name, "\t", m_helpmsg });
			else
				return corecpp::concat<std::string>({ "\t-", std::string(1, m_shortname),  ", --", m_name, "\t", m_helpmsg });
		}
	};

	/**
	 * \brief container which is intented to group multiple options togheter
	 */
	class program_option_group
	{
		std::string m_name;
		std::string m_helpmsg;
	public:
		program_option_group(program_option_group&&) = default;
		template <typename T>
		program_option_group(std::string name, std::string helpmsg)
		: m_name(name), m_helpmsg(helpmsg)
		{
		}
		program_option_group& operator = (program_option_group&&) = default;
	};

	/**
	 * \brief specific command which can be executed when specified on the command line
	 */
	class program_command
	{
		std::string m_name;
		std::string m_helpmsg;
		std::function<int(command_line&)> m_func;
	public:
		program_command(program_command&&) = default;
		program_command(std::string name, std::string helpmsg, std::function<int(command_line&)> func)
		: m_name(name), m_helpmsg(helpmsg), m_func(func)
		{
		}
		program_command& operator = (program_command&&) = default;
		bool match(const std::string& name) const
		{
			return name == m_name;
		}
		int execute(command_line& line) const
		{
			return m_func(line);
		}
		std::string helpmsg() const
		{
			return corecpp::concat<std::string>({ "\t", m_name, "\t", m_helpmsg });
		}
	};

	class command_line_parser final
	{
		static corecpp::diagnostic::event_producer& logger();

		command_line& m_command_line;
		std::vector<program_option> m_options;
		std::vector<program_command> m_commands;
		std::vector<program_parameter> m_params;

		auto get_option(const std::string& name) const
		{
			return std::find_if (std::cbegin(m_options), std::cend(m_options), [&](const program_option& o){ return o.match(name); });
		}
		auto get_command(const std::string& name) const
		{
			return std::find_if (std::cbegin(m_commands), std::cend(m_commands), [&](const program_command& c){ return c.match(name); });
		}

		auto get_option(char shortname) const
		{
			return std::find_if (std::cbegin(m_options), std::cend(m_options), [&](const program_option& o){ return o.match(shortname); });
		}

	public:
		explicit command_line_parser(command_line& line)
		: m_command_line(line)
		{}
		command_line_parser(command_line_parser&&) = default;
		command_line_parser(const command_line_parser&) = delete; /* options can't be copied anyway */
		command_line_parser& operator = (command_line_parser&) = delete;
		command_line_parser& operator = (command_line_parser&& other)  = delete;

		template <typename T>
		void add_option(char shortname, const std::string& name, const std::string& helpmsg, T&& value)
		{
			add_option(program_option { shortname, name, helpmsg, std::forward<T>(value) } );
		}
		void add_option(program_option&& descriptor)
		{
			m_options.emplace_back(std::forward<program_option>(descriptor));
		}
		void add_options(program_option&& descriptor)
		{
			add_option(std::forward<program_option>(descriptor));
		}
		template <typename... Args>
		void add_options(program_option&& descriptor, Args&&... values)
		{
			add_option(std::forward<program_option>(descriptor));
			add_options(std::forward<Args>(values)...);
		}

		void add_command(const std::string& name, const std::string& helpmsg, std::function<int(command_line&)> func)
		{
			add_command(program_command { name, helpmsg, func } );
		}
		void add_command(program_command&& descriptor)
		{
			m_commands.emplace_back(std::forward<program_command>(descriptor));
		}
		void add_commands(program_command&& descriptor)
		{
			add_command(std::forward<program_command>(descriptor));
		}
		template <typename... Args>
		void add_commands(program_command&& descriptor, Args&&... values)
		{
			add_command(std::forward<program_command>(descriptor));
			add_commands(std::forward<Args>(values)...);
		}

		template <typename T>
		void add_param(const std::string& name, const std::string& helpmsg, T&& value, bool optional = false)
		{
			add_param(program_parameter { name, helpmsg, std::forward<T>(value), optional } );
		}
		void add_param(program_parameter&& param)
		{
			m_params.emplace_back(std::forward<program_parameter>(param));
		}
		void add_params(program_parameter&& param)
		{
			add_param(std::forward<program_parameter>(param));
		}
		template <typename... Args>
		void add_params(program_parameter&& param, Args&&... values)
		{
			add_param(std::forward<program_parameter>(param));
			add_params(std::forward<Args>(values)...);
		}

		void usage();
		void parse_options(void);
		/*!
		 *\brief Will execute the command given on the command line.
		 *\return The result of the command. If no command was specified, it returns -1
		 */
		int execute();
		void parse_parameters(void);
	};

}

#endif
