#include <iostream>
#include <corecpp/diagnostic_manager.h>

namespace corecpp
{

manager& manager::instance()
{
	static manager result;
	return result;
}


channel::channel(diagnostic_level level, bool store, bool output, std::ostream& output_stream)
: m_params { std::make_shared<params>(level, store, output, output_stream) }
{
}


void channel::diagnose(event message, params& parameters)
{
	if(parameters.output_events)
	{
		std::lock_guard<std::mutex> lock(parameters.output_mutex);
		parameters.output << message << std::endl;
	}
	if(parameters.store_events)
	{
		std::lock_guard<std::mutex> lock(parameters.store_mutex);
		parameters.store.emplace_back(std::move(message));
	}
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
	{
		auto inserted = m_channels.emplace(name, channel{diagnostic_level::debug, false, true, std::clog});
		return inserted.first->second;
	}
	else
		return found->second;
}


}
