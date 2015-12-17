#ifndef _ERROR_MANAGER_H_
#define _ERROR_MANAGER_H_

#include <vector>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <functional>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace corecpp
{

enum struct diagnostic_level : short
{
	fatal = 0,
	error = 1,
	warning = 2,
	faillure = 3,
	success = 4,
	info = 5,
	trace = 6,
	debug = 7
};

inline static const std::string& to_string(diagnostic_level level)
{
	using namespace std;
	static const std::string string_table[8] = {
		"FATAL", "ERROR", "WARNING", "FAILLURE",
		"SUCCESS", "INFO", "TRACE", "DEBUG"
	};
	return string_table[(short)level];
}


struct event
{
	diagnostic_level level;
	std::string message;
	std::string details;
	std::string file;
	uint line;
	std::chrono::system_clock::time_point timestamp;
};

inline std::ostream& operator << (std::ostream& os, const event& e)
{
	//std::time_t timestamp = std::chrono::system_clock::to_time_t(e.timestamp);
	//return os << std::put_time(std::localtime(&timestamp), "%F %T") << ":" << to_string(e.level) << " at " << e.file << ":" << e.line << "\t" << e.message << "\t" << e.details;
	return os << to_string(e.level) << " at " << e.file << ":" << e.line << "\t" << e.message << "\t" << e.details;
}


class channel final
{
	struct params
	{
		diagnostic_level level;
		bool store_events;
		bool output_events;
		std::mutex output_mutex;
		std::mutex store_mutex;
		std::ostream& output;
		std::vector<event> store;
		params(diagnostic_level l, bool store, bool output, std::ostream& output_stream)
		: level(l), store_events(store), output_events(output), output(output_stream)
		{
		}
	};
	std::shared_ptr<params> m_params; //using a shared_ptr in order to minimize the amount of time during which the channel is locked when logging
	mutable std::mutex m_mutex;
	static void diagnose(event message, params& parameters);
	channel() = default;
	channel(const channel&) = default;//at least for now
	channel& operator=(const channel&) = default;
public:
	channel(channel&& other)
	: m_params(std::move(other.m_params))
	{}
	channel(diagnostic_level level, bool store, bool output, std::ostream& output_stream);
	channel& operator=(channel&& other)
	{
		m_params = std::move(other.m_params);
		return *this;
	}
	diagnostic_level level() const
	{ 
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_params->level;
	}
	void level(diagnostic_level value)
	{ 
		std::lock_guard<std::mutex> lock(m_mutex);
		m_params->level = value; 
	}
	
	bool is_storing_events() const
	{ 
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_params->store_events; 
	}
	void is_storing_events(bool value)
	{ 
		std::lock_guard<std::mutex> lock(m_mutex);
		if(!value)
			m_params->store.clear();
		m_params->store_events = value;
	}
	
	bool is_outputing_events() const
	{ 
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_params->output_events; 
	}
	void is_outputing_events(bool value)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_params->output_events = value;
	}
	
	void swap(channel& other)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		std::lock_guard<std::mutex> other_lock(other.m_mutex);
		std::swap(m_params, other.m_params);
	}
	
	void diagnose(diagnostic_level level, std::string message, std::string file, uint line);
	void diagnose(diagnostic_level level, std::string message, std::string details, std::string file, uint line);
	void diagnose(diagnostic_level level, std::string message, std::function<std::string()> details, std::string file, uint line);
};


class manager
{
	static manager& instance();
	std::unordered_map<std::string, channel> m_channels;
	std::mutex m_mutex;
	channel& _get_channel(const std::string& name);
public:
	template <typename T>
	static channel& get_channel(T&& name)
	{
		return manager::instance()._get_channel(std::forward<T>(name));
	}
};


class event_producer
{
	channel& m_channel;
public:
	event_producer(channel& log_channel)
	: m_channel(log_channel)
	{}
	template <typename T>
	event_producer(T&& channel_name)
	: m_channel(manager::get_channel(std::forward<T>(channel_name)))
	{}
	void fatal(std::string message, std::string file, uint line) const
	{
		m_channel.diagnose(diagnostic_level::fatal, std::move(message), std::move(file), line);
	}
	void fatal(std::string message, std::string details, std::string file, uint line) const
	{
		m_channel.diagnose(diagnostic_level::fatal, std::move(message), std::move(details), std::move(file), line);
	}
	void fatal(std::string message, std::function<std::string()> details, std::string file, uint line)
	{
		m_channel.diagnose(diagnostic_level::fatal, std::move(message), std::move(details), std::move(file), line);
	}
	
