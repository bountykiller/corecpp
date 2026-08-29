#ifndef CORE_CPP_BUFFER_H
#define CORE_CPP_BUFFER_H


#include <cstddef>
#include <cstdlib>
#include <cstring>

extern "C" {
	#include <sys/socket.h>
}

#include <new>
#include <span>
#include <utility>

#include <corecpp/algorithm.h>
#include <corecpp/expected.h>

namespace corecpp
{

_internal corecpp::diagnostic::event_producer& io_logger();

struct bytes final
{
	using value_type = unsigned long long;
	constexpr bytes(value_type value) : m_value { value }
	{}
private:
	value_type m_value;
};

struct kilobytes final
{
	using value_type = unsigned long long;
	constexpr kilobytes(value_type value) : m_value { value }
	{}
	constexpr bytes to_bytes() const noexcept
	{
		return m_value * 1024;
	}
private:
	value_type m_value;
};

struct megabytes final
{
	using value_type = unsigned long long;
	constexpr megabytes(value_type value) : m_value { value }
	{}
	constexpr bytes to_bytes() const noexcept
	{
		return m_value * 1024 * 1024;
	}
	constexpr kilobytes to_kilobytes() const noexcept
	{
		return m_value * 1024;
	}
private:
	value_type m_value;
};

struct gigabytes final
{
	using value_type = unsigned long long;
	constexpr gigabytes(value_type value) : m_value { value }
	{}
	constexpr bytes to_bytes() const noexcept
	{
		return m_value * 1024 * 1024 * 1024;
	}
	constexpr kilobytes to_kilobytes() const noexcept
	{
		return m_value * 1024 * 1024;
	}
	constexpr megabytes to_megabytes() const noexcept
	{
		return m_value * 1024;
	}
private:
	value_type m_value;
};

constexpr bytes operator ""_B(unsigned long long value) noexcept
{
	return bytes { value };
}
constexpr kilobytes operator ""_KB(unsigned long long value) noexcept
{
	return kilobytes { value };
}
constexpr megabytes operator ""_MB(unsigned long long value) noexcept
{
	return megabytes { value };
}
constexpr gigabytes operator ""_GB(unsigned long long value) noexcept
{
	return gigabytes { value };
}


class read_buffer
{
public:
	using value_type = std::byte;
	using size_type = std::size_t;

	read_buffer() = default;

	explicit read_buffer(size_type capacity)
	: m_buffer { static_cast<value_type*>(std::malloc(capacity)) },
	  m_capacity { capacity },
	  m_filled { 0 },
	  m_consumed { 0 }
	{
	}

	~read_buffer()
	{
		std::free(m_buffer);
	}

	read_buffer(const read_buffer&) = delete;
	read_buffer& operator=(const read_buffer&) = delete;

	read_buffer(read_buffer&& other) noexcept
	: m_buffer { std::exchange(other.m_buffer, nullptr) },
	  m_capacity { other.m_capacity },
	  m_consumed { other.m_consumed }
	{
		other.clear();
	}

	read_buffer& operator=(read_buffer&& other) noexcept
	{
		if ( this == &other ) [[unlikely]]
			return *this;
		std::free(m_buffer);
		m_buffer = std::exchange(other.m_buffer, nullptr);
		m_capacity = other.m_capacity;
		m_consumed = other.m_consumed;
		other.clear();
		return *this;
	}

	std::byte* data() noexcept
	{
		return m_buffer;
	}

	const std::byte* data() const noexcept
	{
		return m_buffer;
	}

	size_type capacity() const noexcept
	{
		return m_capacity;
	}

	bool empty() const noexcept
	{
		return (m_filled == m_consumed);
	}

	size_type in_avail() const noexcept
	{
		ensure(m_filled >= m_consumed);
		return m_filled - m_consumed;
	}

	std::span<std::byte> in() const noexcept
	{
		return { &m_buffer[m_consumed], m_filled - m_consumed };
	}

	void reserve(size_type new_capacity)
	{
		if ( new_capacity <= m_capacity )
			return;
		value_type* new_buffer = static_cast<value_type*>(std::realloc(m_buffer, new_capacity));
		if ( !new_buffer ) [[unlikely]]
			throw std::bad_alloc();
		m_buffer = new_buffer;
		m_capacity = new_capacity;
	}

