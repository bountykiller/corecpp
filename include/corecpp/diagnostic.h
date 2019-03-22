#ifndef CORECPP_DIAGNOSTIC_H
#define CORECPP_DIAGNOSTIC_H

#include <vector>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <fstream>
#include <functional>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <thread>
#include <chrono>

#include <corecpp/container/ring_buffer.h>
#include <corecpp/container/waiting_queue.h>
#include <corecpp/algorithm.h>

namespace corecpp
{

namespace diagnostic
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

const std::string& to_string(diagnostic_level type);
bool from_string(const std::string& str, diagnostic_level& type, std::locale l);


struct event
{
	diagnostic_level level;
	std::string message;
	std::string details; /* should we use a deferred object here? */
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


struct appender
{
	virtual void append(const event& ev) = 0;
};


struct formatter
{
	using format_fn_type = std::function<void(std::string& str, const event& ev)>;
	std::vector<format_fn_type> m_functions;
public:
	formatter(const std::string& format);
	std::string format(const event& ev) const;
};


class file_appender : public appender
{
	std::string m_filename;
	std::ofstream m_stream;
	formatter m_formatter;
	std::size_t m_file_size;
	std::size_t m_max_file_size;

	void roll();
public:
	file_appender(std::string filename, const std::string& format);
	~file_appender()
	{}
	std::size_t file_size() const
	{
		return m_file_size;
	}
	file_appender& max_file_size(std::size_t size)
	{
		m_max_file_size = size;
		return *this;
	}
	std::size_t max_file_size() const
	{
		return m_max_file_size;
	}
	void append(const event& ev) override;
};



class periodic_file_appender : public appender
{
	std::string m_file_tmpl_name;
	std::ofstream m_stream;
	std::chrono::minutes m_period;
	std::chrono::time_point<std::chrono::system_clock> m_timestamp;
	formatter m_formatter;

	void roll();
public:
	periodic_file_appender(std::string& file_tmpl_name, const std::string& format, std::chrono::minutes period);
	void append(const event& ev) override;
};



class console_appender : public appender
{
	formatter m_formatter;
public:
	console_appender(const std::string& format)
	: m_formatter(format)
	{}
	~console_appender()
	{}
	void append(const event& ev) override;
};


template <typename AppenderT>
class async_appender : public appender
{
	corecpp::waiting_queue<std::unique_ptr<event>> m_queue;
	AppenderT m_appender;
	std::thread m_thread;

	void run()
	{
		while (std::unique_ptr<event> ev = m_queue.pop())
		{
			m_appender.append(ev);
		}
	}
public:
	template <typename ...T>
	async_appender(T&&... args)
	: m_appender(std::forward<T>(args)...), m_queue(), m_thread(run)
	{
	}
	~async_appender()
	{
		m_queue.emplace(nullptr);
		m_thread.join();
	}
	void append(const event& ev) override
	{
#if defined __cplusplus && __cplusplus >= 201402L
		m_queue.emplace(std::make_unique<event>(ev));
#else
		m_queue.emplace(new event(ev));
#endif
	}
};


class channel final
{
	//NOTE: multhi-treading managment will probably change in near future
	struct params
	{
		diagnostic_level level;
		appender& output;
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
	channel(diagnostic_level level, appender& app)
	: m_params(new params {level, app}), m_mutex()
	{
		/* TODO : resolve problem with make_shared
		 * this will throw a bad_alloc on memory allocation error
		 */
	}
	channel& operator=(channel&& other)
	{
		m_params = std::move(other.m_params);
		return *this;
	}

	void swap(channel& other)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		std::lock_guard<std::mutex> other_lock(other.m_mutex);
		std::swap(m_params, other.m_params);
	}

	void set_level(diagnostic_level value)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_params->level = value;
	}

	void diagnose(diagnostic_level level, std::string message, std::string file, uint line);
	void diagnose(diagnostic_level level, std::string message, std::string details, std::string file, uint line);
	void diagnose(diagnostic_level level, std::string message, std::function<std::string()> details, std::string file, uint line);
};



class manager
{
	std::unique_ptr<channel> m_default;
	std::unordered_map<std::string, channel> m_channels;
	std::mutex m_mutex;
	static manager& instance();
	channel& _get_channel(const std::string& name);
	channel& _default_channel();
	manager() = default;
public:
	template <typename T>
	static channel& get_channel(T&& name)
	{
		return instance()._get_channel(std::forward<T>(name));
	}
	static void set_default(diagnostic_level level, appender& app)
	{
		std::lock_guard<std::mutex> lock(instance().m_mutex);
		instance().m_default.reset(new channel(level, app));
	}
	static channel& default_channel()
	{
		return instance()._default_channel();
	}
	/* TODO: Add methods to register/unregister channels
	 */
};


class event_producer
{
	channel& m_channel;
public:
	event_producer(channel& log_channel)
	: m_channel(log_channel)
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

/* TODO: Add a defered_event_producer or somethig like that
 * to allow storing events and writing them at the same time (on a process success/faillure for example)
 */

inline void swap(channel& a, channel& b)
{
	a.swap(b);
};


}

}

#endif

