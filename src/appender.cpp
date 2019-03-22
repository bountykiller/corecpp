#include <chrono>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cstdio>
#include <cassert>
#include <cmath>
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
					std::time_t time = std::chrono::system_clock::to_time_t(ev.timestamp);
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

std::string formatter::format(const event& ev) const
{
	std::string res;
	for(const auto& fn : m_functions)
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

void file_appender::roll()
{
	std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::ostringstream rolled_name;
	rolled_name << m_filename << "." << std::put_time(std::localtime(&time), "%F-%T");
	m_stream.close();
	//waiting for filesystem API to have something more c++ish
	rename(m_filename.c_str(), rolled_name.str().c_str());
	m_stream.open(m_filename, std::ios_base::out | std::ios_base::app);
}

void file_appender::append(const event& ev)
{
	if (m_max_file_size && m_file_size > m_max_file_size)
		roll();
	std::string message = m_formatter.format(ev);
	m_stream.write(message.c_str(), message.length());
	m_file_size += message.length() * sizeof(char);
}



periodic_file_appender::periodic_file_appender(std::string& file_tmpl_name, const std::string& format, std::chrono::minutes period)
: m_file_tmpl_name(file_tmpl_name), m_stream(), m_period(period), m_timestamp (), m_formatter(format)
{}

void periodic_file_appender::roll()
{
	std::ostringstream rolled_name;
	std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	rolled_name << std::put_time(std::localtime(&time), m_file_tmpl_name.c_str());
	m_stream.close();
	m_stream.open(rolled_name.str(), std::ios_base::out | std::ios_base::app);
}

void periodic_file_appender::append(const event& ev)
{
	if (m_timestamp < ev.timestamp)
	{
		roll();
		long factor = (long)std::ceil((ev.timestamp - m_timestamp) / m_period);
		m_timestamp += m_period * factor;
	}
	assert (m_timestamp >= ev.timestamp);
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
