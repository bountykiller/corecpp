#ifndef CORE_CPP_EVENT_H
#define CORE_CPP_EVENT_H

#include <cassert>

#include <chrono>
#include <format>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <system_error>

#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>

#include <corecpp/diagnostic.h>
#include <corecpp/except.h>
#include <corecpp/expected.h>
#include <corecpp/file.h>
#include <corecpp/flags.h>
#include <corecpp/visibility.h>
#include <corecpp/io/buffer.h>
#include <corecpp/io/socket.h>

namespace corecpp
{

class event;

/*
 * NOTE:
 * Here I implement an ioloop based on Linux epoll API.
 * Latter i'll implement one based on BSD kevent API.
 */
class ioloop final : public filedesc
{
	public:
		using post_action = std::function<void (void)>;
		using timeout_action = std::function<void (void)>;

		ioloop() :
			filedesc { epoll_create1(EPOLL_CLOEXEC) },
			m_loop { true },
			m_events {},
			m_posted_events {}
		{}

		/**
		* Exit the event loop.
		*/
		void exit()
		{
			m_loop = false;
		}

		/**
		* Run the event loop.
		*
		* @return The result of the event loop execution.
		*/
		int run();
		int run_for(std::chrono::milliseconds ms);
		int run_timeout(std::chrono::milliseconds ms, timeout_action&& on_timeout);
		void add(int fd, event& e, uint32_t events);
		void modify(int fd, uint32_t events);
		void remove(int fd);

		void post(post_action&& action)
		{
			m_posted_events.emplace(std::move(action));
		}

	private:

		bool m_loop;
		std::map<int, event*> m_events;
		std::queue<post_action> m_posted_events;
};


class event
{
public:
	using action = std::function<void(event&, uint32_t)>;

	void act(uint32_t events)
	{
		static constexpr std::array<uint32_t, 2> known_events = { EPOLLIN, EPOLLOUT };
		for (const uint32_t ev : known_events)
		{
			if ( !(ev & events) )
				continue;
			on_event(ev);
			if ( m_once & ev )
				unsubscribe(events);
		}
	}

protected:
	event(ioloop& loop, int id) :
		m_loop { loop },
		m_id { id },
		m_events { 0 },
		m_once { 0 }
	{}

	event(const event&) = delete;
	event(event&& other) :
		m_loop { other.m_loop },
		m_id { other.m_id },
		m_events { other.m_events },
		m_once { other.m_once }
	{
		other.m_events = 0;
		other.m_id = -1;
	}

	event& operator =(event&& other) = delete; /* not possible because m_loop is a reference */

	virtual void on_event(uint32_t event) = 0;

	void subscribe(bool once, uint32_t event)
	{
		io_logger().info(std::format("event {}: subscribe to {} (once: {})", m_id, event, once));
		if ( once )
			m_once |= event;
		else
			m_once &= ~event;

		if ( (m_events & event) == event )
			return; // already monitored

		m_events |= event;
		if ( m_events == event )
			m_loop.add(m_id, *this, m_events);
		else
			m_loop.modify(m_id, m_events);
	}

	void unsubscribe(uint32_t event)
	{
		ensure(m_events & event, "Try to unsubscribe non-subscribed event");

		io_logger().info(std::format("event {}: unsubscribe to {}", m_id, event));

		m_events &= ~event;
		if ( m_events )
			m_loop.modify(m_id, m_events);
		else
			m_loop.remove(m_id);
	}

	virtual ~event() noexcept
	{
		if ( m_events )
			m_loop.remove(m_id);
	}

	ioloop& m_loop;

private:
	int m_id;
	uint32_t m_events;
	uint32_t m_once; /* each bit tell us if once is active for the given event */
};


class timer : public filedesc
{
	public:
		//TODO: allow several clock
		timer()
		: filedesc(timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC))
		{}

		/**
		* Schedule a timer to trigger every specified duration.
		*
		* \param timeout The duration after which the timer should trigger.
		*/
		void expires_each(std::chrono::seconds timeout)
		{
			const struct itimerspec value =
			{
				.it_interval = { .tv_sec = timeout.count(), .tv_nsec = 0 },
				.it_value    = { .tv_sec = timeout.count(), .tv_nsec = 0 }
			};

			schedule(value);
		}

