
#include <system_error>
#include <vector>

#include <corecpp/diagnostic.h>
#include <corecpp/visibility.h>
#include <corecpp/expected.h>
#include <corecpp/io/event.h>


namespace corecpp
{

static corecpp::diagnostic::channel& io_channel()
{
	static auto& channel = corecpp::diagnostic::manager::get_channel("corecpp::io");
	return channel;
}

_internal corecpp::diagnostic::event_producer& io_logger()
{
	static auto logger = corecpp::diagnostic::event_producer(io_channel());
	return logger;
}


int ioloop::run()
{
	std::vector<epoll_event> events { 32 };

	m_loop = true;
	while (m_loop)
	{
		while (!m_posted_events.empty() && m_loop)
		{
			m_posted_events.front()();
			m_posted_events.pop();
		};

		if (!m_loop)
			break;

		io_logger().info("epoll: waiting");
		int nevent  = epoll_wait(m_fd, events.data(), static_cast<int>(events.size()), -1);
		if (nevent < 0 && errno != EINTR) [[unlikely]]
			throw_system_error(errno, std::system_category());

		io_logger().info(std::format("epoll: got {} events", std::to_string(nevent)));
		for (int current = 0; current < nevent; ++current)
		{
			io_logger().info(std::format("epoll: processing event {}", std::to_string(current)));
			epoll_event epev = events[current];
			auto it = m_events.find(epev.data.fd);
			if (it == m_events.end())
				continue;
			event *ev = it->second;
			ev->act(epev.events);
		}

		if (!m_loop)
			break;

		while (!m_posted_events.empty() && m_loop)
		{
			m_posted_events.front()();
			m_posted_events.pop();
		};

	}

	return 0;
}

int ioloop::run_for(std::chrono::milliseconds ms)
{
	timer_event stopper
	{
		*this,
		[this](timer_event&, uint64_t)
		{
			exit();
		}
	};
	stopper.expires_in(std::chrono::duration_cast<std::chrono::seconds>(ms));
	return run();
}

int ioloop::run_timeout(std::chrono::milliseconds ms, timeout_action&& on_timeout)
{
	timer_event stopper
	{
		*this,
		[this, on_timeout = std::move(on_timeout)](timer_event&, uint64_t)
		{
			exit();
			on_timeout();//TODO before or after exit()?
		}
	};
	stopper.expires_in(std::chrono::duration_cast<std::chrono::seconds>(ms));
	return run();

}


void ioloop::add(int fd, event& e, uint32_t events)
{
	io_logger().info(std::format("Add fd {}", std::to_string(fd)));

	auto [it, inserted] = m_events.emplace(fd, &e);
	ensure(inserted, "fd already registered");

	epoll_event event { .events = events, .data = { .fd = fd } };
	if ( epoll_ctl(m_fd, EPOLL_CTL_ADD, fd, &event) != 0 ) [[unlikely]]
	{
		m_events.erase(fd);
		throw_system_error(errno, std::system_category());
	}
}

void ioloop::modify(int fd, uint32_t events)
{
	io_logger().info("Modify fd", std::to_string(fd));

	epoll_event event { .events = events, .data = { .fd = fd } };
	if ( epoll_ctl(m_fd, EPOLL_CTL_MOD, fd, &event) != 0 ) [[unlikely]]
		throw_system_error(errno, std::system_category());
}

void ioloop::remove(int fd)
{
	io_logger().info("Remove fd", std::to_string(fd));

	if ( epoll_ctl(m_fd, EPOLL_CTL_DEL, fd, nullptr) != 0 ) [[unlikely]]
		throw_system_error(errno, std::system_category());
	m_events.erase(fd);
}


}
