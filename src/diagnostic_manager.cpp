#include <iostream>
#include <utility>
#include <corecpp/diagnostic.h>

namespace corecpp
{

namespace diagnostic
{

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
	static console_appender default_appender("[%t] %g %f:%l :: %m %d");
	std::lock_guard<std::mutex> lock(m_mutex);
	auto found = m_channels.find(name);
	if(found == m_channels.end())
	{

		auto inserted = m_channels.emplace(std::make_pair(name, channel(diagnostic_level::info, default_appender)));
		return inserted.first->second;
	}
	else
		return found->second;
}


}

}