		/**
		* Schedule a timer to trigger every specified duration.
		*
		* \param timeout The duration after which the timer should trigger.
		*/
		void expires_in(std::chrono::seconds timeout)
		{
			const struct itimerspec value =
			{
				.it_interval = { .tv_sec = 0, .tv_nsec = 0 },
				.it_value    = { .tv_sec = timeout.count(), .tv_nsec = 0 },
			};

			schedule(value);
		}

	private:
		/**
		* Schedule a timer to trigger after a specified duration.
		*
		* \param timeout The duration after which the timer should trigger.
		*/
		void schedule(const struct itimerspec &timeout)
		{
			int res = timerfd_settime(m_fd, 0, &timeout, nullptr);
			/* if we receive ECANCELED, then the timer has been scheduled properly, see the man page for details */
			if (res < 0 && errno != ECANCELED) [[unlikely]]
			{
				corecpp::throws<std::system_error>(errno, std::generic_category());
			}
		}
};


/**
 * \brief Represents a timer event that can be scheduled and canceled.
 *
 * The timer_event class is used to manage timer events within an ioloop.
 * It allows scheduling of timer events and provides automatic cancellation
 * when the timer_event object is destroyed.
 */
class timer_event final : public timer, public event
{
public:
	using timer_action = std::function<void(timer_event&, uint64_t)>;

	timer_event(ioloop& loop, timer_action &&action) :
		timer { },
		event { loop, m_fd },
		m_action(std::move(action))
	{}

	void expires_in(std::chrono::seconds timeout)
	{
		event::subscribe(true, EPOLLIN);
		timer::expires_in(timeout);
	}

	void expires_each(std::chrono::seconds timeout)
	{
		event::subscribe(false, EPOLLIN);
		timer::expires_each(timeout);
	}

	~timer_event()
	{
	}

private:
	void on_event(uint32_t) override
	{
		uint64_t ntimes;
		int res = read(m_fd, &ntimes, sizeof(ntimes));
		ensure(res == sizeof(ntimes));
		m_action(*this, ntimes);
	}

	timer_action m_action;
};

/**
 * \brief Represents a signalfd.
 *
 * Wraps a signalfd created from a set of signals to be caught.
 */
class signal_fd : public filedesc
{
	public:
		signal_fd() : filedesc(-1)
		{
		}

		void wait_for(const sigset_t& set)
		{
			int res = signalfd(m_fd, &set, SFD_CLOEXEC);
			if (res < 0) [[unlikely]]
				corecpp::throws<std::system_error>(errno, std::system_category());
			m_fd = res;
		}
};


/**
 * \brief Represents a signal event that can be caught by an ioloop.
 *
 * The signal_event class blocks the given signals and turns them into
 * readable events on a signalfd, so they can be handled by the ioloop.
 * The signals are unblocked and the subscription removed when the
 * signal_event is destroyed.
 */
class signal_event : public signal_fd, public event
{
	public:
		using signal_action = std::function<void(ioloop& loop, int signal, pid_t sender)>;

		signal_event(ioloop& loop, signal_action &&action) :
			signal_fd {},
			event { loop, m_fd },
			m_action(std::move(action))
		{
		}

		void async_wait(const sigset_t& set)
		{
			// signals must be blocked to be delivered to the signalfd instead
			// of taking their default disposition
			if ( pthread_sigmask(SIG_BLOCK, &set, nullptr) != 0 ) [[unlikely]]
				corecpp::throws<std::system_error>(errno, std::system_category());
			signal_fd::wait_for(set); //should be done 1st to ensure we have a valid file descriptor
			event::subscribe(false, EPOLLIN);
		}

		~signal_event()
		{
		}

	private:
		void on_event(uint32_t) override
		{
			struct signalfd_siginfo info;
			int res = read(m_fd, &info, sizeof(info));
			ensure(res == sizeof(info));
			m_action(m_loop, info.ssi_signo, info.ssi_pid);
		}

