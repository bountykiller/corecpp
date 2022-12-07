/*
 * command_line.cpp
 *
 *  Created on: May 14, 2010
 *      Author: bountykiller
 */
#include <iostream>
#include <corecpp/cli/command_line.h>
#include <corecpp/cli/graphics.h>
#include <corecpp/cli/parser.h>

namespace corecpp
{

corecpp::diagnostic::event_producer& long_option_parser::logger()
{
	static auto& channel = corecpp::diagnostic::manager::get_channel("cli::long_option_parser");
	static auto logger = corecpp::diagnostic::event_producer(channel);
	return logger;
}

corecpp::diagnostic::event_producer& short_option_parser::logger()
{
	static auto& channel = corecpp::diagnostic::manager::get_channel("cli::long_option_parser");
	static auto logger = corecpp::diagnostic::event_producer(channel);
	return logger;
}


corecpp::diagnostic::event_producer& option_reader::logger()
{
	static auto& channel = corecpp::diagnostic::manager::get_channel("cli::option_reader");
	static auto logger = corecpp::diagnostic::event_producer(channel);
	return logger;
}


corecpp::diagnostic::event_producer& command_line::logger()
{
	static auto& channel = corecpp::diagnostic::manager::get_channel("cli::command_line");
	static auto logger = corecpp::diagnostic::event_producer(channel);
	return logger;
}


corecpp::diagnostic::event_producer& command_line_parser::logger()
{
	static auto& channel = corecpp::diagnostic::manager::get_channel("cli::command_line_parser");
	static auto logger = corecpp::diagnostic::event_producer(channel);
	return logger;
}

expected<unsigned int, std::invalid_argument> command_line_parser::parse_options(void)
{
	unsigned int read_options = 0;

	while (const char* token = m_command_line.peek())
	{
		if (*token != '-')
			break;
		std::string param(m_command_line.read());
		if (param.substr(0,2) == "--")
		{
			if (param.length() == 2)
				break; /* seeing -- on the command line means we have no more options to read */
			std::string value = "";
			auto pos = param.find('=');
			if (pos == std::string::npos)
			{
				//param with no value => value is let to an empty string
				param = param.substr(2);
			}
			else
			{
				value = param.substr(pos+1);
				param = param.substr(2, pos - 2);
			}
			logger().trace(corecpp::concat<std::string>({"option ", param, " [", value, "]"}), __FILE__, __LINE__);
			auto option = get_option(param);
			if (option == m_options.end())
				return std::invalid_argument { corecpp::concat<std::string>({ "--", param, ": unknown option" }) };
			++read_options;
			if (!value.empty())
			{
				long_option_parser parser { value };
				try {
					option->read(parser);
				}
				catch (std::exception& e)
				{
					return std::invalid_argument { corecpp::concat<std::string>({ param, ": ", e.what() }) };
				}
			}
			else
				option->use_default();
		}
		else if (param[0] == '-')
		{
			param = param.substr(1);
			logger().trace(corecpp::concat<std::string>({"option(s) ", param}), __FILE__, __LINE__);
			// loop because we can have multiple params
			auto iter = param.begin();
			while(true)
			{
				char shortname = *iter;
				auto option = get_option(shortname);
				if (option == m_options.end())
					return std::invalid_argument { corecpp::concat<std::string>({ "-", std::string { 1, shortname }, ": unknown option" }) };
				++read_options;
				++iter;
				if (iter == param.end())
				{
					short_option_parser parser { m_command_line };
					try {
						option->read(parser);//consume the parameter
					}
					catch (std::exception& e)
					{
						return std::invalid_argument { corecpp::concat<std::string>({ "-", std::string { 1, shortname }, ": invalid value" }) };
					}
					break;
				}
				else
					option->use_default();
			}
		}
	}

	return read_options;
}


void command_line_parser::usage()
{
	//TODO: check if stdout isatty?
	std::cout << graphic_rendition_v<sgr_p::fg_yellow> << "Usage: \n  "
			<< graphic_rendition_v<sgr_p::fg_cyan>
			<< m_command_line.program() << m_command_line.commands();
	if (m_options.size() > 0)
		std::cout << " [<options>]";
	if (m_commands.size() > 0)
		std::cout << " <commands>";
	for (const auto& param : m_params)
	{
		if (param.is_required())
			std::cout << " <" << param.name() << '>';
		else
			std::cout << " [<" << param.name() << ">]";
	}
	std::cout << graphic_rendition_v<sgr_p::all_off>;

	if (m_options.size() > 0)
	{
		std::cout << graphic_rendition_v<sgr_p::fg_yellow> << "\n\nOptions:" << graphic_rendition_v<sgr_p::all_off>;
		for (const auto& option : m_options)
		{
			std::cout << '\n' << option.helpmsg();
		}
	}

	if (m_commands.size() > 0)
	{
		std::cout << graphic_rendition_v<sgr_p::fg_yellow> << "\n\nCommands:" << graphic_rendition_v<sgr_p::all_off>;
		for (const auto& command : m_commands)
		{
			std::cout << '\n' << command.helpmsg();
		}
	}

	if (m_params.size() > 0)
	{
		std::cout << graphic_rendition_v<sgr_p::fg_yellow> << "\n\nParameters:" << graphic_rendition_v<sgr_p::all_off>;
		for (const auto& param : m_params)
		{
			std::cout << '\n' << param.helpmsg();
		}
	}
	std::cout << '\n' << std::endl;
}

expected<unsigned int, std::invalid_argument> command_line_parser::parse_parameters(void)
{
	unsigned int parsed = 0;
	argument_parser parser { m_command_line };
	for (const auto& parameter : m_params)
	{
		if (m_command_line.peek() != nullptr)
		{
			try {
				parameter.read(parser);
			}
			catch (std::exception& e)
			{
				return std::invalid_argument { corecpp::concat<std::string>({ parameter.name(), ": ", e.what() }) };
			}
			parsed++;
		}
		else if (parameter.is_required())
			return std::invalid_argument { corecpp::concat<std::string>({ parameter.name(), " is required." }) };
	}
	return parsed;
}

std::function<int()> command_line_parser::parse_command(void)
{
	const char* command = m_command_line.peek();
	if (!command) // all parameter have already been read
		return {};

	auto cmd = get_command(command);
	if (cmd == m_commands.end())
		return {};

	logger().trace(corecpp::concat<std::string>({"got ", command}), __FILE__, __LINE__);
	m_command_line.consume();
	m_command_line.store_command();
	return [this, cmd]() { return cmd->execute(m_command_line); };
}

}