	void clear() noexcept
	{
		m_consumed = m_filled = 0;
	}

	std::span<std::byte> consume(size_type consumed) noexcept
	{
		ensure(m_capacity >= m_filled);
		ensure(m_filled >= m_consumed);

		m_consumed += consumed;
		ensure(m_filled >= m_consumed, "consumed more bytes than available");
		value_type* start = &m_buffer[m_filled];
		return { start, consumed };
	}

	ssize_t read(int fd)
	{
		ensure(m_capacity >= m_filled);
		remove_consumed(); //TODO sub-optimal as we will move data on every read
		size_type unfilled = m_capacity - m_filled;
		ssize_t res;
		while (true)
		{
			res = ::read(fd, &m_buffer[m_filled], unfilled);
			if ( res < 0) [[unlikely]]
			{
				switch (errno)
				{
					case EAGAIN: /* aka EWOULDBLOCK */
						return res;
					case EINTR:
						continue;
					default:
						corecpp::throw_system_error(errno, std::system_category());
				}
			}
		}

		m_filled += res;
		ensure(m_capacity >= m_filled);
		return res;
	}

	/**
	 * Receives data from a socket and stores it in the buffer.
	 *
	 * @param fd The file descriptor of the socket.
	 * @param flags The flags to use for the receive operation.
	 * @param available The (min) amount available for reading
	 *
	 * @retval 0 data have been received
	 * @retval EAGAIN no data available, try later
	 * @retval ECONNRESET The connection have been reset by the peer
	 * @retval EPIPE The connection have been lost
	 *
	 * @throw system_error if an unexpected error happen
	 */
	int receive(int fd, const flags<msgflags> flags, size_type available = 0)
	{
		ensure(m_capacity >= m_filled);
		size_type unfilled = m_capacity - m_filled;
		if (unfilled < available)
		{
			if (m_consumed > 0)
			{
				remove_consumed();
				unfilled = m_capacity - m_filled;
			}

			if (unfilled < available)
			{
				reserve(std::max(corecpp::next_power_of_2(available), size_t { 1024 }));
				unfilled = m_capacity - m_filled;
			}
		}

		ensure(unfilled >= available);
		ssize_t res;
		while (true)
		{
			res = ::recv(fd, &m_buffer[m_filled], unfilled, flags.get());
			if ( res < 0 ) [[unlikely]]
			{
				switch (errno)
				{
					case EAGAIN: /* aka EWOULDBLOCK */
					case EPIPE:
					case ECONNRESET:
						return errno;
					case EINTR:
						continue;
					default:
						corecpp::throw_system_error(errno, std::system_category());
				}
			}
			break;
		}

		m_filled += res;
		io_logger().debug(std::format("fd {}: received {} bytes", fd, res),
						  std::format("{}/{} available", m_filled - m_consumed, m_capacity));
		ensure(m_capacity >= m_filled);
		return 0;
	}

	void remove_consumed()
	{
		m_filled -= m_consumed;
		::memmove(m_buffer, &m_buffer[m_consumed], m_filled);
		m_consumed = 0;
	}

private:
	value_type* m_buffer = nullptr;
	size_type m_capacity = 0;
	size_type m_filled = 0;
	size_type m_consumed = 0;
};



class write_buffer
{
public:
	using value_type = std::byte;
	using size_type = std::size_t;

	write_buffer() = default;

	explicit write_buffer(size_type capacity)
	: m_buffer { static_cast<value_type*>(std::malloc(capacity)) },
	m_capacity { capacity },
	m_filled { 0 },
	m_consumed { 0 }
	{
	}

	~write_buffer()
	{
		std::free(m_buffer);
	}

	write_buffer(const write_buffer&) = delete;
	write_buffer& operator=(const write_buffer&) = delete;

	write_buffer(write_buffer&& other) noexcept
	: m_buffer { std::exchange(other.m_buffer, nullptr) },
	m_capacity { other.m_capacity },
	m_filled { other.m_filled },
	m_consumed { other.m_consumed }
	{
		other.clear();
	}

	write_buffer& operator=(write_buffer&& other) noexcept
	{
		if ( this == &other ) [[unlikely]]
			return *this;
		std::free(m_buffer);
		m_buffer = std::exchange(other.m_buffer, nullptr);
		m_capacity = other.m_capacity;
		m_filled = other.m_filled;
		m_consumed = other.m_consumed;
		other.clear();
		return *this;
	}

