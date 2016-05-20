#include <chrono>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <corecpp/diagnostic.h>
#include <corecpp/graphics.h>

namespace corecpp
{

namespace diagnostic
{

formatter::formatter(const std::string& format)
{
	auto start = format.cbegin();
	auto pos = format.cbegin();
	while (true)
	{
		while (pos != format.cend() && *pos != '%')
			++pos;
		if (pos == format.cend())
			break;
		switch (*++pos)
		{
			case 't':
			{
				std::string str(start, pos - 1);
				m_functions.emplace_back([str](std::string& msg, const event& ev) {
					std::ostringstream oss;
					time_t time = std::chrono::system_clock::to_time_t(ev.timestamp);
					oss << str << std::put_time(std::localtime(&time), "%F %T");
					msg.append(oss.str());
				});
				start = ++pos;
				break;
			}
			case 'm':
			{
				std::string str(start, pos - 1);
				m_functions.emplace_back([str](std::string& msg, const event& ev) { msg.append(str).append(ev.message); } );
				start = ++pos;
				break;
			}
			case 'l':
			{
				std::string str(start, pos - 1);
				m_functions.emplace_back([str](std::string& msg, const event& ev) { msg.append(str).append(std::to_string(ev.line)); } );
				start = ++pos;
				break;
			}
			case 'd':
			{
				std::string str(start, pos - 1);
				m_functions.emplace_back([str](std::string& msg, const event& ev) { msg.append(str).append(ev.details); } );
				start = ++pos;
				break;
			}
			case 'f':
			{
				std::string str(start, pos - 1);
				m_functions.emplace_back([str](std::string& msg, const event& ev) { msg.append(str).append(ev.file); } );
				start = ++pos;
				break;
			}
			case 'g':
			{
				std::string str(start, pos - 1);
				m_functions.emplace_back([str](std::string& msg, const event& ev) { msg.append(str).append(to_string(ev.level)); } );
				start = ++pos;
				break;
			}
			case '%':
			default:
				continue;
		}
	}
	if (start != format.cend())
	{
		std::string str(start, pos);
		m_functions.emplace_back([str](std::string& msg, const event& ev) { msg.append(str); } );
	}
}

std::string formatter::format(const event& ev)
{
	std::string res;
	for(formatter::format_fn_type& fn : m_functions)
		fn(res, ev);
	return res;
}

file_appender::file_appender(std::string filename, const std::string& format)
: m_filename(filename), m_stream(filename, std::ios::out | std::ios::ate), m_formatter(format), m_file_size(0), m_max_file_size(0)
{
	if (m_stream.fail())
	{
		m_stream.close();
		throw std::runtime_error("Opening log file failed!");
	}
	m_file_size = m_stream.tellp(); // FIXME: approximation, could be inacurate
}


void file_appender::append(const event& ev)
{
	if (m_max_file_size && m_file_size > m_max_file_size)
	{
		//TODO: roll files
	}
	std::string message = m_formatter.format(ev);
	m_stream.write(message.c_str(), message.length());
	m_file_size += message.length() * sizeof(char);
}


void periodic_file_appender::append(const event& ev)
{
	if (m_period > ev.timestamp)
	{
		//TODO: roll files
		m_period += m_duration;
	}
	std::string message = m_formatter.format(ev);
	m_stream.write(message.c_str(), message.length());
}


/* TODO: bold & friends
 */
void console_appender::append(const event& ev)
{
	std::string message = m_formatter.format(ev);
	switch (ev.level)
	{
		case diagnostic_level::fatal:
			std::cerr << corecpp::graphic_rendition<sgr_p::fg_red>::value << message << corecpp::graphic_rendition<sgr_p::all_off>::value << std::endl;
			break;
		case diagnostic_level::error:
			std::cerr << corecpp::graphic_rendition<sgr_p::fg_red>::value << message << corecpp::graphic_rendition<sgr_p::all_off>::value << std::endl;
			break;
		case diagnostic_level::warning:
			std::cerr << corecpp::graphic_rendition<sgr_p::fg_yellow>::value << message << corecpp::graphic_rendition<sgr_p::all_off>::value << std::endl;
			break;
		case diagnostic_level::faillure:
			std::cerr << corecpp::graphic_rendition<sgr_p::fg_yellow>::value << message << corecpp::graphic_rendition<sgr_p::all_off>::value << std::endl;
			break;
		case diagnostic_level::success:
			std::cerr << corecpp::graphic_rendition<sgr_p::fg_green>::value << message << corecpp::graphic_rendition<sgr_p::all_off>::value << std::endl;
			break;
		case diagnostic_level::info:
			std::cerr << message << std::endl;
			break;
		case diagnostic_level::trace:
			std::cerr << corecpp::graphic_rendition<sgr_p::fg_blue>::value << message << corecpp::graphic_rendition<sgr_p::all_off>::value << std::endl;
			break;
		case diagnostic_level::debug:
			std::cerr << corecpp::graphic_rendition<sgr_p::fg_blue>::value << message << corecpp::graphic_rendition<sgr_p::all_off>::value << std::endl;
			break;
	}
}


}

}