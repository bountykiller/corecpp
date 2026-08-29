#include <chrono>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cstdio>
#include <cassert>
#include <cmath>
#include <corecpp/diagnostic.h>
#include <corecpp/cli/graphics.h>

namespace corecpp
{

namespace diagnostic
{

/**
 * @brief Constructs a formatter object with the specified format string.
 * @param format The format string to be used for formatting events.
 */
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
				m_functions.emplace_back([str](std::string& msg, const record& entry) {
					std::ostringstream oss;
					std::time_t time = std::chrono::system_clock::to_time_t(entry.timestamp);
					oss << str << std::put_time(std::localtime(&time), "%F %T");
					msg.append(oss.str());
				});
				start = ++pos;
				break;
			}
			case 'm':
			{
				std::string str(start, pos - 1);
				m_functions.emplace_back(
					[str](std::string& msg, const record& entry)
					{ msg.append(str).append(entry.message); }
				);
				start = ++pos;
				break;
			}
			case 'l':
			{
				std::string str(start, pos - 1);
				m_functions.emplace_back(
					[str](std::string& msg, const record& entry)
					{ msg.append(str).append(std::to_string(entry.line)); }
				);
				start = ++pos;
				break;
			}
			case 'd':
			{
				std::string str(start, pos - 1);
				m_functions.emplace_back(
					[str](std::string& msg, const record& entry)
					{ msg.append(str).append(entry.details); }
				);
				start = ++pos;
				break;
			}
			case 'f':
			{
				std::string str(start, pos - 1);
				m_functions.emplace_back(
					[str](std::string& msg, const record& entry)
					{
						msg.append(str).append(entry.file);
					});
				start = ++pos;
				break;
			}
			case 'g':
			{
				std::string str(start, pos - 1);
				m_functions.emplace_back(
					[str](std::string& msg, const record& entry)
					{
						msg.append(str).append(to_string(entry.level));
					});
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
		m_functions.emplace_back([str](std::string& msg, const record& entry) { msg.append(str); } );
	}
}

/**
 * @brief Formats the given event according to the formatter's format string.
 * @param entry The event to be formatted.
 * @return The formatted string.
 */
std::string formatter::format(const record& entry) const
{
	std::string res;
	for(const auto& fn : m_functions)
		fn(res, entry);
	return res;
}

/**
 * @brief Constructs a file_appender object with the specified filename and format string.
 * @param filename The name of the file to append events to.
 * @param format The format string to be used for formatting events.
 */
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

/**
 * @brief Rolls the log file by renaming it with a timestamp and opens a new file.
 */
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

/**
 * @brief Appends the given event to the log file.
 * @param entry The event to be appended.
 */
void file_appender::append(const record& entry)
{
	if (m_max_file_size && m_file_size > m_max_file_size)
		roll();
	std::string message = m_formatter.format(entry);
	m_stream.write(message.c_str(), message.length());
	m_file_size += message.length() * sizeof(char);
}

/**
 * @brief Constructs a periodic_file_appender object with the specified file template name, format string, and period.
 * @param file_tmpl_name The template name of the file to append events to.
 * @param format The format string to be used for formatting events.
 * @param period The period after which the log file should be rolled.
 */
periodic_file_appender::periodic_file_appender(std::string& file_tmpl_name, const std::string& format, std::chrono::minutes period)
: m_file_tmpl_name(file_tmpl_name), m_stream(), m_period(period), m_timestamp (), m_formatter(format)
{}

/**
 * @brief Rolls the log file by renaming it with a timestamp and opens a new file.
 */
void periodic_file_appender::roll()
{
	std::ostringstream rolled_name;
	std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	rolled_name << std::put_time(std::localtime(&time), m_file_tmpl_name.c_str());
	m_stream.close();
	m_stream.open(rolled_name.str(), std::ios_base::out | std::ios_base::app);
}

/**
 * @brief Appends the given event to the log file.
 * @param entry The event to be appended.
 */
void periodic_file_appender::append(const record& entry)
{
	if (m_timestamp < entry.timestamp)
	{
		roll();
		long factor = (long)std::ceil((entry.timestamp - m_timestamp) / m_period);
		m_timestamp += m_period * factor;
	}
	assert (m_timestamp >= entry.timestamp);
	std::string message = m_formatter.format(entry);
	m_stream.write(message.c_str(), message.length());
}

/* TODO: bold & friends
 */
/**
 * @brief Appends the given event to the console.
 * @param entry The event to be appended.
 */
void console_appender::append(const record& entry)
{
	std::string message = m_formatter.format(entry);
	switch (entry.level)
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