	std::byte* data() noexcept
	{
		return m_buffer;
	}

	const std::byte* data() const noexcept
	{
		return m_buffer;
	}

	size_type capacity() const noexcept
	{
		return m_capacity;
	}

	bool empty() const noexcept
	{
		return (m_filled == m_consumed);
	}

	size_type out_avail() const noexcept
	{
		ensure(m_filled >= m_consumed);
		return m_filled - m_consumed;
	}

	std::span<std::byte> out() const noexcept
	{
		return { &m_buffer[m_consumed], m_filled - m_consumed };
	}

	void enqueue(std::span<const std::byte> data)
	{
		ensure(m_capacity >= m_filled);
		size_type free_data = m_capacity - m_filled;
		if (data.size() > free_data)
		{
			remove_consumed();
			free_data = m_capacity - m_filled;

			if (data.size() > free_data)
			{
				reserve(std::max(corecpp::next_power_of_2(data.size() + m_filled), size_type { 1024 }));
				free_data = m_capacity - m_filled;
			}
		}

		ensure(free_data >= data.size());
		memcpy(&m_buffer[m_filled], data.data(), data.size());
		m_filled += data.size();
	}

	void reserve(size_type new_capacity)
	{
		if ( new_capacity <= m_capacity )
			return;
		value_type* new_buffer = static_cast<value_type*>(std::realloc(m_buffer, new_capacity));
		if ( !new_buffer ) [[unlikely]]
			throw std::bad_alloc();
		m_buffer = new_buffer;
		m_capacity = new_capacity;
	}
	void clear() noexcept
	{
		m_filled = m_consumed = 0;
	}

	ssize_t write(int fd)
	{
		ensure(m_filled >= m_consumed);
		size_type unconsumed = m_filled - m_consumed;
		ssize_t res;
		while (true)
		{
			res = ::write(fd, &m_buffer[m_filled], unconsumed);
			if ( res < 0) [[unlikely]]
			{
				switch (errno)
				{
					case EAGAIN: /* aka EWOULDBLOCK */
						return res;
					case EINTR:
						continue;
					default:
						corecpp::throw_system_error(errno, std::system_category());
				}
			}
		}

		m_consumed += res;
		ensure(m_filled >= m_consumed);
		return res;
	}

	/**
	 * Receives data from a socket and stores it in the buffer.
	 *
	 * @param fd The file descriptor of the socket.
	 * @param flags The flags to use for the receive operation.
	 *
	 * @retval 0 data have been received
	 * @retval EAGAIN no data available, try later
	 * @retval ECONNRESET The connection have been reset by the peer
	 * @retval EPIPE The connection have been lost
	 *
	 * @throw system_error if an unexpected error happen
	 */
	int send(int fd, const flags<msgflags> flags)
	{
		ensure(m_filled >= m_consumed);
		if ( m_filled == 0 )
		{
			io_logger().info(std::format("fd {}: Nothing to send", fd));
			return 0;
		}

		size_type unconsumed = m_filled - m_consumed;
		ssize_t res;
		while (true)
		{
			res = ::send(fd, &m_buffer[m_consumed], unconsumed, flags.get());
			if ( res < 0 ) [[unlikely]]
			{
				switch (errno)
				{
					case EAGAIN: /* aka EWOULDBLOCK */
					case EPIPE:
					case ECONNRESET:
						return errno;
					case EINTR:
						continue;
					default:
						corecpp::throw_system_error(errno, std::system_category());
				}
			}
			break;
		}

		m_consumed += res;
		io_logger().debug(std::format("fd {}: sent {} bytes", fd, res),
						  std::format("{}/{} sent", m_consumed, m_filled));
		ensure(m_filled >= m_consumed);
		return 0;
	}

	void remove_consumed()
	{
		size_type unconsumed = m_filled - m_consumed;
		::memmove(m_buffer, &m_buffer[m_consumed], unconsumed);
		m_consumed = 0;
		m_filled = unconsumed;
	}

private:
	value_type* m_buffer = nullptr;
	size_type m_capacity = 0;
	size_type m_filled = 0;
	size_type m_consumed = 0;
};
}

#endif