		signal_action m_action;
};

/**
 * \brief Represents an asynchronous stream socket driven by an ioloop.
 *
 * The async_streamsocket class is used to manage stream socket events within
 * an ioloop. It allows scheduling of connect, read and write events and
 * provides automatic cancellation when the async_streamsocket object is
 * destroyed.
 */
template <typename DomainT>
class async_streamsocket : public streamsocket<DomainT>, public event
{
	public:
		using read_action = std::function<size_t(async_streamsocket&, std::span<const std::byte>)>;
		using write_action = std::function<void(async_streamsocket&, int)>;
		using connect_action = std::function<void(async_streamsocket&, int)>;

		async_streamsocket(ioloop& loop, int protocol) :
			streamsocket<DomainT> { protocol },
			event { loop, streamsocket<DomainT>::m_fd },
			m_read_buffer  { },
			m_minread { 0 },
			m_write_buffer { },
			m_read_action  { },
			m_write_action { },
			m_connect_action { }
		{
		}

		async_streamsocket(ioloop &loop, streamsocket<DomainT>&& other) :
			streamsocket<DomainT> { std::move(other) },
			event { loop, streamsocket<DomainT>::m_fd },
			m_read_buffer  { },
			m_minread { 0 },
			m_write_buffer { },
			m_read_action  { },
			m_write_action { },
			m_connect_action { }
		{
		}

		/* TODO:
		 * can't work because the callbacks may owns a reference to other
		 * -> in async_accept, forward a (non-async) streamsocket<DomainT>
		 */
		async_streamsocket(async_streamsocket&& other) :
			streamsocket<DomainT> { std::move(other) },
			event { std::move(other) },
			m_read_buffer  { std::move(other.m_read_buffer) },
			m_minread { other.m_minread },
			m_write_buffer { std::move(other.m_write_buffer) },
			m_read_action  { std::move(other.m_read_action) },
			m_write_action { std::move(other.m_write_action) },
			m_connect_action { std::move(other.m_connect_action) }
		{
		}
/*
		async_streamsocket& operator =(async_streamsocket&& other)
		{
			if (*this == &other) [[ unlikely ]]
				return *this;

			streamsocket<DomainT>::operator =(std::move(other));
			event::operator = (std::move(other)); // don't work here since assigning references does not work that way
			m_read_buffer   = std::move(other.m_read_buffer);
			m_minread       = std::move(other.m_minread);
			m_write_buffer  = std::move(other.m_write_buffer);
			m_read_action   = std::move(other.m_read_action);
			m_write_action  = std::move(other.m_write_action);
			m_connect_action= std::move(other.m_connect_action);
			return *this;
		}
*/
		/**
		* Asynchronously connect the socket to the given address.
		*
		* The socket is put in non-blocking mode and subscribed to the ioloop
		* until the connection is established.
		*
		* \param dest The address to connect to.
		* \param action The callback invoked once the connection is settled,
		*        with 0 on success or the errno value on failure.
		*/
		void async_connect(const DomainT::address_type &dest, connect_action &&action)
		{
			int flags = fcntl(F_GETFL, 0);
			if ( flags != -1 ) [[likely]]
				fcntl(F_SETFL, flags | O_NONBLOCK);

			m_connect_action = std::move(action);
			int res = streamsocket<DomainT>::connect(dest);
			if ( res == 0 ) [[likely]]
			{
				m_loop.post(
					[this]()
					{
						on_connect(0);
					});
				return;
			}
			if ( errno != EINPROGRESS && errno != EINTR && errno != EAGAIN ) [[unlikely]]
			{
				m_loop.post(
					[this]()
					{
						on_connect(errno);
					});
				return;
			}

			/* the connection is being established, wait for the socket to become writable */
			event::subscribe(true, EPOLLOUT);
		}

