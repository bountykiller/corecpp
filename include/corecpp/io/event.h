#ifndef CORE_CPP_EVENT_H
#define CORE_CPP_EVENT_H

#include <cassert>

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <system_error>

#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>

#include <corecpp/file.h>
#include <corecpp/flags.h>
#include <corecpp/except.h>
#include <corecpp/expected.h>
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
		ioloop()
		: filedesc(epoll_create1(EPOLL_CLOEXEC)), m_loop(true)
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
		void add(int fd, event& e, uint32_t events = EPOLLIN);
		void modify(int fd, uint32_t events);
		void remove(int fd);

	private:

		bool m_loop;
		std::map<int, event*> m_events;
};


class event
{
public:
	using action = std::function<void(event&, uint32_t)>;

	void act(uint32_t events)
	{
		m_action(*this, events);
		if ( m_once )
			unsubscribe();
	}

protected:
	event(ioloop& loop, event::action &&action) :
	m_loop { loop },
	m_action { std::move(action) },
	m_ident { -1 },
	m_once { false }
	{}

	void subscribe(int ident, bool once, uint32_t events)
	{
		if ( m_ident != -1 )
		{
			ensure(ident == m_ident); //should I call unsubscribe otherwise?
			m_loop.modify(ident, events);
			m_once = once;
			return;
		}

		/* TODO: Add once flag in m_loop */
		m_loop.add(ident, *this, events);
		m_ident = ident;
		m_once = once;
	}

	void unsubscribe()
	{
		if ( m_ident == -1 )
			return;
		m_loop.remove(m_ident);
		m_ident = -1;
	}

	~event() noexcept
	{
		/* unsubscribe must have been called in child class
		 * The reason is that if it is called here, the underlying fd may already have been closed.
		 */
		ensure(m_ident == -1);
	}

	ioloop& m_loop;

private:
	event::action m_action;
	int m_ident;
	bool m_once;
};


class timer final : public filedesc
{
	public:
		//TODO: allow several clock
		timer()
		: filedesc(timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC))
		{}

		/**
		* Schedule a timer to trigger every specified duration.
		*
		* @param timeout The duration after which the timer should trigger.
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
		* @param timeout The duration after which the timer should trigger.
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
		* @param timeout The duration after which the timer should trigger.
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

using timer_action = std::function<void(uint64_t)>;

/**
 * \brief Represents a timer event that can be scheduled and canceled.
 *
 * The timer_event class is used to manage timer events within an ioloop.
 * It allows scheduling of timer events and provides automatic cancellation
 * when the timer_event object is destroyed.
 */
class timer_event final : public event
{
public:
	timer_event(ioloop& loop, timer_action &&action) :
		event { loop, on_action },
		m_timer { },
		m_action(std::move(action))
	{}

	void expires_in(std::chrono::seconds timeout)
	{
		event::subscribe(m_timer.get(), true, EPOLLIN);
		m_timer.expires_in(timeout);
	}

	void expires_each(std::chrono::seconds timeout)
	{
		event::subscribe(m_timer.get(), false, EPOLLIN);
		m_timer.expires_each(timeout);
	}

	~timer_event()
	{
		unsubscribe();
	}

private:
	static void on_action(event& ev, uint32_t)
	{
		// Here the tricks is that we will be called with the reference to the event as the 1st parameter
		// Since we inherit from event, this reference is also a reference to ourselves, so we can cast it here
		timer_event& timer = static_cast<timer_event&>(ev);
		uint64_t ntimes;
		int res = read(timer.m_timer.get(), &ntimes, sizeof(ntimes));
		ensure(res == sizeof(ntimes));
		timer.m_action(ntimes);
	}

	timer m_timer;
	timer_action m_action;
};

/**
 * \brief Represents a signalfd.
 *
 * Wraps a signalfd created from a set of signals to be caught.
 */
class signal_fd final : public filedesc
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

using signal_action = std::function<void(ioloop& loop, int signal, pid_t sender)>;

