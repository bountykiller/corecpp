
#include <system_error>
#include <vector>

#include <corecpp/diagnostic.h>
#include <corecpp/expected.h>
#include <corecpp/io/event.h>


namespace corecpp
{

static corecpp::diagnostic::channel& io_channel()
{
	static auto& channel = corecpp::diagnostic::manager::get_channel("corecpp::io");
	return channel;
}

static corecpp::diagnostic::event_producer& io_logger()
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
		io_logger().info("epoll: waiting", __FILE__, __LINE__);
		int nevent  = epoll_wait(m_fd, events.data(), static_cast<int>(events.size()), -1);
		if (nevent < 0 && errno != EINTR) [[unlikely]]
			throw_system_error(errno, std::system_category());

		std::cout << "epoll: got " << nevent << " events" << std::endl;
		for (int current = 0; current < nevent; ++current)
		{
			epoll_event epev = events[current];
			auto it = m_events.find(epev.data.fd);
			if (it == m_events.end())
				continue;
			event *ev = it->second;
			ev->act(epev.events);
		}
	}

	return 0;
}

void ioloop::add(int fd, event& e, uint32_t events)
{
	io_logger().info("Add fd", std::to_string(fd), __FILE__, __LINE__);

	auto [it, inserted] = m_events.emplace(fd, &e);
	ensure(inserted, "fd already registered");

	epoll_event event { .events = events, .data = { .fd = fd } };
	if ( epoll_ctl(m_fd, EPOLL_CTL_ADD, fd, &event) != 0 ) [[unlikely]]
	{
		m_events.erase(fd);
		throw_system_error(errno, std::system_category());
	}
}

void ioloop::remove(int fd)
{
	io_logger().info("Remove fd", std::to_string(fd), __FILE__, __LINE__);

	if ( epoll_ctl(m_fd, EPOLL_CTL_DEL, fd, nullptr) != 0 ) [[unlikely]]
		throw_system_error(errno, std::system_category());
	m_events.erase(fd);
}

void throw_system_error(int err, const std::error_category& cat)
{
	throw std::system_error { err, cat };
}


}