	void error(std::string message, std::string file, uint line) const
	{
		m_channel.diagnose(diagnostic_level::error, std::move(message), std::move(file), line);
	}
	void error(std::string message, std::string details, std::string file, uint line) const
	{
		m_channel.diagnose(diagnostic_level::error, std::move(message), std::move(details), std::move(file), line);
	}
	void error(std::string message, std::function<std::string()> details, std::string file, uint line)
	{
		m_channel.diagnose(diagnostic_level::error, std::move(message), std::move(details), std::move(file), line);
	}
	
	void warn(std::string message, std::string file, uint line) const
	{
		m_channel.diagnose(diagnostic_level::warning, std::move(message), std::move(file), line);
	}
	void warn(std::string message, std::string details, std::string file, uint line) const
	{
		m_channel.diagnose(diagnostic_level::warning, std::move(message), std::move(details), std::move(file), line);
	}
	void warn(std::string message, std::function<std::string()> details, std::string file, uint line)
	{
		m_channel.diagnose(diagnostic_level::warning, std::move(message), std::move(details), std::move(file), line);
	}
	
	void fail(std::string message, std::string file, uint line) const
	{
		m_channel.diagnose(diagnostic_level::faillure, std::move(message), std::move(file), line);
	}
	void fail(std::string message, std::string details, std::string file, uint line) const
	{
		m_channel.diagnose(diagnostic_level::faillure, std::move(message), std::move(details), std::move(file), line);
	}
	void fail(std::string message, std::function<std::string()> details, std::string file, uint line)
	{
		m_channel.diagnose(diagnostic_level::faillure, std::move(message), std::move(details), std::move(file), line);
	}
	
	void success(std::string message, std::string file, uint line) const
	{
		m_channel.diagnose(diagnostic_level::success, std::move(message), std::move(file), line);
	}
	void success(std::string message, std::string details, std::string file, uint line) const
	{
		m_channel.diagnose(diagnostic_level::success, std::move(message), std::move(details), std::move(file), line);
	}
	void success(std::string message, std::function<std::string()> details, std::string file, uint line)
	{
		m_channel.diagnose(diagnostic_level::success, std::move(message), std::move(details), std::move(file), line);
	}
	
	void info(std::string message, std::string file, uint line) const
	{
		m_channel.diagnose(diagnostic_level::info, std::move(message), std::move(file), line);
	}
	void info(std::string message, std::string details, std::string file, uint line) const
	{
		m_channel.diagnose(diagnostic_level::info, std::move(message), std::move(details), std::move(file), line);
	}
	void info(std::string message, std::function<std::string()> details, std::string file, uint line)
	{
		m_channel.diagnose(diagnostic_level::info, std::move(message), std::move(details), std::move(file), line);
	}
	
	void trace(std::string message, std::string file, uint line) const
	{
		m_channel.diagnose(diagnostic_level::trace, std::move(message), std::move(file), line);
	}
	void trace(std::string message, std::string details, std::string file, uint line) const
	{
		m_channel.diagnose(diagnostic_level::trace, std::move(message), std::move(details), std::move(file), line);
	}
	void trace(std::string message, std::function<std::string()> details, std::string file, uint line)
	{
		m_channel.diagnose(diagnostic_level::trace, std::move(message), std::move(details), std::move(file), line);
	}
	
	void debug(std::string message, std::string file, uint line) const
	{
		m_channel.diagnose(diagnostic_level::debug, std::move(message), std::move(file), line);
	}
	void debug(std::string message, std::string details, std::string file, uint line) const
	{
		m_channel.diagnose(diagnostic_level::debug, std::move(message), std::move(details), std::move(file), line);
	}
	void debug(std::string message, std::function<std::string()> details, std::string file, uint line)
	{
		m_channel.diagnose(diagnostic_level::debug, std::move(message), std::move(details), std::move(file), line);
	}
};

inline void swap(channel& a, channel& b)
{
	a.swap(b);
};


}



#endif