		/**
		* Asynchronously read data from the socket into the given buffer.
		*
		* \param bytes The amount of bytes we want to receive
		* \param action The callback invoked once data has been read, with the
		*        number of bytes read (or -1 on error).
		*/
		void async_recv(size_t bytes, read_action &&action)
		{
			m_read_action = std::move(action);
			m_minread = bytes;
			event::subscribe(false, EPOLLIN);

			if ( m_read_buffer.in_avail() > 0 && m_minread <= m_read_buffer.in_avail() )
			{
				m_loop.post(
					[this]()
					{
						consume_received();
					}
				);
			}
		}

		/**
		* Asynchronously send data from the given buffer through the socket.
		*
		* \param buffer The buffer containing the data to send.
		* \param action The callback invoked once data has been sent, with the
		*        number of bytes sent (or -1 on error).
		*/
		void async_send(std::span<const std::byte> buffer, write_action &&action)
		{
			bool schedule = m_write_buffer.empty();
			m_write_buffer.enqueue(buffer);
			if ( action )
			{
				m_write_action = std::move(action);
				if ( schedule )
					event::subscribe(false, EPOLLOUT);
			}
		}

		~async_streamsocket()
		{
		}

	private:
		void on_event(uint32_t events)
		{
			if ( events & EPOLLIN )
				on_read();
			if ( events & EPOLLOUT )
			{
				if ( m_connect_action )
					on_connect();
				else
					on_write();
			}
		}

		void on_connect()
		{
			int err = 0;
			io_logger().info(std::format("fd {}: connected", streamsocket<DomainT>::get()), std::to_string(err), __FILE__, __LINE__);
			socklen_t len = sizeof(err);
			getsockopt(streamsocket<DomainT>::m_fd, SOL_SOCKET, SO_ERROR, &err, &len);
			on_connect(err);
		}

		void on_connect(int error)
		{
			m_connect_action(*this, error);
			m_connect_action = nullptr;
		}

		void on_read()
		{
			io_logger().info(std::format("fd {}: receiving data", streamsocket<DomainT>::get()));
			int available;
			int ret = streamsocket<DomainT>::ioctl(FIONREAD, &available);
			if (ret < 0) [[unlikely]]
				corecpp::throw_system_error(errno, std::system_category());

			ret = m_read_buffer.receive(streamsocket<DomainT>::get(), msgflags::nosignal | msgflags::dont_wait, available);
			switch ( ret )
			{
				case 0:
					consume_received();
					break;
				case EPIPE:
				case ECONNRESET:
					m_read_action(*this, std::span<const std::byte> { });
					break;
				case EAGAIN:  [[unlikely]]
					return;
				default:
					throws<unexpected_error>("unexpected code received: " + std::to_string(ret));
			}
		}

		void consume_received()
		{
			do
			{
				size_t consumed = m_read_action(*this, m_read_buffer.in());
				m_read_buffer.consume(consumed);
				io_logger().trace(std::format("fd {}: consumed {} bytes", streamsocket<DomainT>::get(), consumed),
								  std::format("remaining: {}", m_read_buffer.in_avail()));
			}
			while ( m_read_buffer.in_avail() > 0 && m_read_buffer.in_avail() >= m_minread);
		}

		void on_write()
		{
			io_logger().info(std::format("fd {}: sending data", streamsocket<DomainT>::get()), __FILE__, __LINE__);
			auto res = m_write_buffer.send(streamsocket<DomainT>::get(), msgflags::nosignal | msgflags::dont_wait);
			if ( m_write_buffer.empty() )
				event::unsubscribe(EPOLLOUT);

			if ( m_write_action )
				m_write_action(*this, res);
		}

		read_buffer    m_read_buffer;
		size_t         m_minread;
		write_buffer   m_write_buffer;
		read_action    m_read_action;
		write_action   m_write_action;
		connect_action m_connect_action;
};

/**
 * \brief Represents an asynchronous stream socket acceptor driven by an ioloop.
 *
 * The async_socketacceptor class is used to accept stream connections within
 * an ioloop. It schedules an accept event and invokes the provided callback
 * each time a new connection is received.
 */
template <typename DomainT>
class async_socketacceptor : public socket_acceptor<DomainT>, public event
{
	public:
		using accept_action = std::function<void(async_streamsocket<DomainT>&&)>;