/**
 * \brief Represents a signal event that can be caught by an ioloop.
 *
 * The signal_event class blocks the given signals and turns them into
 * readable events on a signalfd, so they can be handled by the ioloop.
 * The signals are unblocked and the subscription removed when the
 * signal_event is destroyed.
 */
class signal_event : public event
{
	public:
		signal_event(ioloop& loop, signal_action &&action)
		: event { loop, on_action },
		  m_signal {},
		  m_action(std::move(action))
		{
		}

		void async_wait(const sigset_t& set)
		{
			// signals must be blocked to be delivered to the signalfd instead
			// of taking their default disposition
			if ( pthread_sigmask(SIG_BLOCK, &set, nullptr) != 0 ) [[unlikely]]
				corecpp::throws<std::system_error>(errno, std::system_category());
			m_signal.wait_for(set); //should be done 1st to ensure we have a valid file descriptor
			event::subscribe(m_signal.get(), false, EPOLLIN);
		}

		~signal_event()
		{
			unsubscribe();
		}

	private:
		static void on_action(event& ev, uint32_t)
		{
			// we are called with a reference to ourselves, since we inherit
			// from event, so we can cast it back
			signal_event& sigevent = static_cast<signal_event&>(ev);
			sigevent.action();
		}

		void action()
		{
			struct signalfd_siginfo info;
			int res = read(m_signal.get(), &info, sizeof(info));
			ensure(res == sizeof(info));
			m_action(m_loop, info.ssi_signo, info.ssi_pid);
		}

		signal_fd m_signal;
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
		using read_action = std::function<void(ssize_t)>;
		using write_action = std::function<void(ssize_t)>;
		using connect_action = std::function<void(int)>;

		async_streamsocket(ioloop& loop, int protocol)
		: streamsocket<DomainT> { protocol },
		  event { loop, on_action },
		  m_read_buffer { },
		  m_write_buffer { },
		  m_read_action { },
		  m_write_action { },
		  m_connect_action { }
		{
		}

		/**
		* Asynchronously connect the socket to the given address.
		*
		* The socket is put in non-blocking mode and subscribed to the ioloop
		* until the connection is established.
		*
		* @param dest The address to connect to.
		* @param action The callback invoked once the connection is settled,
		*        with 0 on success or the errno value on failure.
		*/
		void async_connect(const DomainT::address_type &dest, connect_action &&action)
		{
			m_connect_action = std::move(action);

			int flags = fcntl(F_GETFL, 0);
			if ( flags != -1 ) [[likely]]
				fcntl(F_SETFL, flags | O_NONBLOCK);

			int res = connect(dest);
			if ( res == 0 ) [[likely]]
			{
				m_connect_action(0); //Put it in the ioloop?
				return;
			}
			if ( errno != EINPROGRESS && errno != EINTR && errno != EAGAIN ) [[unlikely]]
			{
				m_connect_action(errno); //Put it in the ioloop?
				return;
			}

			/* the connection is being established, wait for the socket to become writable */
			event::subscribe(streamsocket<DomainT>::m_fd, true, EPOLLOUT);
		}

		/**
		* Asynchronously read data from the socket into the given buffer.
		*
		* @param buffer The buffer that will receive the data.
		* @param action The callback invoked once data has been read, with the
		*        number of bytes read (or -1 on error).
		*/
		void async_recv(std::span<std::byte> buffer, read_action &&action)
		{
			m_read_buffer = buffer;
			m_read_action = std::move(action);
			event::subscribe(streamsocket<DomainT>::m_fd, false, EPOLLIN);
		}

		/**
		* Asynchronously send data from the given buffer through the socket.
		*
		* @param buffer The buffer containing the data to send.
		* @param action The callback invoked once data has been sent, with the
		*        number of bytes sent (or -1 on error).
		*/
		void async_send(std::span<const std::byte> buffer, write_action &&action)
		{
			m_write_buffer = buffer;
			m_write_action = std::move(action);
			event::subscribe(streamsocket<DomainT>::m_fd, false, EPOLLOUT);
		}

		~async_streamsocket()
		{
			unsubscribe();
		}

	private:

		static void on_action(event& ev, uint32_t events)
		{
			// we are called with a reference to ourselves, since we inherit
			// from event, so we can cast it back
			async_streamsocket& socket = static_cast<async_streamsocket&>(ev);
			if ( events & EPOLLIN )
				socket.on_read();
			if ( events & EPOLLOUT )
			{
				if ( socket.m_connect_action )
				{
					socket.on_connect();
					socket.m_connect_action = nullptr;
				}
				else
					socket.on_write();
			}
		}

		void on_connect()
		{
			int err = 0;
			socklen_t len = sizeof(err);
			getsockopt(this->get(), SOL_SOCKET, SO_ERROR, &err, &len);
			m_connect_action(err);
		}

		void on_read()
		{
			m_read_action(this->recv(m_read_buffer, msgflags::none));
		}

		void on_write()
		{
			m_write_action(this->send(m_write_buffer, msgflags::none));
		}

		std::span<std::byte> m_read_buffer;
		std::span<const std::byte> m_write_buffer;
		read_action m_read_action;
		write_action m_write_action;
		connect_action m_connect_action;
};

/**
 * \brief Represents an asynchronous stream socket acceptor driven by an ioloop.
 *
 * The async_socket_acceptor class is used to accept stream connections within
 * an ioloop. It schedules an accept event and invokes the provided callback
 * each time a new connection is received.
 */
template <typename DomainT>
class async_socket_acceptor : public socket_acceptor<DomainT>, public event
{
	public:
		using accept_action = std::function<void(streamsocket<DomainT>)>;

		async_socket_acceptor(ioloop& loop, int protocol)
		: socket_acceptor<DomainT> { protocol },
		  event { loop, on_action },
		  m_accept_action { }
		{
		}

		/**
		* Asynchronously accept new connections.
		*
		* The acceptor is subscribed to the ioloop and the given callback is
		* invoked each time a new connection is received.
		*
		* @param action The callback invoked for each accepted connection.
		*/
		void async_accept(accept_action &&action)
		{
			m_accept_action = std::move(action);
			event::subscribe(socket_acceptor<DomainT>::m_fd, false, EPOLLIN);
		}

		~async_socket_acceptor()
		{
			unsubscribe();
		}

	private:

		static void on_action(event& ev, uint32_t)
		{
			// we are called with a reference to ourselves, since we inherit
			// from event, so we can cast it back
			async_socket_acceptor& acceptor = static_cast<async_socket_acceptor&>(ev);
			acceptor.on_accept();
		}

		void on_accept()
		{
			m_accept_action(socket_acceptor<DomainT>::accept());
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
		  event { loop, on_action },
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
		* @param buffer The buffer that will receive the datagram.
		* @param action The callback invoked once a datagram has been received,
		*        with the number of bytes read (or -1 on error) and the source
		*        address of the datagram.
		*/
		void async_recvfrom(std::span<std::byte> buffer, recv_action &&action)
		{
			m_recv_buffer = buffer;
			m_recv_action = std::move(action);
			event::subscribe(datagramsocket<DomainT>::m_fd, false, EPOLLIN);
		}

		/**
		* Asynchronously send a datagram to the given destination.
		*
		* @param buffer The buffer containing the datagram to send.
		* @param dest The address to send the datagram to.
		* @param action The callback invoked once the datagram has been sent,
		*        with the number of bytes sent (or -1 on error).
		*/
		void async_sendto(const std::span<const std::byte> buffer, const DomainT::address_type &dest, send_action &&action)
		{
			m_send_buffer = buffer;
			m_dest = dest;
			m_send_action = std::move(action);
			event::subscribe(datagramsocket<DomainT>::m_fd, false, EPOLLOUT);
		}

		~async_datagramsocket()
		{
			unsubscribe();
		}

	private:

		static void on_action(event& ev, uint32_t events)
		{
			// we are called with a reference to ourselves, since we inherit
			// from event, so we can cast it back
			async_datagramsocket& socket = static_cast<async_datagramsocket&>(ev);
			if ( events & EPOLLIN )
				socket.on_recv();
			if ( events & EPOLLOUT )
				socket.on_send();
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
