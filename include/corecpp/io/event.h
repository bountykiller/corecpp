#ifndef CORE_CPP_EVENT_H
#define CORE_CPP_EVENT_H

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
#include <pthread.h>

#include <corecpp/file.h>
#include <corecpp/except.h>
#include <corecpp/expected.h>
#include <cassert>

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
		void add(int fd, event& e);
		void remove(int fd);

	private:

		bool m_loop;
		std::map<int, event*> m_events;
};


class event
{
public:
	using action = std::function<void(event&)>;

	void act(uintptr_t)
	{
		m_action(*this);
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

	void subscribe(int ident, bool once)
	{
		if ( m_ident != -1 )
		{
			ensure(ident == m_ident); //should I call unsubscribe otherwise?
			return;
		}

		/* TODO: Add once flag in m_loop */
		m_loop.add(ident, *this);
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
		event::subscribe(m_timer.get(), true);
		m_timer.expires_in(timeout);
	}

	void expires_each(std::chrono::seconds timeout)
	{
		event::subscribe(m_timer.get(), false);
		m_timer.expires_each(timeout);
	}

	~timer_event()
	{
		unsubscribe();
	}

private:
	static void on_action(event& ev)
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
			event::subscribe(m_signal.get(), false);
		}

		~signal_event()
		{
			unsubscribe();
		}

	private:
		static void on_action(event& ev)
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

}

#endif