		async_socketacceptor(ioloop& loop, int protocol)
		: socket_acceptor<DomainT> { protocol },
		  event { loop, socket_acceptor<DomainT>::m_fd },
		  m_accept_action { }
		{
		}

		/**
		* Asynchronously accept new connections.
		*
		* The acceptor is subscribed to the ioloop and the given callback is
		* invoked each time a new connection is received.
		*
		* \param action The callback invoked for each accepted connection.
		*/
		void async_accept(accept_action &&action)
		{
			m_accept_action = std::move(action);
			event::subscribe(false, EPOLLIN);
		}

		~async_socketacceptor()
		{
		}

	private:
		void on_event(uint32_t)
		{
			streamsocket<DomainT> new_sock = socket_acceptor<DomainT>::accept();
			io_logger().info(std::format("fd {}: new connection", socket_acceptor<DomainT>::get()),
			                 std::to_string(new_sock.get()));
			m_accept_action(async_streamsocket<DomainT> { event::m_loop, std::move(new_sock) });
		}

		accept_action m_accept_action;
};

/**
 * \brief Represents an asynchronous datagram socket driven by an ioloop.
 *
 * The async_datagramsocket class is used to manage datagram socket events
 * within an ioloop. It allows scheduling of send and receive events and
 * provides automatic cancellation when the async_datagramsocket object is
 * destroyed.
 */
template <typename DomainT>
class async_datagramsocket : public datagramsocket<DomainT>, public event
{
	public:
		using recv_action = std::function<void(ssize_t, const typename DomainT::address_type&)>;
		using send_action = std::function<void(ssize_t)>;

		async_datagramsocket(ioloop& loop, int protocol)
		: datagramsocket<DomainT> { protocol },
		  event { loop, datagramsocket<DomainT>::m_fd },
		  m_recv_buffer { },
		  m_send_buffer { },
		  m_dest { },
		  m_recv_action { },
		  m_send_action { }
		{
		}

		/**
		* Asynchronously receive a datagram into the given buffer.
		*
		* \param buffer The buffer that will receive the datagram.
		* \param action The callback invoked once a datagram has been received,
		*        with the number of bytes read (or -1 on error) and the source
		*        address of the datagram.
		*/
		void async_recvfrom(std::span<std::byte> buffer, recv_action &&action)
		{
			m_recv_buffer = buffer;
			m_recv_action = std::move(action);
			event::subscribe(false, EPOLLIN);
		}

		/**
		* Asynchronously send a datagram to the given destination.
		*
		* \param buffer The buffer containing the datagram to send.
		* \param dest The address to send the datagram to.
		* \param action The callback invoked once the datagram has been sent,
		*        with the number of bytes sent (or -1 on error).
		*/
		void async_sendto(const std::span<const std::byte> buffer, const DomainT::address_type &dest, send_action &&action)
		{
			m_send_buffer = buffer;
			m_dest = dest;
			m_send_action = std::move(action);
			event::subscribe(false, EPOLLOUT);
		}

		~async_datagramsocket()
		{
		}

	private:

		void on_event(uint32_t events)
		{
			// we are called with a reference to ourselves, since we inherit
			// from event, so we can cast it back
			if ( events & EPOLLIN )
				on_recv();
			if ( events & EPOLLOUT )
				on_send();
		}

		void on_recv()
		{
			typename DomainT::address_type recv_src;
			ssize_t n = datagramsocket<DomainT>::recvfrom(m_recv_buffer, msgflags::nosignal, &recv_src);
			m_recv_action(n, recv_src);
		}

		void on_send()
		{
			ssize_t n = datagramsocket<DomainT>::sendto(m_send_buffer, msgflags::dont_wait | msgflags::nosignal, m_dest);
			m_send_action(n);
		}

		std::span<std::byte> m_recv_buffer;
		std::span<const std::byte> m_send_buffer;
		DomainT::address_type m_dest;
		msgflags m_flags;
		recv_action m_recv_action;
		send_action m_send_action;
};

}

#endif
