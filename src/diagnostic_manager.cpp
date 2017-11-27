#include <iostream>
#include <utility>
#include <corecpp/diagnostic.h>

namespace corecpp
{

namespace diagnostic
{


manager& manager::instance()
{
	static manager res;
	return res;
}

static const corecpp::enum_map<diagnostic_level> diagnostic_level_strings =
{
	{ diagnostic_level::fatal, "FATAL" },
	{ diagnostic_level::error, "ERROR" },
	{ diagnostic_level::warning, "WARNING" },
	{ diagnostic_level::faillure, "FAILLURE" },
	{ diagnostic_level::success, "SUCCESS" },
	{ diagnostic_level::info, "INFO" },
	{ diagnostic_level::trace, "TRACE" },
	{ diagnostic_level::debug, "DEBUG" }
};

const std::string& to_string(diagnostic_level type)
{
	return corecpp::etos(type, diagnostic_level_strings);
}

bool from_string(const std::string& str, diagnostic_level& type, std::locale l)
{
	return corecpp::stoe(str, type, diagnostic_level_strings, l);
}


void channel::diagnose(event message, params& parameters)
{
	parameters.output.append(message);
}


void channel::diagnose(diagnostic_level level, std::string message, std::function<std::string()> details, std::string file, uint line)
{
	decltype(m_params) p;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		p = m_params;
	}
	if(p->level < level) return;
	if(p->level > level || level == diagnostic_level::debug)
		diagnose({ level, std::move(message), details(), std::move(file), line, std::chrono::system_clock::now() }, *p);
	else
		diagnose({ level, std::move(message), {}, std::move(file), line, std::chrono::system_clock::now() }, *p);
}


void channel::diagnose(diagnostic_level level, std::string message, std::string details, std::string file, uint line)
{
	decltype(m_params) p;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		p = m_params;
	}
	if(p->level < level) return;
	if(p->level > level || level == diagnostic_level::debug)
		diagnose({ level, std::move(message), std::move(details), std::move(file), line, std::chrono::system_clock::now() }, *p);
	else
		diagnose({ level, std::move(message), {}, std::move(file), line, std::chrono::system_clock::now() }, *p);
}


void channel::diagnose(diagnostic_level level, std::string message, std::string file, uint line)
{
	decltype(m_params) p;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		p = m_params;
	}
	if(p->level < level) return;
	diagnose({ level, std::move(message), {}, std::move(file), line, std::chrono::system_clock::now() }, *p);
}



channel& manager::_get_channel(const std::string& name)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	auto found = m_channels.find(name);
	if(found == m_channels.end())
		return _default_channel();
	else
		return found->second;
}

channel& manager::_default_channel()
{
	static console_appender builtin("[%t] %g %f:%l :: %m %d");
	if (!m_default)
	{
		m_default.reset(new channel(diagnostic_level::success, builtin));
		m_default->diagnose(diagnostic_level::warning, "no default appender provided, using builtin.", __FILE__, __LINE__);
	}
	return *m_default;
}


}

}
