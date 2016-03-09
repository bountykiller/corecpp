#include <chrono>
#include <iostream>
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
				std::string str(start, pos - 2);
				m_functions.append([str](std::string& str, const event& ev) { str.append(str); } );
				m_functions.append([](std::string& str, const event& ev) {
					std::ostringstream oss;
					oss << std::put_time(ev.timestamp, "%F %T");
					str.append(oss.str());
				});
				break;
			}
			case 'm':
			{
				std::string str(start, pos - 2);
				m_functions.append([str](std::string& str, const event& ev) { str.append(str); } );
				m_functions.append([](std::string& str, const event& ev) { str.append(ev.message); } );
				break;
			}
			case 'l':
			{
				std::string str(start, pos - 2);
				m_functions.append([str](std::string& str, const event& ev) { str.append(str); } );
				m_functions.append([](std::string& str, const event& ev) { str.append(to_string(ev.line)); } );
				break;
			}
			case 'd':
			{
				std::string str(start, pos - 2);
				m_functions.append([str](std::string& str, const event& ev) { str.append(str); } );
				m_functions.append([](std::string& str, const event& ev) { str.append(ev.details); } );
				break;
			}
			case 'f':
			{
				std::string str(start, pos - 2);
				m_functions.append([str](std::string& str, const event& ev) { str.append(str); } );
				m_functions.append([](std::string& str, const event& ev) { str.append(ev.file); } );
				break;
			}
			case 'g':
			{
				std::string str(start, pos - 2);
				m_functions.append([str](std::string& str, const event& ev) { str.append(str); } );
				m_functions.append([](std::string& str, const event& ev) { str.append(to_string(ev.level)); } );
				break;
			}
			case '%':
			default:
				continue;
		}
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
: m_filename(filename), m_formatter(format), m_stream(filename, std::ios::out | std::ios::ate), m_file_size(0), m_max_file_size(0)
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
	m_stream.write(message);
	m_file_size += message.length() * sizeof(char);
}


void periodic_file_appender::append(const event& ev)
{
	if (m_period > ev->timestamp)
	{
		//TODO: roll files
		m_period += m_duration;
	}
	std::string message = m_formatter.format(ev);
	m_stream.write(message);
}


/* TODO: bold & friends
 */
void console_appender::append(const event& ev)
{
	std::string message = m_formatter.format(ev);
	switch (ev.level)
	{
		case diagnostic_level::fatal:
			std::cerr << graphic_rendition<sgr_p::fg_red> << message << graphic_rendition<sgr_p::all_off> << std::endl;
			break;
		case diagnostic_level::error:
			std::cerr << graphic_rendition<sgr_p::fg_red> << message << graphic_rendition<sgr_p::all_off> << std::endl;
			break;
		case diagnostic_level::warning:
			std::cerr << graphic_rendition<sgr_p::fg_yellow> << message << graphic_rendition<sgr_p::all_off> << std::endl;
			break;
		case diagnostic_level::faillure:
			std::cerr << graphic_rendition<sgr_p::fg_yellow> << message << graphic_rendition<sgr_p::all_off> << std::endl;
			break;
		case diagnostic_level::success:
			std::cerr << graphic_rendition<sgr_p::fg_green> << message << graphic_rendition<sgr_p::all_off> << std::endl;
			break;
		case diagnostic_level::info:
			std::cerr << message << std::endl;
			break;
		case diagnostic_level::trace:
			std::cerr << graphic_rendition<sgr_p::fg_blue> << message << graphic_rendition<sgr_p::all_off> << std::endl;
			break;
		case diagnostic_level::debug:
			std::cerr << graphic_rendition<sgr_p::fg_blue> << message << graphic_rendition<sgr_p::all_off> << std::endl;
			break;
	}
}


}

}
