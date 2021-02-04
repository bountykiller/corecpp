/*
 * command_line.cpp
 *
 *  Created on: May 14, 2010
 *      Author: bountykiller
 */
#include <iostream>
#include <corecpp/cli/command_line.h>
#include <corecpp/cli/parser.h>

namespace corecpp
{

corecpp::diagnostic::event_producer& command_line::logger()
{
	static auto& channel = corecpp::diagnostic::manager::get_channel("command_line");
	static auto logger = corecpp::diagnostic::event_producer(channel);
	return logger;
}

corecpp::diagnostic::event_producer& command_line_parser::logger()
{
	static auto& channel = corecpp::diagnostic::manager::get_channel("command_line_parser");
	static auto logger = corecpp::diagnostic::event_producer(channel);
	return logger;
}

void command_line_parser::parse_options(void)
{
	while(const char* token = m_command_line.peek())
	{
		if (*token != '-')
			break;
		std::string param(m_command_line.read());
		logger().trace(corecpp::concat<std::string>({"parsing ", param}), __FILE__, __LINE__);
		if (param.substr(0,2) == "--")
		{
			std::string value = "";
			auto pos = param.find('=');
			if(pos == std::string::npos)
			{
				//param with no value => value is let to an empty string
				param = param.substr(2);
			}
			else
			{
				value = param.substr(pos);
				param = param.substr(2, param.length() - 2 - pos);
			}
			auto option = get_option(param);
			if (option == m_options.end())
				throw std::invalid_argument(param);
			long_option_parser parser { value };
			option->read(parser);
		}
		else if (param[0] == '-')
		{
			param = param.substr(1);
			const char* value = m_command_line.peek();
			if(value && *value == '-') //another option
				value = nullptr;
			//for loop because we can have multiple params
			for(auto iter = param.begin(); iter != param.end(); ++iter)
			{
				char shortname = *iter;
				auto option = get_option(shortname);
				if(option == m_options.end())
					throw std::invalid_argument(std::string(1, shortname));
				short_option_parser parser { m_command_line };
				option->read(parser);//consume the parameter
			}
		}
	}
}

void command_line_parser::usage()
{
	std::cout << "Usage : " << m_command_line.program() << m_command_line.commands();
	if (m_options.size() > 0)
		std::cout << " [<options>]";
	if (m_commands.size() > 0)
		std::cout << " <command>";
	for (const auto& param : m_params)
	{
		if (param.is_required())
			std::cout << " <" << param.name() << '>';
		else
			std::cout << " [<" << param.name() << ">]";
	}

	if (m_options.size() > 0)
	{
		std::cout << "\nOptions:\n";
		for (const auto& option : m_options)
		{
			std::cout << option.helpmsg() << '\n';
		}
	}

	if (m_commands.size() > 0)
	{
		std::cout << "\nCommands:\n";
		for (const auto& command : m_commands)
		{
			std::cout << command.helpmsg() << '\n';
		}
	}

	if (m_params.size() > 0)
	{
		std::cout << "\nParameters:\n";
		for (const auto& param : m_params)
		{
			std::cout << param.helpmsg() << '\n';
		}
	}
	std::cout << std::endl;
}

void command_line_parser::parse_parameters(void)
{
	argument_parser parser { m_command_line };
	for (const auto& parameter : m_params)
	{
		 /* read the token from the command line.
		  * won't work for booleans
		  * ==> make a cli directory, move stuff there, put parsers in a separate file and create a parameter parser
		  */
		if (m_command_line.peek() != nullptr)
			parameter.read(parser);
		else if (parameter.is_required())
			corecpp::throws<value_error>(parameter.name());
	}
}

int command_line_parser::execute()
{
	const char* command = m_command_line.read();
	if (!command) // all parameter have already been read
		return -1;

	auto cmd = get_command(command);
	if (cmd == m_commands.end())
		return -1;

	logger().trace(corecpp::concat<std::string>({"calling ", command}), __FILE__, __LINE__);
	m_command_line.store_command();
	return cmd->execute(m_command_line);
}

}
